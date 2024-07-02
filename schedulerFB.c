#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "scheduler.h"
#include "job.h"
#include "linked_list.h"

// Foreground-Background (FB) aka Least Attained Service
// Section 2 - March 19th (water)
// FB scheduler info

#define DEBUG_PRINT 1

typedef struct
{
    list_t *job_queue;
    list_node_t *current_node;
    uint64_t current_node_start_time;
    uint64_t unacounted_time;
} scheduler_FB_t;

int compare_job_(void *a, void *b)
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
void *schedulerFBCreate()
{
    scheduler_FB_t *info = malloc(sizeof(scheduler_FB_t));
    if (info == NULL)
    {
        return NULL;
    }

    info->job_queue = list_create(compare_job_);
    if (info->job_queue == NULL)
    {
        free(info);
        return NULL;
    }

    info->current_node = NULL;
    info->current_node_start_time = 0;
    info->unacounted_time = 0;

#if DEBUG_PRINT
    printf("FB scheduler created\n");
#endif

    return info;
}

// Destroys scheduler specific info
void schedulerFBDestroy(void *schedulerInfo)
{
    scheduler_FB_t *info = (scheduler_FB_t *)schedulerInfo;

#if DEBUG_PRINT
    printf("Destroying FB scheduler\n");
#endif

    list_destroy(info->job_queue);
    free(info);
}

// Called to schedule a new job in the queue
void schedulerFBScheduleJob(void *schedulerInfo, scheduler_t *scheduler, job_t *job, uint64_t currentTime)
{
#if DEBUG_PRINT
    printf("\n*** Scheduling ***\n");
    printf("Current time: %lu\n", currentTime);
    printf("Job %lu arrived at %lu with duration %lu\n", jobGetId(job), jobGetArrivalTime(job), jobGetRemainingTime(job));
#endif

    scheduler_FB_t *info = (scheduler_FB_t *)schedulerInfo;

    // Check if this job has less remaining time than the current job
    // Handel the canceled job's remaining time
    if (info->current_node != NULL) // && jobGetRemainingTime(list_data(info->current_node)) > jobGetRemainingTime(job))
    {
        // Cancel the previous job and schedule the this job
        schedulerCancelNextCompletion(scheduler);

        job_t *canceled_job = list_data(info->current_node);
        uint64_t old_remaining_time = jobGetRemainingTime(canceled_job);

        // Calculates the time spent on the current job
        uint64_t normalized_time = currentTime - info->current_node_start_time + info->unacounted_time;

#if DEBUG_PRINT
        printf("Unaccounted time: %lu\n", info->unacounted_time);
#endif

        // The amount of time left over
        info->unacounted_time = normalized_time % (list_count(info->job_queue));

        // The amount of time that was spent on each job
        uint64_t time_spend_on_job_n = (normalized_time / (list_count(info->job_queue)));

#if DEBUG_PRINT
        printf("Unaccounted time: %lu\n", info->unacounted_time);
        printf("Normalized time: %lu\n", normalized_time);
        printf("List count: %lu\n", list_count(info->job_queue));
        printf("Time spend on job: %lu\n", time_spend_on_job_n);
#endif

        // Decrement the remaining time of all the jobs in the queue
        // TODO: Make this into a fucntion
        {
            list_node_t *current = list_head(info->job_queue);
            while (current != NULL)
            {
                job_t *iter_job = list_data(current);
                jobSetRemainingTime(iter_job, jobGetRemainingTime(iter_job) - time_spend_on_job_n);
                current = list_next(current);
            }
        }

#if DEBUG_PRINT
        printf("Canceled job %lu remaining time: %lu\n", jobGetId(canceled_job), jobGetRemainingTime(canceled_job));
        printf("Canceled job %lu remaining time decreased by %lu\n", jobGetId(canceled_job), old_remaining_time - jobGetRemainingTime(canceled_job));
#endif
    }

    // Insert the job according to the remaining time
    list_node_t *node = list_insert(info->job_queue, job);

    // Schedule the shortest job (head)
    info->current_node = list_head(info->job_queue); // This is the next latest job to be worked on which is also the shortest
    info->current_node_start_time = currentTime;
    //                      the time it is now + (the time it will take to complete the next shortest job * the number of jobs in the queue)
    uint64_t remaining_time = currentTime + ((jobGetRemainingTime(info->current_node->data) * list_count(info->job_queue)) - info->unacounted_time);
    if (remaining_time < currentTime)
    {
        remaining_time = currentTime + (jobGetRemainingTime(info->current_node->data) * list_count(info->job_queue));
    }

    schedulerScheduleNextCompletion(scheduler, remaining_time);

#if DEBUG_PRINT
    printf("Job %lu scheduled to complete at %lu\n", jobGetId(info->current_node->data), remaining_time);

    // Printf the queue with id and remaining time
    // TODO: Make this into a function
    {
        list_node_t *current = list_head(info->job_queue);
        while (current != NULL)
        {
            job_t *job = list_data(current);
            printf("ID %lu w/ %lu size --> ", jobGetId(job), jobGetRemainingTime(job));
            current = list_next(current);
        }
        printf("\n");
    }
#endif
}

