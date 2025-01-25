#include "types.h"
#include "stat.h"
#include "user.h"
#include "kthread.h"
#define TIME_SLICE 10 //in ms
#define MAX_STACK_SIZE 1024
#define ASSERT(assumption, errMsg) assert(assumption, errMsg, __LINE__)

//TODO join before exit
int stdout = 1;
int pid;
int thread_ids[3] = {-1,-1,-1};

void
assert(_Bool assumption, char* errMsg, int curLine)
{
	if(!assumption)
	{
		printf(stdout, "at %s:%d, ", __FILE__, curLine);
		printf(stdout, "%s\n", errMsg);
		printf(stdout, "test failed\n");
		kill(pid);
	}
}

void*
first(void)
{
	printf(stdout, "%s\n", "thread 1 says hello");
  sleep(2 * TIME_SLICE); //sleep for some scheduling rounds instead of immediately exiting so that others can join
	kthread_exit();

  ASSERT(0, "thread 1 continues to execute after exit");
  	return 0;
}

void*
second(void)
{
	while(thread_ids[0]<0); //wait for the first thread to be created
	ASSERT(kthread_join(thread_ids[0]) >= 0, "failed to join thread 1");

	printf(stdout, "%s\n", "thread 2 says hello");
  sleep(2 * TIME_SLICE); //sleep for some scheduling rounds instead of immediately exiting so that others can join
	kthread_exit();

  ASSERT(0, "thread 2 continues to execute after exit");
  	return 0;
}

void*
third(void)
{
	while(thread_ids[0]<0 && thread_ids[1]<0); //wait for the first two threads to be created
	ASSERT(kthread_join(thread_ids[0]) >= 0, "failed to join thread 1");
	ASSERT(kthread_join(thread_ids[1]) >= 0, "failed to join thread 2");

	printf(stdout, "%s\n", "thread 3 says hello");
  sleep(2 * TIME_SLICE); //sleep for some scheduling rounds instead of immediately exiting so that others can join
	kthread_exit();

  ASSERT(0, "thread 3 continues to execute after exit");
  	return 0;
}

int
main(int argc, char *argv[])
{
  printf(stdout, "~~~~~~~~~~~~~~~~~~ thread test ~~~~~~~~~~~~~~~~~~\n");
  pid = getpid();

  char stack1[MAX_STACK_SIZE];
  char stack2[MAX_STACK_SIZE];
  char stack3[MAX_STACK_SIZE];
  
  thread_ids[2] = kthread_create(third, stack3, MAX_STACK_SIZE);
  ASSERT(thread_ids[2] >= 0, "failed to create thread 3");
  thread_ids[1] = kthread_create(second, stack2, MAX_STACK_SIZE);
  ASSERT(thread_ids[1] >= 0, "failed to create thread 2");
  thread_ids[0] = kthread_create(first, stack1, MAX_STACK_SIZE);
  ASSERT(thread_ids[0] >= 0, "failed to create thread 1");

  ASSERT(kthread_join(thread_ids[0]) >= 0, "failed to join thread 1");
  ASSERT(kthread_join(thread_ids[1]) >= 0, "failed to join thread 2");
  ASSERT(kthread_join(thread_ids[2]) >= 0, "failed to join thread 3");
  printf(stdout, "%s\n", "all threads exited");
  //attempt to join myself
  ASSERT(kthread_join(kthread_id()) < 0, "joining calling thread returns success");
  //attempt to join an invalid thread id
  ASSERT(kthread_join(-10) < 0, "joining invalid thread returns success");
  //attempt to join another process(init)'s thread
  ASSERT(kthread_join(1) < 0, "joining another process's thread returns success");
  
  kthread_exit();

  ASSERT(0, "main thread continues to execute after exit");
}