/*
 * @file knn_1_dma.c
 * @brief KNN classifier implementation using HW for calculating distances
 *
 * This program runs a KNN implementation with configurable parameter K.
 * The dataset must be uploaded to memory before execution.
 * Hardware Acceleration is used in the calculation of squared euclidean
 * distances between testing and training objects.
 * Direct Memory Access (DMA) provides fast access to data in the hardware.
 * A single DMA + IP Combo is used
 */

/************************************************************************/

#include <stdio.h>

#include "xil_printf.h"
#include "xaxidma.h"
#include "xparameters.h"
#include "xtime_l.h"
#include "xil_mmu.h"
#include "xil_cache.h"
#include "xil_cache_l.h"

#include "data_1_dma.h"

/************************************************************************/

/* Macros Definition */

/** K-nearest neighbours parameter */
#define K 3

/* Device hardware build related constants. */
#define DMA_0_DEV_ID XPAR_AXIDMA_0_DEVICE_ID

/************************************************************************/

/* Function prototypes */

/* AXI_DMA Functions */
int XAxiDma_Simple_KNN(u16 device_0_ID);
int init_XAxiDma_SimplePollMode(u16 device_0_ID);

/* K-Selection sort */
void selectionSortK(float *distances, int *smallest, int size, int k);

/************************************************************************/

/* Global Variables */

/** Global AXI DMA instance */
XAxiDma AxiDma0;

/* Timing variables */

/* Total execution */
XTime t_start, t_end;
/* Kernel execution (Distance calculation) */
XTime t_kernel_start, t_kernel_end;

/************************************************************************/

/**
 * @brief main program
 * @return XST_SUCCESS on success.
 */
