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
    list_node_t *next_node;

} scheduler_SJF_t;

int compare_job_time(void *a, void *b)
{
    job_t *job_a = (job_t *)a;
    job_t *job_b = (job_t *)b;

    if (jobGetRemainingTime(job_a) < jobGetRemainingTime(job_b))
    {
        return -1;
    }
    else if (jobGetRemainingTime(job_a) > jobGetRemainingTime(job_b))
    {
        return 1;
    }
    else
    {
        // Tie breaker
        if (jobGetId(job_a) < jobGetId(job_b))
        {
            return -1;
        }
        else if (jobGetId(job_a) > jobGetId(job_b))
        {
            return 1;
        }

        return 0;
    }
}

// Creates and returns scheduler specific info
void *schedulerSJFCreate()
{
    scheduler_SJF_t *info = malloc(sizeof(scheduler_SJF_t));
    if (info == NULL)
    {
        return NULL;
    }

    info->job_queue = list_create(compare_job_time);
    if (info->job_queue == NULL)
    {
        free(info);
        return NULL;
    }

#if DEBUG_PRINT
    printf("SJF scheduler created\n");
#endif

    return info;
}

// Destroys scheduler specific info
void schedulerSJFDestroy(void *schedulerInfo)
{
    scheduler_SJF_t *info = (scheduler_SJF_t *)schedulerInfo;

#if DEBUG_PRINT
    printf("Destroying SJF scheduler\n");
#endif

    list_destroy(info->job_queue);
    free(info);
}

// Called to schedule a new job in the queue
void schedulerSJFScheduleJob(void *schedulerInfo, scheduler_t *scheduler, job_t *job, uint64_t currentTime)
{
#if DEBUG_PRINT
    printf("\n*** Scheduling ***\n");
#endif

    scheduler_SJF_t *info = (scheduler_SJF_t *)schedulerInfo;

    // Insert the job at the head of the queue
    list_insert(info->job_queue, job);

#if DEBUG_PRINT
    printf("Current time: %lu\n", currentTime);
    printf("Job %lu arrived at %lu with duration %lu\n", jobGetId(job), jobGetArrivalTime(job), jobGetRemainingTime(job));
#endif

    // If this is the only job in the queue, schedule its completion
    if (list_count(info->job_queue) == 1)
    {
        info->next_node = list_head(info->job_queue);
        schedulerScheduleNextCompletion(scheduler, currentTime + jobGetJobTime(job));
#if DEBUG_PRINT
        printf("Job %lu scheduled to complete at %lu\n", jobGetId(job), currentTime + jobGetJobTime(job));
#endif
    }
}

// Called to complete a job in response to an earlier call to schedulerScheduleNextCompletion
job_t *schedulerSJFCompleteJob(void *schedulerInfo, scheduler_t *scheduler, uint64_t currentTime)
{
#if DEBUG_PRINT
    printf("\n*** Completing ***\n");
#endif

    scheduler_SJF_t *info = (scheduler_SJF_t *)schedulerInfo;
    list_node_t *node = info->next_node;

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
        node = list_head(info->job_queue);
        job_t *next_job = list_data(node);
        info->next_node = node;
        schedulerScheduleNextCompletion(scheduler, currentTime + jobGetRemainingTime(next_job));

#if DEBUG_PRINT
        printf("Next job %lu scheduled to complete at %lu\n", jobGetId(next_job), currentTime + jobGetRemainingTime(next_job));
#endif
    }

    return job;
}