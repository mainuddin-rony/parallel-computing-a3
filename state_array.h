#ifndef STATE_ARRAY_H
#define STATE_ARRAY_H

// A struct holding the sum value for a "state array" element, 
// including an associated mutex and condition variable.
typedef struct{

	pthread_cond_t  cv;
	pthread_mutex_t lock;
	int sum;
} state;

/** Allocate a new state array with the specified number of rows and columns. For
 *  each element, the "condition variable" and "mutex" data members are initialized, and
 *  the sum is set to 0.
 *
 *  @param _nrows number of rows in the new array
 *  @param _ncols number of columns in the new array 
 */
void createStateArray(int _nrows, int _ncols);

/** Destroy all mutex and condition variables in each element, and then free the memory used
 *  by the array.
 */
void destroyStateArray();


/** Return the number of rows in the state array.*/
int getNumRows();

/** Return the number of columns in the state array.*/
int getNumCols();

/** Return a reference to the state array.*/
state * getStateArray();

/** Set the sum field to 1 for all border elements. Border elements are found in the last column
 *  and in the bottom row of the array.
 */
void initBorders();


/** For each element, including border elements, set the sum field to 0. 
 */
void resetStateArray();


int waitOnNeighbor(int index);

/** Given a row and column, compute the index of the corresponding element of the state_array.
 *
 *  @param r the row coordinate
 *  @param c the column coordinate    
 *  @return the element index
*/
int index(int r, int c);


/** Given the index of an element, return the index of the north neighbor.
 *  @param index the element index
 *  @return the neighbor index
 */
int N(int index);

/** Given the index of an element, return the index of the south neighbor.
 *  @param index the element index
 *  @return the neighbor index
 */
int S(int index);

/** Given the index of an element, return the index of the east neighbor.
 *  @param index the element index
 *  @return the neighbor index
 */
int E(int index);

/** Given the index of an element, return the index of the west neighbor.
 *  @param index the element index
 *  @return the neighbor index
 */
int W(int index);


#endif
