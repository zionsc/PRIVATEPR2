#include "types.h"
#include "stat.h"
#include "user.h"
#include "kthread.h"
#define TIME_SLICE 10 //in ms
#define MAX_STACK_SIZE 1024
#define ASSERT(assumption, errMsg) assert(assumption, errMsg, __LINE__)

int stdout = 1;
int pid;

int mutex;
int thread_ids[5];
volatile int shared;

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
allocate_and_lock(void)
{
	mutex = kthread_mutex_alloc();
	ASSERT(mutex >= 0, "failed to allocate mutex");
	ASSERT(kthread_mutex_lock(mutex) >= 0, "failed to lock mutex");
	kthread_exit();
	ASSERT(0, "thread continues to execute after exit");
	  return 0;
}

void*
increment(void)
{
	ASSERT(kthread_mutex_lock(mutex) >= 0, "failed to lock mutex");
	shared++;
	ASSERT(kthread_mutex_unlock(mutex) >= 0, "failed to unlock mutex");

	kthread_exit();
	ASSERT(0, "thread continues to execute after exit");
	  return 0;
}

void*
plus_three(void)
{
	//waits for times_five
	ASSERT(kthread_mutex_lock(mutex) >= 0, "failed to lock mutex");
	shared += 3;
	ASSERT(kthread_mutex_unlock(mutex) >= 0, "failed to unlock mutex"); 

	kthread_exit();
	ASSERT(0, "thread continues to execute after exit");
	  return 0;
}

void*
times_five(void)
{
	shared *= 5;
	//unblocks plus_three when done
	ASSERT(kthread_mutex_unlock(mutex) >= 0, "failed to unlock mutex"); 

	kthread_exit();
	ASSERT(0, "thread continues to execute after exit");
	  return 0;
}

int
main(int argc, char *argv[])
{
	printf(stdout, "~~~~~~~~~~~~~~~~~~ mutex test 2 ~~~~~~~~~~~~~~~~~~\n");

	int thread;
	pid = getpid();
	int expected;

	//attempt to lock a nonexistent mutex
	ASSERT(kthread_mutex_lock(-10) < 0, "locking an invalid mutex returns success");
	//attempt to unlock a nonexistent mutex
	ASSERT(kthread_mutex_unlock(-10) < 0, "unlocking an invalid mutex returns success");
	//allocate and deallocate
	for(int i=0; i<3; i++)
	{
		int dummy = kthread_mutex_alloc();
		ASSERT(dummy >= 0, "failed to allocate mutex");
		ASSERT(kthread_mutex_dealloc(dummy) >= 0, "failed to deallocate mutex");
	}


	mutex = kthread_mutex_alloc();
	ASSERT(mutex >= 0, "failed to allocate mutex");
	//attempt to unlock before mutex is locked
	ASSERT(kthread_mutex_unlock(mutex) < 0, "unlocking unlocked mutex returns success");
	ASSERT(kthread_mutex_lock(mutex) >= 0, "failed to lock mutex");
	//attempt to deallocate when mutex is locked
	ASSERT(kthread_mutex_dealloc(mutex) < 0, "deallocating a locked mutex returns success");
	ASSERT(kthread_mutex_unlock(mutex) >= 0, "failed to unlock mutex");
	ASSERT(kthread_mutex_dealloc(mutex) >= 0, "failed to deallocate mutex");

	thread = kthread_create(allocate_and_lock, malloc(MAX_STACK_SIZE), MAX_STACK_SIZE);
  	ASSERT(thread >= 0, "failed to create thread");
  	ASSERT(kthread_join(thread) >= 0, "failed to join thread");
	//attempt to unlock a mutex that's locked by another thread
	ASSERT(kthread_mutex_unlock(mutex) >= 0, "failed to unlock mutex");

	//shared variable test #1
	ASSERT(kthread_mutex_lock(mutex) >= 0, "failed to lock mutex");
	shared = 0;

	for(int i=0; i<5; i++)
	{
	  	thread = kthread_create(increment, malloc(MAX_STACK_SIZE), MAX_STACK_SIZE);
  		ASSERT(thread >= 0, "failed to create thread");
  		thread_ids[i] = thread;
	}

	sleep(6 * TIME_SLICE); //sleep with lock held for a while
	ASSERT(shared == 0, "mutex failed to prevent writing to shared");
	ASSERT(kthread_mutex_unlock(mutex) >= 0, "failed to unlock mutex");

	for(int i=0; i<5; i++)
	{
		ASSERT(kthread_join(thread_ids[i]) >= 0, "failed to join thread");
	}

	expected = 5;
	if(shared != expected)
	{
		printf(stdout, "value=%d, expected=%d\n", shared, expected);
	}
	ASSERT(shared == expected, "shared variable does not have a correct value");


	//shared variable test #2
	//loop 5 times
	//for each iteration, correct order: *5 then +3
	for(int i=0; i<5; i++)
	{
		//make sure plus_three is blocked
		ASSERT(kthread_mutex_lock(mutex) >= 0, "failed to lock mutex");

		thread = kthread_create(plus_three, malloc(MAX_STACK_SIZE), MAX_STACK_SIZE);
		ASSERT(thread >= 0, "failed to create thread");
		thread_ids[0] = thread;

		thread = kthread_create(times_five, malloc(MAX_STACK_SIZE), MAX_STACK_SIZE);
		ASSERT(thread >= 0, "failed to create thread");
		thread_ids[1] = thread;

		ASSERT(kthread_join(thread_ids[0]) >= 0, "failed to join thread");
		ASSERT(kthread_join(thread_ids[1]) >= 0, "failed to join thread");
	}

	expected = 17968;
	if(shared != expected)
	{
		printf(stdout, "value=%d, expected=%d\n", shared, expected);
	}
	ASSERT(shared == expected, "shared variable does not have a correct value");

	ASSERT(kthread_mutex_dealloc(mutex) >= 0, "failed to deallocate mutex");
	printf(stdout, "%s\n", "test passed");
	exit();
}