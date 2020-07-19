#pragma once

#include <memory>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <queue>
#include <vector>

#include "Thread.h"

struct Task
{
	Task(void*(*function)(void *arg), void *arg);
	Task();

	void *(*m_function) (void *);
	void *m_arg;
};

class ThreadPool
{
public:
  typedef std::shared_ptr<ThreadPool> ptr;
	typedef SpinLock MutexType;

	ThreadPool(size_t minNum = 3, size_t maxNum = 10, size_t queueSize = 10);
  ~ThreadPool();

	//向任务队列中添加任务
  bool threadpool_add(void*(*function)(void *arg), void *arg);

private:
	//工作线程执行函数
  void worker_thread();

	//管理线程执行函数
  void adjust_thread();
	
	//判断线程是否已经退出
	bool is_alive(Thread::ptr thr);

private:
	MutexType m_mutex;						//用于锁该结构体本身
	MutexType m_mutexCnt;					//用于读取计数
	
	Semaphore  m_CSemaNotFull;		//任务队列未满信号量
	Semaphore  m_CSemaNotEmpty;		//任务队列非空信号量
	Semaphore  m_CSemaExit;				//任务处理完毕且退出信号量

	std::vector<Thread::ptr> m_workers;		//工作线程
	Thread::ptr m_adjustThr;						//管理者线程
  std::queue<Task> m_taskQueue;				//任务队列

	size_t m_minNum = 0;					//工作线程最小容量
	size_t m_maxNum = 0;					//工作线程最大容量
	size_t m_busyNum = 0;					//当前正在忙的线程
	size_t m_waitExitNum = 0;			//需要推出的线程数量
	size_t m_workerId = 0;				//记录该工作线程是第几个
	size_t m_maxQueSize = 0;			//任务队列的最大值
	size_t m_liveNum = 0;					//存活的工作线程的数量

	bool m_isShutdown = false;		//是否退出
};