int main(){

	int Status;

	xil_printf("Started\n");

	/* Start Timer */
	XTime_GetTime(&t_start);

	/* Initialise DMA in poll mode for simple transfer */
	Status = init_XAxiDma_SimplePollMode(DMA_0_DEV_ID);
	if (Status != XST_SUCCESS) {
		xil_printf("DMA0: init_XAxiDma_SimplePollMode: Failed\r\n");
		return XST_FAILURE;
	}

	// TODO - DEBUG
	//xil_printf("Configured\n");

	Status = XAxiDma_Simple_KNN(DMA_0_DEV_ID);
	if (Status != XST_SUCCESS) {
		xil_printf("XAxiDma_Simple_KNN: Failed\r\n");
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

/************************************************************************/

/**
 * @brief Initialises the DMA
 * @param device_0_ID First DMA ID
 * @return XST_SUCCESS on success, XST_FAILURE otherwise
 */
int init_XAxiDma_SimplePollMode(u16 device_0_ID){

  XAxiDma_Config *CfgPtr;
  int Status;

  /* Initialize the XAxiDma devices. */
  CfgPtr = XAxiDma_LookupConfig(device_0_ID);
  if (!CfgPtr) {
    xil_printf("DMA0: No config found for %d\r\n", device_0_ID);
    return XST_FAILURE;
  }

  Status = XAxiDma_CfgInitialize(&AxiDma0, CfgPtr);
  if (Status != XST_SUCCESS) {
    xil_printf("DMA0: Initialization failed %d\r\n", Status);
    return XST_FAILURE;
  }

  if(XAxiDma_HasSg(&AxiDma0)){
    xil_printf("DMA0: Device configured as SG mode \r\n");
    return XST_FAILURE;
  }

  /* Disable interrupts, polling mode is used instead */
  XAxiDma_IntrDisable(&AxiDma0, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DEVICE_TO_DMA);
  XAxiDma_IntrDisable(&AxiDma0, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DMA_TO_DEVICE);

  return XST_SUCCESS;
}

/**
 * @brief Main KNN Function
 * @param device_0_ID First DMA ID
 * @return XST_SUCCESS on success, XST_FAILURE otherwise
 */
int XAxiDma_Simple_KNN(u16 device_0_ID){

	/* Classification */

	/** Number of correctly classified objects */
	int correct = 0;
	/** Occurrence of a given class */
	int vote = 0;
	/** Array for storing the class of each K nearest neighbour */
	int votes[CLASSES];
	/** Label assigned to a single test object */
	int assigned_label;

	/* Iterative variables */
	int i,j;

	/* HW - Fill data structures */
	float *data_trn = TRN_DATA_BASE_ADDR;
	float *data_tst = TST_DATA_BASE_ADDR;
	int *label_trn = TRN_LABEL_BASE_ADDR;
	int *label_tst = TST_LABEL_BASE_ADDR;

	/** Final classification output */
	int *label_prediction = OUT_LABELS_BASE_ADDR;
	/** Output distance matrix */
	float *distances = OUT_DIST_BASE_ADDR;

	/** Temporary trn labels array used for classification */
	int closest[K];

	/* DMA variables */
	int status;
	float *tx_buffer_ptr, *rx_buffer_ptr;

	/************************************************************************/

	/* Distance Calculation */

	XTime_GetTime(&t_kernel_start);

	/* For each object in testing set */
	for (i = 0; i < NUM_TST_OBJ; i++) {

		/* Send a test object - DMA 0 */
		tx_buffer_ptr = (float *)&(data_tst[i*FEATURES]);
		status = XAxiDma_SimpleTransfer(&AxiDma0, (UINTPTR) tx_buffer_ptr,
			SIZE_FEATURE * FEATURES, XAXIDMA_DMA_TO_DEVICE);
		if (status != XST_SUCCESS){
			xil_printf("DMA0: Failed snd tst obj\n");
			return XST_FAILURE;
		}

		/* Wait for TX */
		while (XAxiDma_Busy(&AxiDma0, XAXIDMA_DMA_TO_DEVICE)){};

		/* Receive distance buffer - DMA 0 */
		rx_buffer_ptr = (float *) (distances + i*NUM_TRN_OBJ);
		status = XAxiDma_SimpleTransfer(&AxiDma0,(UINTPTR) (rx_buffer_ptr),
			sizeof(float) * NUM_TRN_OBJ , XAXIDMA_DEVICE_TO_DMA);
		if (status != XST_SUCCESS) {
			xil_printf("DMA0: Failed rcv dist\n");
			return XST_FAILURE;
		}

		/* Send full training set - DMA 0 */
		tx_buffer_ptr = (float *)data_trn;
		status = XAxiDma_SimpleTransfer(&AxiDma0, (UINTPTR) tx_buffer_ptr,
			NUM_TRN_OBJ * SIZE_FEATURE * FEATURES,
			XAXIDMA_DMA_TO_DEVICE);
	    if (status != XST_SUCCESS) {
	    	xil_printf("DMA0: Failed snd trn\n");
	    	return XST_FAILURE;
	    }

	    /* Wait for TX and RX */
	    while (XAxiDma_Busy(&AxiDma0,XAXIDMA_DMA_TO_DEVICE)){}
	    while (XAxiDma_Busy(&AxiDma0,XAXIDMA_DEVICE_TO_DMA)){}
	}

	XTime_GetTime(&t_kernel_end);

	/************************************************************************/

	// TODO - Sanity Check
	 Xil_DCacheInvalidateRange((INTPTR) (distances),
	    		(unsigned)(sizeof(float)) * NUM_TST_OBJ * NUM_TRN_OBJ);

	/************************************************************************/

	/* Classification */

	/* From the distance matrix assign labels to testing objects */
	for (i = 0; i < NUM_TST_OBJ; i++){
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

	xil_printf("Total of %d correctly classified out of %d", correct, NUM_TST_OBJ);
	xil_printf("\nTiming Report (us)\nKernel Execution: %d\nTotal Execution: %d\n%d;%d;",
			(int) (1.0 * (t_kernel_end - t_kernel_start) / (COUNTS_PER_SECOND/1000000)),
			(int) (1.0 * (t_end - t_start) / (COUNTS_PER_SECOND/1000000)),
			(int) (1.0 * (t_kernel_end - t_kernel_start) / (COUNTS_PER_SECOND/1000000)),
			(int) (1.0 * (t_end - t_start) / (COUNTS_PER_SECOND/1000000))
	);
	return XST_SUCCESS;
}

/************************************************************************/

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


