#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "scheduler.h"
#include "job.h"
#include "linked_list.h"

#define DEBUG_PRINT 0

// Preemptive Shortest Job First (PSJF) 
// PSJF scheduler info
typedef struct {
    list_t* job_queue;
    list_node_t* current_node;
    uint64_t current_node_start_time;
    // uint64_t current_node_size;
} scheduler_PSJF_t;

int compare_job_size(void *a, void *b)
{
    job_t *job_a = (job_t *)a;
    job_t *job_b = (job_t *)b;

    if (jobGetJobTime(job_a) < jobGetJobTime(job_b))
    {
        return -1;
    }
    else if (jobGetJobTime(job_a) > jobGetJobTime(job_b))
    {
        return 1;
    }
    else
    {
        // Tie breaker
        return jobGetId(job_a) < jobGetId(job_b) ? -1 : 1;
    }
}

// Creates and returns scheduler specific info
void* schedulerPSJFCreate()
{
    scheduler_PSJF_t* info = malloc(sizeof(scheduler_PSJF_t));
    if (info == NULL) {
        return NULL;
    }

    info->job_queue = list_create(compare_job_size);
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
void schedulerPSJFDestroy(void* schedulerInfo)
{
    scheduler_PSJF_t* info = (scheduler_PSJF_t*)schedulerInfo;
#if DEBUG_PRINT
    printf("Destroying PLCFS scheduler\n");
#endif

    list_destroy(info->job_queue);
    free(info);
}

// Called to schedule a new job in the queue
// schedulerInfo - scheduler specific info from create function
// scheduler - used to call schedulerScheduleNextCompletion and schedulerCancelNextCompletion
// job - new job being added to the queue
// currentTime - the current simulated time
void schedulerPSJFScheduleJob(void* schedulerInfo, scheduler_t* scheduler, job_t* job, uint64_t currentTime)
{

#if DEBUG_PRINT
    printf("\n*** Scheduling ***\n");
#endif

    scheduler_PSJF_t* info = (scheduler_PSJF_t*)schedulerInfo;

    // Insert the job at the head of the queue
    list_node_t* node = list_insert(info->job_queue, job);

#if DEBUG_PRINT
    printf("Current time: %lu\n", currentTime);
    printf("Job %lu arrived at %lu with duration %lu\n", jobGetId(job), jobGetArrivalTime(job), jobGetRemainingTime(job));

    if (info->current_node != NULL)
        printf("Comparing Job %lu with duration %lu\n", jobGetId((job_t*) info->current_node), jobGetRemainingTime(info->current_node->data)); 
    
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

    // Cancel the previous job and schedule the this job if this job is the shortest
    // if (info->current_node == NULL || jobGetRemainingTime(job) < jobGetRemainingTime(info->current_node->data))
    if (info->current_node == NULL || jobGetJobTime(job) < jobGetJobTime(info->current_node->data))
    {
        schedulerCancelNextCompletion(scheduler);

        // Handel the canceled job's remaining time
        if (info->current_node != NULL) {
            job_t *canceled_job = list_data(info->current_node);
            uint64_t old_remaining_time = jobGetRemainingTime(canceled_job);
            jobSetRemainingTime(canceled_job, jobGetRemainingTime(canceled_job) - (currentTime - info->current_node_start_time));

        }

        // Schedule the next completion
        schedulerScheduleNextCompletion(scheduler, currentTime + jobGetRemainingTime(job));

        info->current_node = node;
        info->current_node_start_time = currentTime;

#if DEBUG_PRINT
        printf("Job %lu scheduled to complete at %lu\n", jobGetId(job), currentTime + jobGetRemainingTime(job));
#endif
    }

    // If the current job is the shortest, do nothing
    else {

#if DEBUG_PRINT
        printf("Job %lu is not the shortest, not scheduling\n", jobGetId(job));
#endif

    }
}

// Called to complete a job in response to an earlier call to schedulerScheduleNextCompletion
// schedulerInfo - scheduler specific info from create function
// scheduler - used to call schedulerScheduleNextCompletion and schedulerCancelNextCompletion
// currentTime - the current simulated time
// Returns the job that is being completed
job_t* schedulerPSJFCompleteJob(void* schedulerInfo, scheduler_t* scheduler, uint64_t currentTime)
{

#if DEBUG_PRINT
    printf("\n*** Completing ***\n");
    printf("Current time: %lu\n", currentTime);
#endif

    scheduler_PSJF_t* info = (scheduler_PSJF_t*)schedulerInfo;

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