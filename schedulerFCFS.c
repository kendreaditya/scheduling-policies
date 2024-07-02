#include <stdint.h>
#include <stdlib.h>
#include "scheduler.h"
#include "job.h"
#include "linked_list.h"
#include <stdio.h>

#define DEBUG_PRINT 0

typedef struct
{
    list_t *job_queue;
} scheduler_FCFS_t;

// Creates and returns scheduler specific info
void *schedulerFCFSCreate()
{
    scheduler_FCFS_t *info = malloc(sizeof(scheduler_FCFS_t));
    if (info == NULL)
    {
        return NULL;
    }

    info->job_queue = list_create(NULL);
    if (info->job_queue == NULL)
    {
        free(info);
        return NULL;
    }

#if DEBUG_PRINT
    printf("FCFS scheduler created\n");
#endif

    return info;
}

// Destroys scheduler specific info
void schedulerFCFSDestroy(void *schedulerInfo)
{
    scheduler_FCFS_t *info = (scheduler_FCFS_t *)schedulerInfo;

#if DEBUG_PRINT
    printf("Destroying FCFS scheduler\n");
#endif

    list_destroy(info->job_queue);
    free(info);
}

// Called to schedule a new job in the queue
void schedulerFCFSScheduleJob(void *schedulerInfo, scheduler_t *scheduler, job_t *job, uint64_t currentTime)
{
#if DEBUG_PRINT
    printf("\n*** Scheduling ***\n");
#endif

    scheduler_FCFS_t *info = (scheduler_FCFS_t *)schedulerInfo;

    // Insert the job at the head of the queue
    list_insert(info->job_queue, job);

#if DEBUG_PRINT
    printf("Current time: %lu\n", currentTime);
    printf("Job %lu arrived at %lu with duration %lu\n", job->id, job->arrivalTime, job->remainingTime);
#endif

    // If this is the only job in the queue, schedule its completion
    if (list_count(info->job_queue) == 1)
    {
        schedulerScheduleNextCompletion(scheduler, currentTime + jobGetJobTime(job));
#if DEBUG_PRINT
        printf("Job %lu scheduled to complete at %lu\n", job->id, currentTime + jobGetJobTime(job));
#endif
    }
}

// Called to complete a job in response to an earlier call to schedulerScheduleNextCompletion
job_t *schedulerFCFSCompleteJob(void *schedulerInfo, scheduler_t *scheduler, uint64_t currentTime)
{
#if DEBUG_PRINT
    printf("\n*** Completing ***\n");
#endif

    scheduler_FCFS_t *info = (scheduler_FCFS_t *)schedulerInfo;

    // Get the tail of the queue
    list_node_t *node = list_tail(info->job_queue);
    if (node == NULL)
    {
#if DEBUG_PRINT
        printf("No jobs in the queue\n");
#endif
        return NULL; // No jobs in the queue
    }

    job_t *job = list_data(node);

    // Remove the job from the queue
    list_remove(info->job_queue, node);

#if DEBUG_PRINT
    printf("Completed job %lu at time %lu\n", job->id, currentTime);
    printf("Job %lu arrived at %lu with duration %lu\n", job->id, job->arrivalTime, job->remainingTime);
#endif

    // Schedule the next job completion if the queue is not empty
    if (list_count(info->job_queue) > 0)
    {
        node = list_tail(info->job_queue);
        job_t *next_job = list_data(node);
        schedulerScheduleNextCompletion(scheduler, currentTime + jobGetRemainingTime(next_job));

#if DEBUG_PRINT
        printf("Next job %lu scheduled to complete at %lu\n", next_job->id, currentTime + jobGetRemainingTime(next_job));
#endif
    }

    return job;
}