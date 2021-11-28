#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sched.h>
#include <sys/mman.h>
#include <string.h>
#include <sched.h>
#include <unistd.h>

//we use max value of 49 as the PRREMPT_RT use 50
//as the priority of kernel tasklets and interrupt handler by default
#define MY_PRIORITY1 (45)
#define MY_PRIORITY2 (42)
#define MY_PRIORITY3 (40)

#define MAX_SAFE_STACK (8*1024) /* The maximum stack size which is
                                   guaranteed safe to access without
                                   faulting */

#define NSEC_PER_SEC    (1000000000) /* The number of nsecs per sec. */
#define INTERVAL (50000)

int count = 0;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condvar = PTHREAD_COND_INITIALIZER;

void stack_prefault(void) {

	unsigned char dummy[MAX_SAFE_STACK];

	memset(dummy, 0, MAX_SAFE_STACK);
	return;
}

// Function that will be used by Task1

void* tfun1(void*n) {
	struct timespec t;
	struct sched_param param;
	int interval = INTERVAL;
	int num_runs = 10; // Instaed of infinite loop, use finite number of iterations
	int num;

	/* Assign priority and scheduling policy to the task */
	param.sched_priority = MY_PRIORITY1;
	if (sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
		perror("sched_setscheduler failed");
		exit(-1);
	}

	/* Lock memory */

	if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
		perror("mlockall failed");
		exit(-2);
	}

	/* Pre-fault our stack */

	stack_prefault();
  pthread_mutex_lock(&lock);
  pthread_cond_wait(&condvar, &lock);
  pthread_mutex_unlock(&lock);

	clock_gettime(CLOCK_MONOTONIC, &t);
	/* start after one second */
	t.tv_sec++;

	while (num_runs) {
		pthread_mutex_lock(&lock);
		num = count;
		printf("Count in thread 1:%d\n", num);

		num = num + 1;
		/* wait until next shot */
		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t, NULL);

		/* do the stuff */

		count = num;
		pthread_mutex_unlock(&lock);
		printf("Count in thread 1, after increment: %d\n", count);

		//printf("Hello! This is task1, Priority:%d\n", param.sched_priority);

		/* calculate next shot */
		t.tv_nsec += interval;

		while (t.tv_nsec >= NSEC_PER_SEC) {
			t.tv_nsec -= NSEC_PER_SEC;
			t.tv_sec++;
		}
		num_runs = num_runs - 1;
	}

	return NULL;
}

void* tfun2(void*n) {
	struct timespec t;
	struct sched_param param;
	int interval = INTERVAL;
	int num_runs = 10; // replaced infinite loop
	int num;

	/* Assign priority and scheduling policy to the task */
	param.sched_priority = MY_PRIORITY2;
	if (sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
		perror("sched_setscheduler failed");
		exit(-1);
	}

	/* Lock memory */

	if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
		perror("mlockall failed");
		exit(-2);
	}

	/* Pre-fault our stack */

	stack_prefault();
  pthread_mutex_lock(&lock);
  pthread_cond_wait(&condvar, &lock);
  pthread_mutex_unlock(&lock);

	clock_gettime(CLOCK_MONOTONIC, &t);
	/* start after one second */
	t.tv_sec++;

	while (num_runs) {
		pthread_mutex_lock(&lock);
		num = count;
		printf("Count in thread 2:%d\n", num);
		num = num -1;

		/* wait until next shot */
		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t, NULL);

		/* do the stuff */

		count = num;
		pthread_mutex_unlock(&lock);
		printf("Count in thread 2, after decrement:%d\n", count);

		//printf("Hello! This is task2, Priority:%d\n", param.sched_priority);

		/* calculate next shot */
		t.tv_nsec += interval;

		while (t.tv_nsec >= NSEC_PER_SEC) {
			t.tv_nsec -= NSEC_PER_SEC;
			t.tv_sec++;
		}
		num_runs = num_runs - 1;
	}

	return NULL;
}

void* tfun3(void*n) {
	struct timespec t;
	struct sched_param param;
	int interval = INTERVAL;
	int num_runs = 10; // replaced infinite loop

	/* Declare ourself as a real time task */

	param.sched_priority = MY_PRIORITY3;
	if (sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
		perror("sched_setscheduler failed");
		exit(-1);
	}

	/* Lock memory */

	if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
		perror("mlockall failed");
		exit(-2);
	}

	/* Pre-fault our stack */

	stack_prefault();
  pthread_mutex_lock(&lock);
  pthread_cond_wait(&condvar, &lock);
  pthread_mutex_unlock(&lock);

	clock_gettime(CLOCK_MONOTONIC, &t);
	/* start after one second */
	t.tv_sec++;

	while (num_runs) {
		/* wait until next shot */
		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t, NULL);

		/* do the stuff */

		printf("Hello! This is task3, Priority:%d\n", param.sched_priority);

		/* calculate next shot */
		t.tv_nsec += interval;

		while (t.tv_nsec >= NSEC_PER_SEC) {
			t.tv_nsec -= NSEC_PER_SEC;
			t.tv_sec++;
		}
		num_runs = num_runs - 1;
	}
	return NULL;
}

int main(int argc, char* argv[]) {
	pthread_t tid1, tid2, tid3;
	cpu_set_t cpus;
	// Force the program to run on one cpu,
	CPU_ZERO(&cpus); //Initialize cpus to nothing clear previous info if any
	CPU_SET(0, &cpus); // Set cpus to a cpu number zeor here

	if (sched_setaffinity(0, sizeof(cpu_set_t), &cpus) < 0)
		perror("set affinity");

  //sched_setscheduler(0, SCHED_FIFO, NULL);

	// Create three threads
	pthread_create(&tid1, NULL, tfun1, NULL);
	pthread_create(&tid2, NULL, tfun2, NULL);
	pthread_create(&tid3, NULL, tfun3, NULL);

  sleep(1);
  pthread_mutex_lock(&lock);
  pthread_cond_broadcast(&condvar);
  pthread_mutex_unlock(&lock);

	// Wait for the threads to terminate
	pthread_join(tid1, NULL);
	pthread_join(tid2, NULL);
	pthread_join(tid3, NULL);

	printf("Value of count %d\n", count);
	return 0;

}
