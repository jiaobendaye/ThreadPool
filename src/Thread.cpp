#include <iostream>
#include <unistd.h>

#include "Thread.h"

pid_t GetThreadId() {
    return syscall(SYS_gettid);
}

static thread_local Thread* t_thread = nullptr;
static thread_local std::string t_thread_name = "NUKNOW";

Semaphore::Semaphore(uint32_t count) {
  if(sem_init(&m_sem, 0, count)) {
    throw std::logic_error("sem_init error");
  }
}

Semaphore::~Semaphore() {
  sem_destroy(&m_sem);
}

void Semaphore::wait() {
  if(sem_wait(&m_sem)) {
    throw std::logic_error("sem_wait error");
  }
}

void Semaphore::notify() {
  if(sem_post(&m_sem)) {
    throw std::logic_error("sem_post error");
  }
}

Thread* Thread::GetThis() {
  return t_thread;
}

const std::string& Thread::GetName() {
  return t_thread_name;
}

void Thread::SetName(const std::string& name) {
  if(t_thread) {
   t_thread->m_name = name; 
  }
  t_thread_name = name;
}

Thread::Thread(std::function<void()> cb, const std::string& name) 
  : m_cb(cb)
  , m_name(name) {
  if(name.empty()) {
    m_name = "UNKNOW";
  }
  int rt = pthread_create(&m_thread, nullptr, &Thread::run, this);
  if(rt) {
    std::cout << "pthread_creat thread fail, rt=" << rt 
                << " name=" << m_name;
    throw std::logic_error("pthread_creat error");
  }
  m_semaphore.wait();
}

Thread::~Thread() {
  if(m_thread) {
    printf("thread:%s exit\n", getName().c_str());
    pthread_cancel(m_thread);
    //pthread_detach(m_thread);
  }
}

void Thread::join(){
  if(m_thread) {
    int rt = pthread_join(m_thread, nullptr);
    if(rt) {
      std::cout << "pthread_join thread fail, rt=" << rt 
                  << " name=" << m_name;
      throw std::logic_error("pthread_join error");
    }
    m_thread = 0;
  }
}

void* Thread::run(void* arg) {
  Thread* thread = (Thread*)arg;
  t_thread = thread;
  t_thread_name = thread->m_name;
  thread->m_id = GetThreadId();
  pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());

  //防止函数中存在智能指针时，计数加1。swap可以少一次引用
  std::function<void()> cb;
  cb.swap(thread->m_cb);

  thread->m_semaphore.notify();

  cb();
  return 0;
}
