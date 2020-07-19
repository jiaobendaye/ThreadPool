#include <unistd.h>

#include "ThreadPool.h"

#define DEFAULT_TIME 10						 //管理线程每10s 检测一次
#define MIN_WAIT_TASK_NUM  10      //触发扩容的队列中最小积攒任务数
#define DEFAULT_THREAD_VARY 10     //每次增加或销毁的线程数量

Task::Task(void*(*function)(void *arg), void *arg)
  : m_function(function)
  , m_arg(arg)
{
}

ThreadPool::ThreadPool(size_t minNum, size_t maxNum, size_t queueSize)
  : m_minNum(minNum)
  , m_maxNum(maxNum)
  , m_maxQueSize(queueSize)
{
  m_workers.reserve(minNum + (maxNum - minNum) / 2 );

  for (size_t i=0; i<m_minNum; ++i)
  {
    Thread::ptr thr(new Thread([this](){worker_thread();}, "worker_"+std::to_string(m_workerId)));
    ++m_workerId;
    m_workers.push_back(thr);
  }
  m_liveNum = m_minNum;

  m_adjustThr.reset(new Thread([this](){adjust_thread();}, "adjuster"));

}

ThreadPool::~ThreadPool()
{
  //不再添加新的任务
  m_isShutdown = true;

  //等待现有的任务完成
  m_CSemaExit.wait();

  m_adjustThr.reset();
  m_workers.clear();
}

void ThreadPool::worker_thread()
{
  Task task(nullptr, nullptr);
  while(true)
  {
    //使用信号量模拟条件变量
    while (true)
    {
      m_mutex.lock();
      if (m_taskQueue.empty())
      {
        m_mutex.unlock();
        //printf("thread %s is waiting\n", Thread::GetName().c_str());
        m_CSemaNotEmpty.wait();
        m_mutex.lock();
        if(!m_taskQueue.empty())
        {
          break;
        }
        else if (m_waitExitNum > 0 && m_liveNum > m_minNum)
        {
          --m_waitExitNum;
          --m_liveNum;
          m_mutex.unlock();
          pthread_exit(NULL);
        }
        else
        {
          m_mutex.unlock();
        }
      }
      else
      {
        break;
      }
    } // pick out task
    task = m_taskQueue.front();
    m_taskQueue.pop();
    m_CSemaNotFull.notify();
    m_mutex.unlock();

    {
      MutexType::Lock lock(m_mutexCnt);
      m_busyNum++;
    }

    //执行回调函数任务
		(*(task.m_function))(task.m_arg);					

    {
      MutexType::Lock lock(m_mutexCnt);
      m_busyNum--;
    }

    {
      //MutexType::Lock lock(m_mutexCnt);
      MutexType::Lock ll(m_mutex);
      //the last thread and the last task
      if (m_isShutdown && m_busyNum == 0
          && m_taskQueue.size() == 0)
      {
        m_CSemaExit.notify();
        break;
      }
    }
  } // end of while

  pthread_exit(NULL);
}

void ThreadPool::adjust_thread()
{
  size_t i = 0;
  size_t busyNum = 0;

  while (true)
  {
    sleep(DEFAULT_TIME);
    {
      MutexType::Lock lock(m_mutexCnt);
      busyNum = m_busyNum;
    }

    MutexType::Lock lock(m_mutex);

    //清除已经死去的线程
    auto it = m_workers.begin();
    while (it != m_workers.end())
    {
      if (!is_alive(*it))
      {
        //printf("thread %s is died\n", (*it)->getName().c_str());
        it = m_workers.erase(it);
      }
      else
      {
        ++it;
      }
    }

    //扩容
    if (m_taskQueue.size() >= MIN_WAIT_TASK_NUM && m_workers.size() < m_maxNum)
    {
      int add = 0;

      for (i=0; i<m_maxNum && add < DEFAULT_THREAD_VARY
              && m_workers.size() < m_maxNum; ++i)
      {
        Thread::ptr thr(new Thread([this](){worker_thread();},
                       "worker_"+std::to_string(m_workerId)));
        m_workers.push_back(thr);
        ++m_workerId;
        ++m_liveNum;
        ++add;
      }
    }

    //收缩(shrink)
    if (busyNum * 2 < m_workers.size() && m_workers.size() > m_minNum )
    {
      m_waitExitNum = DEFAULT_THREAD_VARY;

      for (i=0; i<DEFAULT_THREAD_VARY; ++i)
      {
        //通知空闲的线程结束
        m_CSemaNotEmpty.notify();
      }
    }

  }  //end of while
  pthread_exit(NULL);
}

bool ThreadPool::is_alive(Thread::ptr thr)
{
  int kill_rc = pthread_kill(thr->getThread(), 0);
	if (kill_rc == ESRCH) {
		return false;
	}
	return true;
}

bool ThreadPool::threadpool_add(void*(*function)(void *arg), void *arg)
{
  if (m_isShutdown)
  {
    return false;
  }

  Task task(function, arg);
  
  //通过信号量模拟条件变量
  while (true)
  {
    m_mutex.lock();
    if (m_taskQueue.size() >= m_maxQueSize)
    {
      m_mutex.unlock();
      m_CSemaNotFull.wait();
      m_mutex.lock();
      if(m_taskQueue.size() < m_maxQueSize)
      {
        m_taskQueue.push(task);
        break;
      }
      else
      {
        m_mutex.unlock();
      }
    }
    else
    {
      m_taskQueue.push(task);
      break;
    }
  }
  m_CSemaNotEmpty.notify();
  m_mutex.unlock();

  return true;
}
