#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>

#define TASK_NUM       2  //# of tasks
#define SIMU_PERIOD    20 //simulation period in seconds
#define MAX_EVENTS     20 //Max # of events considered
#define INTERVAL       (50000)

//#define FIFO
//#define RM
#define EDF

struct timespec tInit;
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER; // mutex

struct taskInfo
{
  float phase;
  float period;
  float exec_time;
  float deadline;
};

struct Jobs
{
  float     arrival_time;
  int       taskID;
  float     exec_time;
  float     deadline;
  int       priority;
  pthread_t threadID;
};

float time_diff(struct timespec end, struct timespec start)
{
  struct timespec result;

  if ((end.tv_nsec - start.tv_nsec) < 0) {
    result.tv_sec  = end.tv_sec - start.tv_sec - 1;
    result.tv_nsec = end.tv_nsec - start.tv_nsec + 1000000000;
  }
  else {
    result.tv_sec  = end.tv_sec - start.tv_sec;
    result.tv_nsec = end.tv_nsec - start.tv_nsec;
  }
  float t = (float)result.tv_nsec / 1000000000;

  return result.tv_sec + t;
}

void * taskfunc(void *arg)
{
  struct Jobs *   jobInfo = arg;
  struct timespec tCurrent;
  float           t;
  int             t1, t2;

  jobInfo->threadID = pthread_self();
  for (t1 = 0; t1 < 1000 * (jobInfo->exec_time - 0.001); t1++) {
    pthread_mutex_lock(&mut);
    for (t2 = 0; t2 < 80000; t2++) //run for 0.0001 second
      ;
#ifdef EDF
    struct sched_param param;
    if (t1 % 1000 == 0) {
      clock_gettime(CLOCK_MONOTONIC, &tCurrent);
      t = time_diff(tCurrent, tInit);
      //Your dynamic EDF priority settings code goes here

      param.sched_priority = (int)(100 - (jobInfo->deadline - t));


      if (pthread_setschedparam(pthread_self(), SCHED_FIFO, &param) == -1) {
        perror("sched_setscheduler failed\n");
        exit(-1);
      }
      else {
        printf("[%f sec]: Task %d Priority: %d\n", t, jobInfo->taskID, param.sched_priority);
      }
    }
#endif
    usleep(1);
    pthread_mutex_unlock(&mut);
  }

  clock_gettime(CLOCK_MONOTONIC, &tCurrent);
  t = time_diff(tCurrent, tInit);
  if (t <= jobInfo->deadline)
    printf("[%f sec]: Task %d is completed\n", t, jobInfo->taskID);
  else
    printf("[%f sec]: Task %d missed deadline\n", t, jobInfo->taskID);

  return NULL;
}

int comp(const void *a, const void *b)
{
  struct Jobs *job1 = (struct Jobs *)a;
  struct Jobs *job2 = (struct Jobs *)b;

  return job1->arrival_time - job2->arrival_time;
}

