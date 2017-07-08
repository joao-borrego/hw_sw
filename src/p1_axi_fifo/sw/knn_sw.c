/*
 * @file knn.c
 * @brief KNN classifier implementation using HW for calculating distances
 *
 * This program runs a KNN implementation with configurable parameter K.
 * The dataset must be uploaded to memory before execution.
 * Hardware Acceleration is used in the calculation of squared euclidean
 * distances between testing and training objects.
 */

#include <stdio.h>

#include "xil_printf.h"
#include "data.h"
#include "my_axis_fifo.h"
#include "xtime_l.h"

/** K-nearest neighbours parameter */
#define K 3

/** @brief Distance from test object A to trn object B, an label of B */
typedef struct DistLabelPair_Struct{
    float distance;   /**< Distance from a given test object to B */
    int label;        /**< Class label of B */
}DistLabelPair;

/** @brief Sorts an array of floats with Selection Sort for K iterations O(n*K)
 *
 * @param array Array of floats to be sorted
 * @param size The size of the array
 * @param k Number of iterations (sorted objects)
 * @return Void.
 */
void selectionSortK(DistLabelPair *array, int size, int k){
    int i, j;
    int min;
    float tmp_distance;
    int tmp_label;

    for (i = 0; i < k; i++){
        min = i;
        for (j = i + 1; j < size; j++){
            if (array[min].distance > array[j].distance){
                min = j;
            }
        }
        if (min != i){
            tmp_distance = array[i].distance; tmp_label = array[i].label;
            array[i].distance = array[min].distance; array[i].label = array[min].label;
            array[min].distance = tmp_distance; array[min].label = tmp_label;
        }
    }
}

/**
 * @brief main program
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

	/**< Distance matrix, each entry associated with the trn object label */
    DistLabelPair dist_label[NUM_TST_OBJ][NUM_TRN_OBJ];

    int correct = 0;					/**< Number of correctly classified objects */
    int vote = 0;						/**< Occurrence of a given class */
    int votes[CLASSES];					/**< Array for storing the class of each K nearest neighbour */
    int assigned_label;					/**< Label assigned to a single test object */
    int label_prediction[NUM_TST_OBJ];	/**< Final classification output */
    //float accuracy;     				/**< correctly_classified / total */

    int i,j;
	int bytes_read;

    /* HW - Fill data structures */
    float* data_trn = TRN_DATA_BASE_ADDR;
    float* data_tst = TST_DATA_BASE_ADDR;
    int* label_trn = TRN_LABEL_BASE_ADDR;
    int* label_tst = TST_LABEL_BASE_ADDR;

    /* HW - Initialize FIFO */
    my_axis_fifo_init();
    
    // DEBUG
    //xil_printf("Initialized FIFO\n");

    // DEBUG
    /*
    printf("\nTRAINING SET\n\n");
    for (i = 0; i < NUM_TRN_OBJ; i++){
        printf("trn :%.2f %.2f %.2f %.2f %d (%s)\n",
        data_trn[i*FEATURES + 0],
        data_trn[i*FEATURES + 1],
        data_trn[i*FEATURES + 2],
        data_trn[i*FEATURES + 3],
        label_trn[i],
		label_strings[label_trn[i]]);
    }

    printf("\nTESTING SET\n\n");
    for (i = 0; i < NUM_TST_OBJ; i++){
        printf("tst :%.2f %.2f %.2f %.2f %d (%s)\n",
        data_tst[i*FEATURES + 0],
        data_tst[i*FEATURES + 1],
        data_tst[i*FEATURES + 2],
        data_tst[i*FEATURES + 3],
        label_tst[i],
		label_strings[label_tst[i]]);
    }
	*/

    float terminator = 1E30;

    float distances_buffer[NUM_TRN_OBJ];

    /* Calculate distance matrix */

    XTime_GetTime(&t_kernel_start);

    /* For object in testing set */
    for (i = 0; i < NUM_TST_OBJ; i++){
    	/* HW -  Send 1 testing object to HW memory */
    	my_send_to_fifo((void *) &(data_tst[i*FEATURES]), sizeof(float) * FEATURES);

    	/* For object in training set */
    	for (j = 0; j < NUM_TRN_OBJ; j++){

    		if (j == NUM_TRN_OBJ - 1){
    			my_send_to_fifo((void*) &terminator, sizeof(float));
    			//printf("Sending Terminator %f at %d\n", terminator, j);
    		}

            /* Send 1 training object to HW memory */
        	my_send_to_fifo((void *) &(data_trn[j*FEATURES]), sizeof(float) * FEATURES);
        }

    	/* Retrieve calculated squared distance from HW */
    	bytes_read = my_receive_from_fifo((void *) distances_buffer, sizeof(float) * NUM_TRN_OBJ);

        for(j = 0; j < NUM_TRN_OBJ; j++){
        	dist_label[i][j].distance = distances_buffer[j];
        	dist_label[i][j].label = label_trn[j];
        	//printf("tst obj %d - distance to trn obj %d of class %d is %f \n", i, j, label_trn[j], dist_label[i][j].distance);
        }
    }

    XTime_GetTime(&t_kernel_end);

    /* From the distance matrix assign labels to testing objects */
    for (i = 0; i <  NUM_TST_OBJ; i++){
        
        selectionSortK((DistLabelPair*) (dist_label[i]), NUM_TRN_OBJ, K);
        for (j = 0; j < CLASSES; j++){
            votes[j] = 0;
        }
        
        for (j = 0; j < K; j++){
            votes[ dist_label[i][j].label ]++;
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

    XTime_GetTime(&t_end);

    /* Output predictions and calculate accuracy */
	for (i = 0; i < NUM_TST_OBJ; i++){
		if (label_prediction[i] == label_tst[i]){
			correct++;
		}

		xil_printf("tst object %d assigned to class %d (%s)\n", i,
				label_prediction[i], label_strings[label_prediction[i]]);

	}

    //accuracy = (correct * 100.0)/NUM_TST_OBJ;
    //printf("Total of %d correctly classified (%.2f%%)\n", correct, accuracy);
	xil_printf("Total of %d correctly classified out of %d", correct, NUM_TST_OBJ);

    xil_printf("\nTiming Report (us)\nKernel Execution: %d\nTotal Execution: %d\n%d;%d;",
       		(int) (1.0 * (t_kernel_end - t_kernel_start) / (COUNTS_PER_SECOND/1000000)),
   			(int) (1.0 * (t_end - t_start) / (COUNTS_PER_SECOND/1000000)),
   			(int) (1.0 * (t_kernel_end - t_kernel_start) / (COUNTS_PER_SECOND/1000000)),
   			(int) (1.0 * (t_end - t_start) / (COUNTS_PER_SECOND/1000000))
    );

    return 0;
}
