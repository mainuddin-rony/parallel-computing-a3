//
//  barrier.c
//  a2
//
//  Created by Philip Rhodes on 2/28/14.
//  Copyright (c) 2014 Philip Rhodes. All rights reserved.
/* *
 *  Created by Philip Rhodes on 2/17/09.
 *  Modified from
 *  http://www.informit.com/content/images/0201633922/sourcceode/barrier.c
 *
 */

/*
 * barrier.c
 *
 * This file implements the "barrier" synchronization construct.
 *
 * A barrier causes threads to wait until a set of threads has
 * all "reached" the barrier. The number of threads required is
 * set when the barrier is initialized, and cannot be changed
 * except by reinitializing.
 *
 * The barrier_init() and barrier_destroy() functions,
 * respectively, allow you to initialize and destroy the
 * barrier.
 *
 * The barrier_wait() function allows a thread to wait for a
 * barrier to be completed. One thread (the one that happens to
 * arrive last) will return from barrier_wait() with the status
 * -1 on success -- others will return with 0. The special
 * status makes it easy for the calling code to cause one thread
 * to do something in a serial region before entering another
 * parallel section of code.
 *
 * An additional mechanism allows a "barrier function" to be
 * executed by the last thread only WHILE HOLDING THE BARRIER
 * MUTEX. The other threads have been signaled, but cannot run
 * until after the barrier function runs and the mutex becomes
 * available.
 *
 */
#include <pthread.h>
#include <errno.h>
#include "barrier.h"
#include <stdio.h>

/**
 * Initialize a barrier for use.
 *
 * @param barrier a pointer to a barrier variable
 * @param count the number of threads that will participate in this barrier
 * @param barrier_func a function pointer referring to a "barrier function" that will
 *  be executed by the last thread to enter the barrier.
 */
int barrier_init (barrier_t *barrier, int count, void * (* barrier_func)(void *) )
{
    int status;
    printf("Inside barrier init. Count is %d\n", count);
    barrier->threshold = barrier->counter = count;
    barrier->cycle = 0;
    barrier->barrier_func=barrier_func;

    status = pthread_mutex_init (&barrier->mutex, NULL);
    if (status != 0)
        return status;
    status = pthread_cond_init (&barrier->cv, NULL);
    if (status != 0) {
        pthread_mutex_destroy (&barrier->mutex);
        return status;
    }
    barrier->valid = BARRIER_VALID;
    return 0;
}

/*
 * Destroy a barrier when done using it.
 * @param barrier a pointer to a barrier variable
 */
int barrier_destroy (barrier_t *barrier)
{
    int status, status2;

    if (barrier->valid != BARRIER_VALID)
        return EINVAL;

    status = pthread_mutex_lock (&barrier->mutex);
    if (status != 0)
        return status;

    /*
     * Check whether any threads are known to be waiting; report
     * "BUSY" if so.
     */
    if (barrier->counter != barrier->threshold) {
        pthread_mutex_unlock (&barrier->mutex);
        return EBUSY;
    }

    barrier->valid = 0;
    status = pthread_mutex_unlock (&barrier->mutex);
    if (status != 0)
        return status;

    /*
     * If unable to destroy either 1003.1c synchronization
     * object, return the error status.
     */
    status = pthread_mutex_destroy (&barrier->mutex);
    status2 = pthread_cond_destroy (&barrier->cv);
    return (status == 0 ? status : status2);
}

/**
 * Wait for all members of a barrier to reach the barrier. When
 * the count (of remaining members) reaches 0, execute the
 * barrier function (if any), and broadcast to wake
 * all threads waiting.
 *
 * @param barrier a barrier variable
 * @param barrier_func_args a pointer that will be passed to the barrier
 *  function, if there is one.
 */
int barrier_wait (barrier_t *barrier, void * barrier_func_args)
{
    int status, cancel, cycle;
    printf("%s\n", "Into barrier wait");


    if (barrier->valid != BARRIER_VALID)
        return EINVAL;

    status = pthread_mutex_lock (&barrier->mutex);
    if (status != 0)
        return status;

    cycle = barrier->cycle;   /* Remember which cycle we're on */
    printf("Barrier cycle is %d\n", cycle);
    printf("Barrier counter is %d\n", barrier->counter);
    // PJR: note the pre-decrement operator below (--)
    // We'll decrement counter BEFORE evaluating the equality test.
    if (--barrier->counter == 0) {
        barrier->cycle = !barrier->cycle;
        barrier->counter = barrier->threshold;
        printf("Barrier threshold is %d\n", barrier->threshold);
        status = pthread_cond_broadcast (&barrier->cv);

        // Execute the barrier function, if there is one.
        if(barrier->barrier_func != NULL){

            barrier->barrier_func(barrier_func_args);
        }

        /*
         * The last thread into the barrier will return status
         * -1 rather than 0, so that it can be used to perform
         * some special serial code following the barrier.
         */
        if (status == 0)
            status = -1;
    } else {
        /*
         * Wait with cancellation disabled, because barrier_wait
         * should not be a cancellation point.
         */
        printf("Barrier Wait Else");
        pthread_setcancelstate (PTHREAD_CANCEL_DISABLE, &cancel);

        /*
         * Wait until the barrier's cycle changes, which means
         * that it has been broadcast, and we don't want to wait
         * anymore.
         */
        while (cycle == barrier->cycle) {
            status = pthread_cond_wait ( &barrier->cv, &barrier->mutex);
            if (status != 0) break;
        }

        pthread_setcancelstate (cancel, NULL);
    }
    /*
     * Ignore an error in unlocking. It shouldn't happen, and
     * reporting it here would be misleading -- the barrier wait
     * completed, after all, whereas returning, for example,
     * EINVAL would imply the wait had failed. The next attempt
     * to use the barrier *will* return an error, or hang, due
     * to whatever happened to the mutex.
     */
    pthread_mutex_unlock (&barrier->mutex);
    return status;          /* error, -1 for waker, or 0 */
}
