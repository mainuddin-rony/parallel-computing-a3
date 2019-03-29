//
//  barrier.h
//  a1b
//
//  Created by Philip Rhodes on 2/28/14.
//  Copyright (c) 2014 Philip Rhodes. All rights reserved.
// Modified from
// http://www.informit.com/content/images/0201633922/sourcecode/barrier.h
//

#ifndef BARRIER_H
#define BARRIER_H


/*
 * barrier.h
 *
 * This header file describes the "barrier" synchronization
 * construct. The type barrier_t describes the full state of the
 * barrier including the POSIX 1003.1c synchronization objects
 * necessary.
 *
 * A barrier causes threads to wait until a set of threads has
 * all "reached" the barrier. The number of threads required is
 * set when the barrier is initialized, and cannot be changed
 * except by reinitializing.
 */
#include <pthread.h>


/*
 * Structure describing a barrier.
 */
typedef struct barrier_tag {
    pthread_mutex_t     mutex;          /* Control access to barrier */
    pthread_cond_t      cv;             /* wait for barrier */
    int                 valid;          /* set when valid */
    int                 threshold;      /* number of threads required */
    int                 counter;        /* current number of threads */
    int                 cycle;          /* alternate wait cycles (0 or 1) */
    void * (* barrier_func)(void *);    // A function executed by the waking thread
                                        // under the protection of the barrier mutex.
} barrier_t;

#define BARRIER_VALID   0xdbcafe



/**
 * Initialize a barrier for use.
 *
 * @param barrier a pointer to a barrier variable
 * @param count the number of threads that will participate in this barrier
 * @param barrier_func a function pointer referring to a "barrier function" that will
 *  be executed by the last thread to enter the barrier.
 */
int barrier_init(barrier_t *barrier, int count, void * (* barrier_func)(void *) );


/*
 * Destroy a barrier when done using it.
 * @param barrier a pointer to a barrier variable
 */
int barrier_destroy(barrier_t *barrier);

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
int barrier_wait(barrier_t *barrier, void * barrier_func_args);


#endif

