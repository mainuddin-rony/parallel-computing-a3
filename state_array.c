#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "state_array.h"

/** This C code contains several functions that work with a "state array" that is declared
 *  statically in this file. That is, there is only one state array, and the functions work
 *  on that single instance. Of course, you'll want to call createStateArray() before doing
 *  much else.
 *
 *  Each element of the state array is of type "state", as found in state_arr.h.  Although
 *  it is useful to think of the array as two dimensional, C really only supports 1-D arrays.
 *  However, we can map (row, column) coordinates to a 1D "index" very easily when necessary.
 *  But if we want to "visit" every element (like for initialization), it's simpler to stick
 *  with the 1D index.
 *
 *  Elements in the last column and in the bottom row are considered border elements, and
 *  are treated specially by functions like initBorders(). The diagram below shows the position
 *  of the border elements for a 6 x 6 array, the cardinal directions (N,S,E,W), as well as element 0
 *  in the upper left.
 *
 *          N
 *
 *      0 * * * * B
 *      * * * * * B
 *      * * * * * B
 * W    * * * * * B     E
 *      * * * * * B
 *      B B B B B B
 *
 *          S
 */




// These variables have "static" scope, meaning they are only visible
// inside this file. Currently, only one state array can be created.
state * state_arr=NULL;  // The state array.
int nrows, ncols;        // The number of rows and columns in the state array.
int arr_len = 0;         // The total number of elements in the state array.




/** Allocate a new state array with the specified number of rows and columns. For
 *  each element, the "condition variable" and "mutex" data members are initialized, and
 *  the sum is set to 0.
 *
 *  @param _nrows number of rows in the new array
 *  @param _ncols number of columns in the new array
 */
void createStateArray(int _nrows, int _ncols){

    nrows = _nrows;
    ncols = _ncols;

    arr_len = nrows * ncols;
    printf("Initializing State Array. Arr len is %d\n", arr_len);

    state_arr = malloc(arr_len * sizeof(state));

    for(int i=0; i < arr_len; i++){

        pthread_cond_init(&(state_arr[i].cv), NULL);
        pthread_mutex_init(&(state_arr[i].lock), NULL);

        state_arr[i].sum = 0;
    }
}

/** Return a reference to the state array.*/
state * getStateArray(){

    return state_arr;
}

/** Return the number of rows in the state array.*/
int getNumRows(){

    return nrows;
}

/** Return the number of columns in the state array.*/
int getNumCols(){

    return ncols;
}



/** Destroy all mutex and condition variables in each element, and then free the memory used
 *  by the array.
 */
void destroyStateArray(){

    for(int i=0; i < arr_len; i++){

        pthread_cond_destroy(&(state_arr[i].cv));
        pthread_mutex_destroy(&(state_arr[i].lock));
    }

    free(state_arr);
    state_arr = NULL;
}


/** For each element, including border elements, set the sum field to 0.
 */
void resetStateArray(){

    for(int i=0; i < arr_len; i++){

        pthread_mutex_lock(&(state_arr[i].lock));   // probably not required, because the barrier
                                                    // prevents parallel execution,
        state_arr[i].sum = 0;
        pthread_mutex_unlock(&(state_arr[i].lock)); // but using the mutex makes this code more
                                                    // portable
    }

}

/** For each border element, set the sum field to 1 and signal on the elements condition
 *  variable. Border elements are found in the last column and in the bottom row of the
 *  array.
 */
void initBorders(){

    //TODO: Write me.

    // It's important to use the lock and cv fields here. You're not just
    // setting the sum values of the border elements, you are triggering the
    // wavefront.  Use pthread_cond_broadcast() rather than pthread_cond_signal().
    //  ( is there a case where it could make a difference? )
    printf("%s\n", "Initiating Borders");
    for (int i = 0; i <  nrows; i ++){
      int b_idx = index(i, ncols-1);
      printf("Initializing border idx %d\n", b_idx);
      pthread_mutex_lock(&state_arr[b_idx].lock);
      state_arr[b_idx].sum = 1;
      pthread_cond_broadcast(&state_arr[b_idx].cv);
      pthread_mutex_unlock(&state_arr[b_idx].lock);
    }

    for (int i = 0; i <  ncols-1; i ++){
      int b_idx = index(nrows-1, i);
      printf("Initializing border idx %d\n", b_idx);
      pthread_mutex_lock(&state_arr[b_idx].lock);
      state_arr[b_idx].sum = 1;
      pthread_cond_broadcast(&state_arr[b_idx].cv);
      pthread_mutex_unlock(&state_arr[b_idx].lock);
    }

}


void signalBorderCVs()
{
  for (int i=0; i <  nrows; i ++){
    int b_idx = index(i, ncols-1);
    pthread_mutex_lock(&state_arr[b_idx].lock);
    pthread_cond_broadcast(&state_arr[b_idx].cv);
    pthread_mutex_unlock(&state_arr[b_idx].lock);
  }

  for (int i=0; i <  ncols; i ++){
    int b_idx = index(nrows-1, i);
    pthread_mutex_lock(&state_arr[b_idx].lock);
    pthread_cond_broadcast(&state_arr[b_idx].cv);
    pthread_mutex_unlock(&state_arr[b_idx].lock);
  }

}

/** Given a row and column, compute the index of the corresponding element of the state_array.
 *
 *  @param r the row coordinate
 *  @param c the column coordinate
 *  @return the element index
*/
int index(int r, int c){

    return  r * ncols + c;
}

/** Given the index of an element, return the index of the north neighbor.
 *  @param index the element index
 *  @return the neighbor index
 */
int N(int index){

	return index - ncols;
}

/** Given the index of an element, return the index of the south neighbor.
 *  @param index the element index
 *  @return the neighbor index
 */
int S(int index){

	return index + ncols;
}

/** Given the index of an element, return the index of the east neighbor.
 *  @param index the element index
 *  @return the neighbor index
 */
int E(int index){

	return index + 1;
}

/** Given the index of an element, return the index of the west neighbor.
 *  @param index the element index
 *  @return the neighbor index
 */
int W(int index){

	return index - 1;
}

/** Given the index of an element, wait on that element's condition variable until
 *  the element's sum field is non-zero, and then return the sum value.
 *
 *  @param index the element index
 *  @return the element's sum value.
 */
int waitOnNeighbor(int index){

    //TODO: Write me.
    //
    // putting this code in a function that is called by doWork() cleans
    // up the doWork() code, but if you'd rather write this code directly
    // in the thread function, that's okay too.
    //
    // In any case, remember to use a while loop with cond_wait(), so
    // you don't have problems with missed signals, etc.

	return 42;
}
