/*
 * @file knn_dma_cpu1.c
 * @brief KNN classifier HW/SW implementation - CPU1
 *
 * This program runs a KNN implementation with configurable parameter K.
 * The dataset must be uploaded to memory before execution.
 * Processes a part of the distance matrix calculations and a part
 * of the classification, not necessarily corresponding to the same
 * testing elements
 */

/************************************************************************/

#include <stdio.h>
#include "xil_printf.h"
#include "xtime_l.h"

#include "xtime_l.h"
#include "xil_mmu.h"
#include "xil_cache.h"
#include "xil_cache_l.h"

#include "data_cpu1.h"

/************************************************************************/

/* Macros Definition */

/** K-nearest neighbours parameter */
#define K 3

/************************************************************************/

/* Function prototypes */

/* Euclidean distance calulation */
float euclideanDistance(float* a, float* b, int size);

/* K-Selection sort */
void selectionSortK(float *distances, int *smallest, int size, int k);

/************************************************************************/

/* Global Variables */

/** Global Sync Semaphore */
volatile int *sync = SEM_ADDR;

/************************************************************************/

/**
 * @brief main program
 * @param argc Argument count
 * @param argv Argument values
 * @return 0 on success.
 */
int main(int argc, char** argv){


	/* Timing variables */

	/* Total execution */
	XTime t_start;
	/* Kernel execution (Distance calculation) */
	XTime t_kernel_start, t_kernel_end;

	/* Disable cache on OCM region */
	Xil_SetTlbAttributes(0xFFFF0000, 0x14de2);

	while (*sync != START_0){};
	*sync = START_1;

	XTime_GetTime(&t_start);

	/** Output distance matrix */
	float *distances = OUT_DIST_BASE_ADDR;

	int i,j;

	/* HW - Fill data structures */
	float *data_trn = TRN_DATA_BASE_ADDR;
	float *data_tst = TST_DATA_BASE_ADDR;
	int *label_trn = TRN_LABEL_BASE_ADDR;

    /* Calculate distance matrix */

    XTime_GetTime(&t_kernel_start);

    /* For object in testing set */
    for (i = FIRST_CPU1; i <= LAST_CPU1; i++){
    	/* For object in training set */
    	for (j = 0; j < NUM_TRN_OBJ; j++){
    		distances[i*NUM_TRN_OBJ + j] = euclideanDistance(&(data_tst[i*FEATURES]), &(data_trn[j*FEATURES]), FEATURES);
        }
    }

    XTime_GetTime(&t_kernel_end);

    /************************************************************************/

    Xil_DCacheFlushRange((INTPTR) (distances),
        		(unsigned)(sizeof(float)) * NUM_TST_OBJ * NUM_TRN_OBJ);

    /* Synchronisation */

    /* Notify CPU0 that distance calculations are done */
    while(*sync != END_DIST_0){}
    *sync = END_DIST_1;

    /************************************************************************/

    /* Classification */
    Xil_DCacheInvalidateRange((INTPTR) (distances),
    		(unsigned)(sizeof(float)) * NUM_TST_OBJ * NUM_TRN_OBJ);

	/** Occurrence of a given class */
	int vote = 0;
	/** Array for storing the class of each K nearest neighbour */
	int votes[CLASSES];
	/** Label assigned to a single test object */
	int assigned_label;
	/** Final classification output */
	int *label_prediction = OUT_LABELS_BASE_ADDR;
	/** Temporary trn labels array used for classification */
	int closest[K];

    /* From the distance matrix assign labels to testing objects */
	for (i = FIRST_CPU1_CLASS; i <= LAST_CPU1_CLASS; i++){
		selectionSortK((float *)(&distances[i*NUM_TRN_OBJ]), closest, NUM_TRN_OBJ, K);
		for (j = 0; j < CLASSES; j++){
			votes[j] = 0;
		}

		for (j = 0; j < K; j++){
			votes[ label_trn[closest[j]]] ++;
		}

		assigned_label = 0;
		vote = 0;

		for (j = 0; j < CLASSES; j++){
			if (votes[j] > vote){
				vote = votes[j];
				assigned_label = j;
			}
		}
		label_prediction[i] = assigned_label;
	}

    /************************************************************************/

    /* Synchronisation */

	 Xil_DCacheFlushRange((INTPTR) (label_prediction),
			 (unsigned)(sizeof(int) * NUM_TST_OBJ));

    /* Notify CPU0 that distance calculations are done */
    while(*sync != END_CLASS_0){}
    *sync = END_CLASS_1;

    /************************************************************************/

    return 0;
}

/************************************************************************/

/** @brief Calculates squared euclidean distance between 2 feature vectors
 *
 *	@param a First feature vector
 *	@param b Second feature vector
 *	@param size Feature dimensionality
 *	@return The desired distance.
 */
float euclideanDistance(float* a, float* b, int size){

    int i;
    float diff;
    float sum = 0.0;
    for (i = 0; i < size; i++){
        diff = (a[i] - b[i]);
        sum += diff * diff;
    }
    return sum;
}

/**
 * @brief Sorts an array of floats with Selection Sort for K iterations O(n*K)
 *
 * @param array Array of floats to be sorted
 * @param closest Initial index of the K smallest values
 * @param size The size of the array
 * @param k Number of iterations (sorted objects)
 * @return Void.
 */
void selectionSortK(float *array, int *closest, int size, int k){
    int i, j;
    int min;
    float tmp_distance;

    for (i = 0; i < k; i++){
        min = i;
        for (j = i + 1; j < size; j++){
            if (array[min] > array[j]){
                min = j;
            }
        }
        if (min != i){
            tmp_distance = array[i];
            array[i] = array[min];
            array[min] = tmp_distance;
        }
        closest[i] = min;
    }
}

