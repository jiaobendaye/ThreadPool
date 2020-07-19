#include <iostream>
#include <unistd.h>

#include "src/ThreadPool.h"
#include "src/Thread.h"

void *process(void *arg) {
	printf("thread %s working on task%d\n ", Thread::GetName().c_str(), *((int*) arg));
	sleep(1);
	printf("task %d is end\n", *((int*) arg));

	return NULL;
}

int main()
{
  ThreadPool::ptr pool(new ThreadPool(3, 30, 20));

	int num[40], i;
	for (i = 0; i < 20; ++i) {
		num[i] = i;
		printf("add task %d\n", i);
		pool->threadpool_add(process, (void*)&num[i]);
	}

	sleep(10);
	for (; i < 40; ++i) {
		num[i] = i;
		printf("add task %d\n", i);
		pool->threadpool_add(process, (void*)&num[i]);
	}

  return 0;
}
