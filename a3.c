#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>

#include "barrier.h"
#include "state_array.h"

// We'll use a "global" (really "static") variable for the result. This makes it easy to
// access from the barrier function.
int gResult = 0;



// A struct for passing arguments to a thread function.
typedef struct{

    //int tid; // thread id -- useful for debugging
    int s_index; // this thread's "home" location in the state array

    int numRounds; // how many times to repeat the wave
	  barrier_t *barrier;
} thread_function_args;



// Function prototypes.
int wavefront(int num_thread_rows, int num_thread_cols, int numRounds);
void *doWork(void *a);
void * barrier_function(void * a);

/** Performs a very lightweight wavefront computation using threads, mutexes, and condition
 *  variables.
 *
 *  Usage: ./a3 nrows ncols reps
 *      where nrows and ncols are the dimensions of the array, and
 *          nreps is the number of repetitions (or rounds). Each
 *          round is independent (and a duplicate) of the other rounds.
 *
 */
int main( int argc, char *argv[]){

    if(argc != 4){

      printf("Usage: ./a3 nrows ncols reps\n");
      return EXIT_FAILURE;
    }

	int nrows = atoi(argv[1]);
	int ncols = atoi(argv[2]);
	int reps  = atoi(argv[3]);

	wavefront(nrows, ncols, reps);
}



/** Run the specified number of independent rounds of a wavefront computation conducted
 *  over a 2D array. Each element of the array has a sum field, and this field will be
 *  computed as the sum of three neighbors: sum = east_sum + south_sum + southeast_sum.
 *
 *  Elements on the east and south edges of the array have their sum fields initialized
 *  to 1 before the computation begins. We'll call these elements "border elements".
 *
 *  Because each sum depends on three neighboring values, the computation must be performed
 *  as a "wave" that travels across the array as elements find their neighbors to have valid
 *  sums.
 *
 *  The wavefront begins in (nearly) the south-east corner, since this is the first element
 *  with valid east_sum, south_sum, and southeast_sum values due to the border elements.
 *
 *  The wavefront ends in the north-west corner (0,0), and the sum value for this element is
 *  considered to be the result. The result is printed to stdout, but not returned.
 *
 *
 *  @param num_state_rows number of rows in the state array
 *  @param num_state_cols number of columns in the state array
 *  @param numRounds number of rounds to repeat
 *  @return 0 on success, or an error code
 */