// Called to complete a job in response to an earlier call to schedulerScheduleNextCompletion
job_t *schedulerFBCompleteJob(void *schedulerInfo, scheduler_t *scheduler, uint64_t currentTime)
{
#if DEBUG_PRINT
    printf("\n*** Completing ***\n");
    printf("Current time: %lu\n", currentTime);
#endif

    scheduler_FB_t *info = (scheduler_FB_t *)schedulerInfo;
    list_node_t *node = info->current_node;

    if (node == NULL)
    {
#if DEBUG_PRINT
        printf("No jobs in the queue\n");
#endif
        return NULL; // No jobs in the queue
    }

    job_t *job = list_data(node);

    // Remove the job from the queue

    // Calculates the time spent on the current job
    uint64_t normalized_time = currentTime - info->current_node_start_time + info->unacounted_time;

    // The amount of time left over
    info->unacounted_time = normalized_time % (list_count(info->job_queue));

    // The amount of time that was spent on each job
    uint64_t time_spend_on_job_n = (normalized_time / (list_count(info->job_queue)));

#if DEBUG_PRINT
    printf("Normalized time: %lu\n", normalized_time);
    printf("List count: %lu\n", list_count(info->job_queue));
    printf("Time spend on job: %lu\n", time_spend_on_job_n);
#endif

    // Decrement the remaining time of all the jobs in the queue
    // TODO: Make this into a fucntion
    {
        list_node_t *current = list_head(info->job_queue);
        while (current != NULL)
        {
            job_t *iter_job = list_data(current);
            jobSetRemainingTime(iter_job, jobGetRemainingTime(iter_job) - time_spend_on_job_n);
            current = list_next(current);
        }
    }

    //         jobSetRemainingTime(iter_job, jobGetRemainingTime(iter_job) - jobGetRemainingTime(job));

    // Remove the job from the queue
    list_remove(info->job_queue, node);

#if DEBUG_PRINT
    printf("Completed job %lu at time %lu\n", jobGetId(job), currentTime);
    printf("Job %lu arrived at %lu with duration %lu\n", jobGetId(job), jobGetArrivalTime(job), jobGetRemainingTime(job));
#endif

    // Schedule the next job completion if the queue is not empty
    if (list_count(info->job_queue) > 0)
    {
        node = list_head(info->job_queue); // This is the next latest job to be worked on that is the shortest
        job_t *next_job = list_data(node);
        uint64_t next_job_remaining_time = currentTime + ((jobGetRemainingTime(next_job) * list_count(info->job_queue)) - info->unacounted_time);
        schedulerScheduleNextCompletion(scheduler, next_job_remaining_time);
        info->current_node = node;
        info->current_node_start_time = currentTime;

#if DEBUG_PRINT
        printf("Next job %lu scheduled to complete at %lu\n", jobGetId(next_job), next_job_remaining_time);
        printf("Next job %lu has duration %lu\n", jobGetId(next_job), jobGetRemainingTime(next_job));
#endif
    }
    else
    {
        info->current_node = NULL;
    }

#if DEBUG_PRINT
    // Printf the queue with id and remaining time
    {
        list_node_t *current = list_head(info->job_queue);
        while (current != NULL)
        {
            job_t *job = list_data(current);
            printf("ID %lu w/ %lu size --> ", jobGetId(job), jobGetRemainingTime(job));
            current = list_next(current);
        }
        printf("\n");
    }
#endif

    return job;
}