int main(int argc, char *argv[])
{
  //Test how many loops that costs 1 seconds (0.99s is the best)

  /*
   * long t = 0;
   * struct timespec  tStart, tEnd;
   * clock_gettime(CLOCK_MONOTONIC, &tStart);
   * for (t=0;t<82400000;t++);
   * clock_gettime(CLOCK_MONOTONIC, &tEnd);
   * printf("time: %f\n", time_diff(tEnd, tStart));
   */

  struct taskInfo tasks[TASK_NUM];
  struct Jobs     sortedJobs[MAX_EVENTS]; //sorted jobs by arrival times
  int             numJobs[TASK_NUM];

  int i, j;

  //Get Task Info from standard input
  for (i = 0; i < TASK_NUM; i++) {
    printf("Enter 4-tuple task parameters for Task %d:\n", i + 1);
    scanf("%f %f %f %f", &tasks[i].phase, &tasks[i].period,
          &tasks[i].exec_time, &tasks[i].deadline);
  }


  //Create Jobs
  int numEvents = 0;

  for (i = 0; i < TASK_NUM; i++) {
    numJobs[i] = (SIMU_PERIOD - tasks[i].phase) / tasks[i].period;
    for (j = 0; j < numJobs[i]; j++) {
      sortedJobs[numEvents].arrival_time = tasks[i].phase
                                           + j * tasks[i].period;
      sortedJobs[numEvents].taskID    = i;
      sortedJobs[numEvents].exec_time = tasks[i].exec_time;
      sortedJobs[numEvents].deadline  = sortedJobs[numEvents].arrival_time
                                        + tasks[i].deadline;
#ifdef FIFO
      sortedJobs[numEvents].priority = 1;
#endif
#ifdef RM
      //Your RM priority settings code goes here
      sortedJobs[numEvents].priority = 99 - tasks[i].period;
#endif
#ifdef EDF
      //Your initial EDF priority settings code goes here
      sortedJobs[numEvents].priority = 99 - sortedJobs[numEvents].deadline;
#endif
      numEvents++;
    }
  }

  //Sort Jobs by arriving time
  qsort(sortedJobs, numEvents, sizeof(sortedJobs[0]), comp);

  //Print Jobs info
  printf("Events:\n");
  for (i = 0; i < numEvents; i++)
    printf(
      "Events %d: Task %d --- arr: %.4f, period: %.4f, e: %.4f, d: %.4f, priority: %d\n",
      i, sortedJobs[i].taskID, sortedJobs[i].arrival_time,
      tasks[sortedJobs[i].taskID].period, sortedJobs[i].exec_time,
      sortedJobs[i].deadline, sortedJobs[i].priority);

  //Force the program to run on one CPU
  cpu_set_t cpus;
  CPU_ZERO(&cpus);   //Initialize CPUs to nothing clear previous info if any
  CPU_SET(0, &cpus); // Set CPUs to a CPU number zero here

  //Simulation
  printf("\nRunning Jobs:\n");
  pthread_t          tid[MAX_EVENTS]; //real size should be numEvents, but C compile doesn't allow dynamic size of array.
  struct timespec    tCurrent;
  pthread_attr_t     attr;
  struct sched_param param;

  /*
   * printf("Scheduled Priority interval: [%d %d]\n",
   * sched_get_priority_min(SCHED_FIFO),
   * sched_get_priority_max(SCHED_FIFO));
   */

  clock_gettime(CLOCK_MONOTONIC, &tInit);
  for (i = 0; i < numEvents; i++) {
    clock_gettime(CLOCK_MONOTONIC, &tCurrent);
    for (; time_diff(tCurrent, tInit) < sortedJobs[i].arrival_time;)
      clock_gettime(CLOCK_MONOTONIC, &tCurrent);

    //initialized with default attributes
    if (pthread_attr_init(&attr)) {
      perror("pthread_attr_init failed!");
      exit(-1);
    }

    //set policy
    if (pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED)) {
      perror("pthread_attr_setinheritsched failed!");
      exit(-1);
    }
    if (pthread_attr_setschedpolicy(&attr, SCHED_FIFO)) {
      perror("pthread_attr_setschedpolicy failed!");
      exit(-1);
    }

    //set the priority
    param.sched_priority = sortedJobs[i].priority;

    //setting the new scheduling param
    if (pthread_attr_setschedparam(&attr, &param)) {
      perror("pthread_attr_setschedparam failed!");
      exit(-1);
    }

    //get existing scheduling param
    if (pthread_attr_getschedparam(&attr, &param)) {
      perror("pthread_attr_getschedparam failed!");
      exit(-1);
    }

    printf("[%f sec]: Create Job of Task %d with priority %d\n",
           time_diff(tCurrent, tInit), sortedJobs[i].taskID,
           param.sched_priority);
    pthread_create(&tid[i], &attr, taskfunc, &sortedJobs[i]);
  }

  for (i = 0; i < numEvents; i++) {
    pthread_join(tid[i], NULL);

    //there is delay after threads join the main thread.

    /*      clock_gettime(CLOCK_MONOTONIC,&tCurrent);
     * taskCompleteTime = time_diff(tCurrent, tInit);
     * if( taskCompleteTime < sortedJobs[i].deadline)
     * printf("Task %d has been completed before deadline at %f!\n", sortedJobs[i].taskID, taskCompleteTime);
     * else
     * printf("Task %d missed deadline at %f!\n", sortedJobs[i].taskID, taskCompleteTime);    */
  }

  return 0;
}
