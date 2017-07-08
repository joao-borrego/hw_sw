/*
 * @file knn_seq.c
 * @brief KNN classifier SW implementation
 *
 * This program runs a KNN implementation with configurable parameter K.
 * The dataset must be uploaded to memory before execution.
 */

/************************************************************************/

#include <stdio.h>

#include "xil_printf.h"
#include "xtime_l.h"
#include "xtime_l.h"
#include "xil_mmu.h"
#include "xil_cache.h"
#include "xil_cache_l.h"

#include "data_seq.h"

/************************************************************************/

/* Macros Definition */

/** K-nearest neighbours parameter */
#define K 3

/************************************************************************/

/* Function Prototypes */

void selectionSortK(float *distances, int *smallest, int size, int k);
float euclideanDistance(float* a, float* b, int size);

/************************************************************************/

/**
 * @brief Main Program
 * @param argc Argument count
 * @param argv Argument values
 * @return 0 on success.
 */
int main(int argc, char** argv){

	/* Timing variables */

	/* Total execution */
	XTime t_start, t_end;
	/* Kernel execution (Distance calculation) */
	XTime t_kernel_start, t_kernel_end;

	XTime_GetTime(&t_start);

	/** Output distance matrix */
	float *distances = OUT_DIST_BASE_ADDR;

	int i,j;

	/* HW - Fill data structures */
	float *data_trn = TRN_DATA_BASE_ADDR;
	float *data_tst = TST_DATA_BASE_ADDR;
	int *label_trn = TRN_LABEL_BASE_ADDR;
	int *label_tst = TST_LABEL_BASE_ADDR;

	/************************************************************************/

    /* Calculate distance matrix */

    XTime_GetTime(&t_kernel_start);

    /* For object in testing set */
    for (i = 0; i < NUM_TST_OBJ; i++){
    	/* For object in training set */
    	for (j = 0; j < NUM_TRN_OBJ; j++){
    		distances[i*NUM_TRN_OBJ + j] = euclideanDistance(&(data_tst[i*FEATURES]), &(data_trn[j*FEATURES]), FEATURES);
        }
    }

    XTime_GetTime(&t_kernel_end);

    /************************************************************************/

    /* Classification */

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
	/** Number of correctly classified objects */
	int correct = 0;

    /* From the distance matrix assign labels to testing objects */
	for (i = 0; i < NUM_TST_OBJ ; i++){
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

		XTime_GetTime(&t_end);

		/* Output predictions and calculate accuracy */
		for (i = 0; i < NUM_TST_OBJ; i++){
			if (label_prediction[i] == label_tst[i]){
				correct++;
			}
			// TODO - DEBUG
			/*
			xil_printf("tst object %d assigned to class %d (%s)\n", i,
				label_prediction[i], label_strings[label_prediction[i]]);
			*/
		}

		xil_printf("CPU0: Total of %d correctly classified out of %d", correct, NUM_TST_OBJ);
		xil_printf("\nCPU0: Timing Report (us)\n\tKernel Execution: %d\n\tTotal Execution: %d\n\t%d;%d;",
				(int) (1.0 * (t_kernel_end - t_kernel_start) / (COUNTS_PER_SECOND/1000000)),
				(int) (1.0 * (t_end - t_start) / (COUNTS_PER_SECOND/1000000)),
				(int) (1.0 * (t_kernel_end - t_kernel_start) / (COUNTS_PER_SECOND/1000000)),
				(int) (1.0 * (t_end - t_start) / (COUNTS_PER_SECOND/1000000))
		);

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