int wavefront(int num_state_rows, int num_state_cols, int numRounds){

    // The thread array will be smaller than the state_array, because
    // of the border elements. The border elements won't have a
    // corresponding thread to compute their sum.

    int num_thread_rows = (num_state_rows - 1);
    int num_thread_cols = (num_state_cols - 1);

    int thread_arr_len = num_thread_rows * num_thread_cols;
    // printf("Thread Array len is %d\n", thread_arr_len);

    createStateArray(num_state_rows, num_state_cols);

    pthread_t * thread_arr = malloc(thread_arr_len * sizeof(pthread_t));

    barrier_t barrier;



    // Why is the barrier initialized for thread_arr_len + 1 threads?
    if(barrier_init(&barrier, thread_arr_len + 1, barrier_function) != 0){
        return EXIT_FAILURE;
    }


    //TODO: Write me.
    // As usual, we need a loop that creates the threads. No load balancing is
    // involved, but there's another complication. The thread_arr and state_array
    // are different sizes. Each thread needs to know its position in the
    // state_array, but pthread_create() needs to index the thread_arr.
    //
    // The index() function in state_array.c will help map (row, column) coordinates
    // to an index in the state_array, but don't use that index with thread_arr.
    //
    // There's many ways to solve this issue.
    //
    // With that said, the contents of the loop are otherwise straightforward:
    //    malloc a new thread_function_args and set its fields,
    //    then create your thread



    // After the threads have been launched, the loop below prints out
    // the result after each round. The same value should be printed each
    // time.
    int thrd_count = 0;
    for(int i=0; i<num_state_rows; i++){
      for(int j=0; j<num_state_cols; j++){
        if((i!=num_thread_rows) && (j!=num_thread_cols)){
          int idx = index(i,j);
          // printf("Current Idx is %d\n", idx);
          thread_function_args * args = malloc(sizeof(thread_function_args));
          args->s_index = idx;
          args->numRounds = numRounds;
          args->barrier = &barrier;
          // printf("Current Thread Idx is %d\n", thrd_count);
          pthread_create(&(thread_arr[thrd_count]), NULL, doWork, args);
          thrd_count++;
        }
    }
  }

    initBorders();

    for(int round=0; round < numRounds; round++){

	    barrier_wait(&barrier, NULL);
      // printf("%s\n", "Came into loop");
		  printf("Round %d, result is %d\n", round, gResult);

    }



    for (int i=0; i<thread_arr_len; i++){

        if (pthread_join(thread_arr[i], NULL) != 0){

            return EXIT_FAILURE;
        }
    }

    destroyStateArray();

    if (barrier_destroy(&barrier) != 0){

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}



/** The thread function. After unpacking the arguments, this function performs a specified
 *  number of rounds of a small piece of a wavefront computation. Using the condition variables
 *  in the state array, the thread first waits for a signal from each of its east, south, and
 *  southeast neighbors. When this thread has received a signal from these three neighbors, it
 *  can compute the sum value for its own element in the state array. Next, it signals all
 *  threads waiting on its own element, and then synchronizes with the other threads at a
 *  barrier. After the barrier, the process is repeated until the specified number of rounds
 *  have been executed.
*/
void *doWork(void *a){
    // printf("%s\n", "Do work a aschi");
    thread_function_args * args = (thread_function_args * ) a;
    int idx = args->s_index;
    int nRounds = args->numRounds;
    barrier_t *barr = args->barrier;

    free(args);
    args = NULL; a=NULL;



    int e_idx = E(idx);
    int s_idx = S(idx);
    int es_idx = s_idx + 1;
    // printf("Main Index: %d, East Index: %d, South Index: %d, South-East Index: %d\n", idx, e_idx, s_idx, es_idx);

    //TODO: Write me.
    // Take a look at the functions available through state_array.c.
    //
    // Consider implementing waitOnNeighbor() in state_array.c for cleaner
    // looking code, but it's fine if you'd prefer all your mutex/CV code in one
    // place, at least to begin with.
    //
    // Also, use pthread_cond_broadcast() rather than pthread_cond_signal(). You
    // want to wake _all_ sleeping threads, not just one.

    for(int round = 0; round<nRounds; round++){

      int s_sum, e_sum, es_sum = 0;

      pthread_mutex_lock(&getStateArray()[e_idx].lock);
      if (getStateArray()[e_idx].sum == 0){
        pthread_cond_wait(&getStateArray()[e_idx].cv, &getStateArray()[e_idx].lock);
      }

      e_sum = getStateArray()[e_idx].sum;

      pthread_mutex_unlock(&getStateArray()[e_idx].lock);

      pthread_mutex_lock(&getStateArray()[s_idx].lock);
      if (getStateArray()[s_idx].sum == 0){
        pthread_cond_wait(&getStateArray()[s_idx].cv, &getStateArray()[s_idx].lock);
      }

      s_sum = getStateArray()[s_idx].sum;

      pthread_mutex_unlock(&getStateArray()[s_idx].lock);

      pthread_mutex_lock(&getStateArray()[es_idx].lock);
      if (getStateArray()[es_idx].sum == 0){
        pthread_cond_wait(&getStateArray()[es_idx].cv, &getStateArray()[es_idx].lock);
      }

      es_sum = getStateArray()[es_idx].sum;

      pthread_mutex_unlock(&getStateArray()[es_idx].lock);


      pthread_mutex_lock(&getStateArray()[idx].lock);
      // printf("S sum: %d E sum: %d ES sum: %d\n", s_sum, e_sum, es_sum );

      if ((e_sum != 0) && (s_sum != 0) && (es_sum != 0)){
        getStateArray()[idx].sum = e_sum +
                s_sum +
                es_sum ;
        pthread_cond_broadcast(&getStateArray()[idx].cv);
      }

      pthread_mutex_unlock(&getStateArray()[idx].lock);


     barrier_wait(barr, NULL);
   }


    return NULL;
}


/** This function is executed by the last thread to enter the barrier. It is executed under
 *  the protection of the barrier mutex, which guarantees that it runs before the other
 *  threads have started running.
 *
 *  The function does three things: It sets gResult to the sum value of element 0 of the
 *  state array. It resets all sum values in the whole state array to 0. It sets the sums
 *  of the border elements to 1.
 */
void * barrier_function(void * a){
  // printf("%s\n", "Barrier call hocche");
	gResult = getStateArray()[0].sum;

 	  resetStateArray();
    initBorders();

    return NULL;
}
