/*
 * @file my_axis_fifo.c
 * @brief Implements basic AXI Stream FIFO I/O interface functions
 *
 * @author hcn
 */

// Adapted from XLlFifo_polling_example.c

#include "my_axis_fifo.h"

/************************** Variable Definitions *****************************/
/*
 * Device instance definitions
 */
XLlFifo FifoInstance;

int my_axis_fifo_init()
{
	XLlFifo_Config *Config;
	XLlFifo *InstancePtr;
	int Status;

	InstancePtr = &FifoInstance;
	Status = XST_SUCCESS;

	/* Initialize the Device Configuration Interface driver */
	Config = XLlFfio_LookupConfig(XPAR_AXI_FIFO_0_DEVICE_ID);
	if (!Config) {
		xil_printf("No config found for %d\r\n", XPAR_AXI_FIFO_0_DEVICE_ID);
		return XST_FAILURE;
	}

	Status = XLlFifo_CfgInitialize(InstancePtr, Config, Config->BaseAddress);
	if (Status != XST_SUCCESS) {
		xil_printf("Initialization failed\n\r");
		return Status;
	}

	/* Check for the Reset value */
	Status = XLlFifo_Status(InstancePtr);
	XLlFifo_IntClear(InstancePtr,0xffffffff);
	Status = XLlFifo_Status(InstancePtr);
	if(Status != 0x0) {
		xil_printf("\n ERROR : Reset value of ISR0 : 0x%x\t Expected : 0x0\n\r",
			       XLlFifo_Status(InstancePtr));
		return XST_FAILURE;
	}

	return Status;
}

// A frame is transmitted by using the following sequence:
// 1) call XLlFifo_Write() one or more times to write all of the bytes in the next frame.
// 2) call XLlFifo_TxSetLen() to begin the transmission of frame just written.

void my_send_to_fifo(void *BufPtr, unsigned nBytes)
{
  XLlFifo_Write(&FifoInstance, BufPtr, nBytes);
  XLlFifo_TxSetLen(&FifoInstance, nBytes);
  /* Check for Transmission completion */
  // while( !(XLlFifo_IsTxDone(&FifoInstance)) );
}

// A frame is received by using the following sequence:
// 1) call XLlFifo_RxOccupancy() to check the occupancy count
// 2) call XLlFifo_RxGetLen() to get the length of the next incoming frame
// 3) call XLlFifo_Read() one or more times to read the number of bytes reported by XLlFifo_RxGetLen().

unsigned my_receive_from_fifo(void *BufPtr, unsigned nBytes)
{
	unsigned bytes_to_read=0, frame_len;

 	if (XLlFifo_RxOccupancy(&FifoInstance)) {
 		frame_len = (unsigned)XLlFifo_RxGetLen(&FifoInstance);
 		bytes_to_read = (nBytes > frame_len) ? frame_len : nBytes;
		XLlFifo_Read(&FifoInstance, BufPtr, bytes_to_read);
 	}
    /* Check for Reception completion */
    // while( !(XLlFifo_IsRxDone(&FifoInstance)) );
 	return bytes_to_read;
}
