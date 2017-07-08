/*
 * @file knn_dma.c
 * @brief KNN classifier HW/SW implementation - CPU0
 *
 * This program runs a KNN implementation with configurable parameter K.
 * The dataset must be uploaded to memory before execution.
 * Hardware Acceleration is used in the calculation of squared euclidean
 * distances between testing and training objects.
 * Direct Memory Access (DMA) provides fast access to data in the hardware.
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

#include "data_cpu0.h"

/************************************************************************/

/* Macros Definition */

/** K-nearest neighbours parameter */
#define K 3

/* Device hardware build related constants. */
#define DMA_0_DEV_ID XPAR_AXIDMA_0_DEVICE_ID
#define DMA_1_DEV_ID XPAR_AXIDMA_1_DEVICE_ID

/************************************************************************/

/* Function prototypes */

/* AXI_DMA Functions */
int XAxiDma_Simple_KNN(u16 device_0_ID, u16 device_1_ID);
int init_XAxiDma_SimplePollMode(u16 device_0_ID, u16 device_1_ID);

/* K-Selection sort */
void selectionSortK(float *distances, int *smallest, int size, int k);

/************************************************************************/

/* Global Variables */

/** Global AXI DMA instance */
XAxiDma AxiDma0, AxiDma1;

/** Global Sync Semaphore */
volatile int *sync = SEM_ADDR;

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

	xil_printf("CPU0: Started\n");

	Xil_DCacheInvalidate();

	/* Disable cache on OCM region */
	Xil_SetTlbAttributes(0xFFFF0000, 0x14de2);

	*sync = START_0;
	while (*sync != START_1){};

	xil_printf("CPU0: SYNC\n");

	/* Start Timer */
	XTime_GetTime(&t_start);

	/* Initialise DMA in poll mode for simple transfer */
	Status = init_XAxiDma_SimplePollMode(DMA_0_DEV_ID, DMA_1_DEV_ID);
	if (Status != XST_SUCCESS) {
		xil_printf("init_XAxiDma_SimplePollMode: Failed\r\n");
		return XST_FAILURE;
	}

		//xil_printf("CPU0: Configured\n");

	Status = XAxiDma_Simple_KNN(DMA_0_DEV_ID, DMA_1_DEV_ID);
	if (Status != XST_SUCCESS) {
		xil_printf("XAxiDma_Simple_KNN: Failed\r\n");
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

/************************************************************************/

/**
 * @brief Initialises the 2 DMAs
 * @param device_0_ID First DMA ID
 * @param device_1_ID Second DMA ID
 * @return XST_SUCCESS on success, XST_FAILURE otherwise
 */
int init_XAxiDma_SimplePollMode(u16 device_0_ID, u16 device_1_ID){

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

  CfgPtr = XAxiDma_LookupConfig(device_1_ID);
  if (!CfgPtr) {
    xil_printf("DMA1: No config found for %d\r\n", device_1_ID);
    return XST_FAILURE;
  }

  Status = XAxiDma_CfgInitialize(&AxiDma1, CfgPtr);
  if (Status != XST_SUCCESS) {
    xil_printf("DMA1: Initialization failed %d\r\n", Status);
    return XST_FAILURE;
  }

  Status = XAxiDma_CfgInitialize(&AxiDma1, CfgPtr);
    if (Status != XST_SUCCESS) {
      xil_printf("DMA1: Initialization failed %d\r\n", Status);
      return XST_FAILURE;
    }

  if(XAxiDma_HasSg(&AxiDma0)){
    xil_printf("DMA0: Device configured as SG mode \r\n");
    return XST_FAILURE;
  }

  if(XAxiDma_HasSg(&AxiDma1)){
    xil_printf("DMA1: Device configured as SG mode \r\n");
    return XST_FAILURE;
  }

  /* Disable interrupts, polling mode is used instead */
  XAxiDma_IntrDisable(&AxiDma0, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DEVICE_TO_DMA);
  XAxiDma_IntrDisable(&AxiDma0, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DMA_TO_DEVICE);
  XAxiDma_IntrDisable(&AxiDma1, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DEVICE_TO_DMA);
  XAxiDma_IntrDisable(&AxiDma1, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DMA_TO_DEVICE);

  return XST_SUCCESS;
}

/**
 * @brief Main KNN Function
 * @param device_0_ID First DMA ID
 * @param device_1_ID Second DMA ID
 * @return XST_SUCCESS on success, XST_FAILURE otherwise
 */
int XAxiDma_Simple_KNN(u16 device_0_ID, u16 device_1_ID){

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
	for (i = FIRST_CPU0; i <= LAST_CPU0; i+= 2) {

		/* Send a test object - DMA 0 */
		tx_buffer_ptr = (float *)&(data_tst[i*FEATURES]);
		status = XAxiDma_SimpleTransfer(&AxiDma0, (UINTPTR) tx_buffer_ptr,
				SIZE_FEATURE * FEATURES, XAXIDMA_DMA_TO_DEVICE);
		if (status != XST_SUCCESS){
			xil_printf("DMA0: Failed snd tst obj\n");
			return XST_FAILURE;
		}

		if ((i+1) <= LAST_CPU0){
			/* Send a test object - DMA 1 */
			tx_buffer_ptr = (float *)&(data_tst[(i+1)*FEATURES]);
			status = XAxiDma_SimpleTransfer(&AxiDma1, (UINTPTR) tx_buffer_ptr,
					SIZE_FEATURE * FEATURES, XAXIDMA_DMA_TO_DEVICE);
			if (status != XST_SUCCESS){
				xil_printf("DMA1: Failed snd tst obj\n");
				return XST_FAILURE;
			}
		}

		/* Wait for TX */
		while (XAxiDma_Busy(&AxiDma0, XAXIDMA_DMA_TO_DEVICE)){};
		if ((i+1) <= LAST_CPU0){
			while (XAxiDma_Busy(&AxiDma1, XAXIDMA_DMA_TO_DEVICE)){};
		}

		/* Receive distance buffer - DMA 0 */
		rx_buffer_ptr = (float *) (distances + i*NUM_TRN_OBJ);
		status = XAxiDma_SimpleTransfer(&AxiDma0,(UINTPTR) (rx_buffer_ptr),
				sizeof(float) * NUM_TRN_OBJ , XAXIDMA_DEVICE_TO_DMA);
		if (status != XST_SUCCESS) {
			xil_printf("DMA0: Failed rcv dist\n");
			return XST_FAILURE;
		}

		/* Receive distance buffer - DMA 1 */
		if ((i+1) <= LAST_CPU0){
			rx_buffer_ptr = (float *) (distances + (i+1)*NUM_TRN_OBJ);
			status = XAxiDma_SimpleTransfer(&AxiDma1,(UINTPTR) (rx_buffer_ptr),
					sizeof(float) * NUM_TRN_OBJ , XAXIDMA_DEVICE_TO_DMA);
			if (status != XST_SUCCESS) {
				xil_printf("DMA1: Failed rcv dist\n");
				return XST_FAILURE;
			}
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

	    if ((i+1) <= LAST_CPU0){
			/* Send full training set - DMA 1 */
			status = XAxiDma_SimpleTransfer(&AxiDma1, (UINTPTR) tx_buffer_ptr,
							NUM_TRN_OBJ * SIZE_FEATURE * FEATURES,
							XAXIDMA_DMA_TO_DEVICE);
			if (status != XST_SUCCESS) {
				xil_printf("DMA1: Failed snd trn\n");
				return XST_FAILURE;
			}
	    }

	    /* Wait for TX and RX */
	    while (XAxiDma_Busy(&AxiDma0,XAXIDMA_DMA_TO_DEVICE)){}
	    while (XAxiDma_Busy(&AxiDma0,XAXIDMA_DEVICE_TO_DMA)){}
	    if ((i+1) <= LAST_CPU0){
			while (XAxiDma_Busy(&AxiDma1,XAXIDMA_DMA_TO_DEVICE)){}
			while (XAxiDma_Busy(&AxiDma1,XAXIDMA_DEVICE_TO_DMA)){}
	    }
	}

	/************************************************************************/

	/* Synchronisation */
	*sync = END_DIST_0;
	/* Wait for CPU1 to finish calculations */
	while(*sync != END_DIST_1){}

	XTime_GetTime(&t_kernel_end);

	// TODO - DEBUG
	//xil_printf("CPU0: CPU1 finished dist\n");

    Xil_DCacheInvalidateRange((INTPTR) (distances),
    		(unsigned)(sizeof(float)) * NUM_TST_OBJ * NUM_TRN_OBJ);

    /************************************************************************/

	/* Classification */

    /* From the distance matrix assign labels to testing objects */
	for (i = FIRST_CPU0_CLASS; i <= LAST_CPU0_CLASS; i++){
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

	*sync = END_CLASS_0;
	while(*sync != END_CLASS_1){}

	// TODO - DEBUG
	//xil_printf("CPU0: CPU1 has finished\n");

	Xil_DCacheInvalidateRange((INTPTR) (label_prediction),
		    	(unsigned)(sizeof(int) * NUM_TST_OBJ));

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


