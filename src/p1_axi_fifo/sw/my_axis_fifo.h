/*
 * @file my_axis_fifo.h
 * @brief Header file for my_axis_fifo.c
 *
 * @author João Borrego
 */

/***************************** Include Files *********************************/
#include "xparameters.h"
#include "xil_exception.h"
#include "xstreamer.h"
#include "xil_cache.h"
#include "xllfifo.h"
#include "xstatus.h"

/************************** Function Prototypes ******************************/
int my_axis_fifo_init();
void my_send_to_fifo(void *BufPtr, unsigned nBytes);
unsigned my_receive_from_fifo(void *BufPtr, unsigned nBytes);
