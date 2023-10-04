/*
 * capsense_input_preprocessing.h
 *
 *  Created on: 24 lug 2023
 *      Author: Gioele Mombelli (IFI EMEA SMD TMS)
 */

#ifndef SRC_CAPSENSE_INPUT_PREPROCESSING_H_
#define SRC_CAPSENSE_INPUT_PREPROCESSING_H_

#include "raw_data_size.h"
#include "bitmatrix_data.h"

void acquire_data(BitMatrix112x112* raw_data, uint8_t input_data[28][28], bool* data_ready, cyhal_timer_t* timer_obj, bool* timer_done);



#endif /* SRC_CAPSENSE_INPUT_PREPROCESSING_H_ */
