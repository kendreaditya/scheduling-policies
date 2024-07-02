#include <stdint.h>
#include <stdlib.h>
#include "scheduler.h"
#include "job.h"
#include "linked_list.h"
#include <stdio.h>

#define DEBUG_PRINT 1

typedef struct
{
    list_t *job_queue;
    list_node_t *current_node;
    uint64_t current_node_start_time;
} scheduler_PLCFS_t;

// Creates and returns scheduler specific info
void *schedulerPLCFSCreate()
{
    scheduler_PLCFS_t *info = malloc(sizeof(scheduler_PLCFS_t));
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

    info->current_node = NULL;

#if DEBUG_PRINT
    printf("PLCFS scheduler created\n");
#endif

    return info;
}

// Destroys scheduler specific info
void schedulerPLCFSDestroy(void *schedulerInfo)
{
    scheduler_PLCFS_t *info = (scheduler_PLCFS_t *)schedulerInfo;

#if DEBUG_PRINT
    printf("Destroying PLCFS scheduler\n");
#endif

    list_destroy(info->job_queue);
    free(info);
}

// Called to schedule a new job in the queue
void schedulerPLCFSScheduleJob(void *schedulerInfo, scheduler_t *scheduler, job_t *job, uint64_t currentTime)
{
#if DEBUG_PRINT
    printf("\n*** Scheduling ***\n");
#endif

    scheduler_PLCFS_t *info = (scheduler_PLCFS_t *)schedulerInfo;

    // Insert the job at the head of the queue
    list_node_t* node = list_insert(info->job_queue, job);

#if DEBUG_PRINT
    printf("Current time: %lu\n", currentTime);
    printf("Job %lu arrived at %lu with duration %lu\n", jobGetId(job), jobGetArrivalTime(job), jobGetRemainingTime(job));
#endif

    // Cancel the previous job and schedule the this job
    schedulerCancelNextCompletion(scheduler);

    // Handel the canceled job's remaining time
    if (info->current_node != NULL)
    {
        job_t *canceled_job = list_data(info->current_node);
        uint64_t old_remaining_time = jobGetRemainingTime(canceled_job);
        jobSetRemainingTime(canceled_job, jobGetRemainingTime(canceled_job) - (currentTime - info->current_node_start_time));

#if DEBUG_PRINT
        printf("Canceled job %lu remaining time: %lu\n", jobGetId(canceled_job), jobGetRemainingTime(canceled_job));
        printf("Canceled job %lu remaining time decreased by %lu\n", jobGetId(canceled_job), old_remaining_time - jobGetRemainingTime(canceled_job));
#endif

    }

    // Schedule the this job
    schedulerScheduleNextCompletion(scheduler, currentTime + jobGetRemainingTime(job));
    info->current_node = node;
    info->current_node_start_time = currentTime;

#if DEBUG_PRINT
        printf("Job %lu scheduled to complete at %lu\n", jobGetId(job), currentTime + jobGetRemainingTime(job));
#endif
}

// Called to complete a job in response to an earlier call to schedulerScheduleNextCompletion
job_t *schedulerPLCFSCompleteJob(void *schedulerInfo, scheduler_t *scheduler, uint64_t currentTime)
{
#if DEBUG_PRINT
    printf("\n*** Completing ***\n");
    printf("Current time: %lu\n", currentTime);
#endif

    scheduler_PLCFS_t *info = (scheduler_PLCFS_t *)schedulerInfo;
    list_node_t *node = info->current_node;
    info->current_node_start_time = currentTime;

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
    printf("Completed job %lu at time %lu\n", jobGetId(job), currentTime);
    printf("Job %lu arrived at %lu with duration %lu\n", jobGetId(job), jobGetArrivalTime(job), jobGetRemainingTime(job));
#endif

    // Schedule the next job completion if the queue is not empty
    if (list_count(info->job_queue) > 0)
    {
        node = list_head(info->job_queue); // This is the next latest job to be worked on
        job_t *next_job = list_data(node);
        schedulerScheduleNextCompletion(scheduler, currentTime + jobGetRemainingTime(next_job));
        info->current_node = node;
        info->current_node_start_time = currentTime;

#if DEBUG_PRINT
        printf("Next job %lu scheduled to complete at %lu\n", jobGetId(next_job), currentTime + jobGetRemainingTime(next_job));
        printf("Next job %lu has duration %lu\n", jobGetId(next_job), jobGetRemainingTime(next_job));
#endif
    }
    else
    {
        info->current_node = NULL;
    }

    return job;
}