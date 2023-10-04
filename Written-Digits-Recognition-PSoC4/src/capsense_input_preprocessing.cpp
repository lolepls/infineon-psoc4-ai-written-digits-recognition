/*
 * capsense_input_preprocessing.c
 *
 *  Created on: 24 lug 2023
 *      Author: Gioele Mombelli (IFI EMEA SMD TMS)
 */

/*******************************************************************************
 * Include header files
 ******************************************************************************/

#include "cybsp.h"
#include "cy_pdl.h"
#include "cyhal.h"

#include "cy_retarget_io.h"
#include "cycfg_capsense.h"
#include "raw_data_size.h"
#include "bitmatrix_data.h"
#include "intensity_LUT.h"

void input_preprocessing(BitMatrix112x112* raw_data, uint8_t input_data[28][28]);
void fillInputMatrix(BitMatrix112x112* raw_data);

/*******************************************************************************
* Macros
*******************************************************************************/
#define MSC_CAPSENSE_WIDGET_INACTIVE     (0u)

#define CENTER_INTENSITY				 /*(255)*/	(255)
#define NEAR_INTENSITY				 	 /*(84)*/	(255)
#define FAR_INTENSITY				 	 /*(63)*/	(255)

// Macros for bit manipulation
#define SET_BIT(matrix, row, col)   ((matrix)->data[row][(col) >> 4] |= (1U << ((col) & 0x0F)))
#define CLEAR_BIT(matrix, row, col) ((matrix)->data[row][(col) >> 4] &= ~(1U << ((col) & 0x0F)))
#define TOGGLE_BIT(matrix, row, col) ((matrix)->data[row][(col) >> 4] ^= (1U << ((col) & 0x0F)))
#define READ_BIT(matrix, row, col)  (((matrix)->data[row][(col) >> 4] >> ((col) & 0x0F)) & 0x01)



/*******************************************************************************
* Global variables
*******************************************************************************/

uint8_t acquired_data = 0;
cy_stc_capsense_touch_t* touch_data;
bool timer_stopped = true;


/*******************************************************************************
* Function Name: acquire_data
********************************************************************************
* Summary:
*
*
*******************************************************************************/
// Function to clear the entire BitMatrix112x112
void clearMatrix(BitMatrix112x112* matrix) {
    for (uint8_t i = 0; i < 112; i++) {
        for (uint8_t j = 0; j < 7; j++) {
            matrix->data[i][j] = 0;
        }
    }
}

/*******************************************************************************
* Function Name: acquire_data
********************************************************************************
* Summary:
* If a touch is detected, corresponding coordinates are
*
*******************************************************************************/
void acquire_data(BitMatrix112x112* raw_data, uint8_t input_data[28][28], bool* data_ready, cyhal_timer_t* timer_obj, bool* timer_done)
{
    if(MSC_CAPSENSE_WIDGET_INACTIVE != Cy_CapSense_IsWidgetActive(CY_CAPSENSE_TOUCHPAD0_WDGT_ID, &cy_capsense_context))
    {

    	if(!timer_stopped){
    		cyhal_timer_stop(timer_obj); // Stops timer if running
    		timer_stopped = true;
    	}

    	Cy_GPIO_Write(CYBSP_USER_LED_PORT, CYBSP_USER_LED_NUM, CYBSP_LED_STATE_ON);
        /* Start the next scan */
        Cy_CapSense_ScanAllSlots(&cy_capsense_context);
        touch_data = Cy_CapSense_GetTouchInfo(CY_CAPSENSE_TOUCHPAD0_WDGT_ID, &cy_capsense_context);

        fillInputMatrix(raw_data);

        acquired_data = 1;
    }
    else
    {
        Cy_GPIO_Write(CYBSP_USER_LED_PORT, CYBSP_USER_LED_NUM, CYBSP_LED_STATE_OFF);

        if(acquired_data){

            // Start the timer with the configured settings
            cyhal_timer_start(timer_obj);
            timer_stopped = false;

            // When timer is done...
            if(*timer_done){

				/*Start input data preprocessing...*/
				input_preprocessing(raw_data, input_data);

				*data_ready = true;

				/*Input data reset...*/
				clearMatrix(raw_data);

				acquired_data = 0;

				cyhal_timer_stop(timer_obj);
				cyhal_timer_reset(timer_obj);
				timer_stopped = true;
				*timer_done = false;
            }
        }

    }
}

/*******************************************************************************
* Function Name: fillInputMatrix
********************************************************************************
* Summary:
*  This function prints x and y coordinates of touch on Capsense pad.
*
*******************************************************************************/
void fillInputMatrix(BitMatrix112x112* raw_data){

	cy_stc_capsense_position_t * coordinates = touch_data->ptrPosition;
	uint16_t x = coordinates->x;
	uint16_t y = coordinates->y;

	y = MAX_Y-y;


	printf("(%d,%d)\n\r", x, y);

	SET_BIT(raw_data, x, y);

	// Update values around the center
	if (x + 1 < MAX_X) {
	    SET_BIT(raw_data, x+1, y);

	}
	if (x - 1 >= MIN_X) {
	    SET_BIT(raw_data, x-1, y);
	}
	if (y + 1 < MAX_Y) {
	    SET_BIT(raw_data, x, y+1);
	}
	if (y - 1 >= 0) {
	    SET_BIT(raw_data, x, y-1);
	}
	if (x + 1 < MAX_X && y + 1 < MAX_Y) {
	    SET_BIT(raw_data, x+1, y+1);
	}
	if (x + 1 < MAX_X && y - 1 >= 0) {
	    SET_BIT(raw_data, x+1, y-1);
	}
	if (x - 1 >= MIN_X && y + 1 < MAX_Y) {
	    SET_BIT(raw_data, x-1, y+1);
	}
	if (x - 1 >= 0 && y - 1 >= 0) {
	    SET_BIT(raw_data, x-1, y-1);
	}

	// Update values further away from the center
	if (x + 2 < MAX_X) {
	    SET_BIT(raw_data, x+2, y);
	}
	if (x - 2 >= MIN_X) {
	    SET_BIT(raw_data, x-2, y);
	}
	if (y + 2 < MAX_Y) {
	    SET_BIT(raw_data, x, y+2);
	}
	if (y - 2 >= 0) {
	    SET_BIT(raw_data, x, y-2);
	}
	if (x + 2 < MAX_X && y + 2 < MAX_Y) {
	    SET_BIT(raw_data, x+2, y+2);
	}
	if (x + 2 < MAX_X && y - 2 >= 0) {
	    SET_BIT(raw_data, x+2, y-2);
	}
	if (x - 2 >= MIN_X && y + 2 < MAX_Y) {
	    SET_BIT(raw_data, x-2, y+2);
	}
	if (x - 2 >= MIN_X && y - 2 >= 0) {
	    SET_BIT(raw_data, x-2, y-2);
	}

	// Update values further away from the center
	if (x + 3 < MAX_X) {
	    SET_BIT(raw_data, x+3, y);
	}
	if (x - 3 >= MIN_X) {
	    SET_BIT(raw_data, x-3, y);
	}
	if (y + 3 < MAX_Y) {
	    SET_BIT(raw_data, x, y+3);
	}
	if (y - 3 >= 0) {
	    SET_BIT(raw_data, x, y-3);
	}
	if (x + 3 < MAX_X && y + 3 < MAX_Y) {
	    SET_BIT(raw_data, x+3, y+3);
	}
	if (x + 3 < MAX_X && y - 3 >= 0) {
	    SET_BIT(raw_data, x+3, y-3);
	}
	if (x - 3 >= MIN_X && y + 3 < MAX_Y) {
	    SET_BIT(raw_data, x-3, y+3);
	}
	if (x - 3 >= MIN_X && y - 3 >= 0) {
	    SET_BIT(raw_data, x-3, y-3);
	}

    return;

}

/*******************************************************************************
* Function Name: input_preprocessing(BitMatrix112x112 *raw_data, uint8_t input_data[][])
********************************************************************************
* Summary:
* This function preprocesses the CAPSENSE acquired by rescaling it.
*
* Output: 28x28 image for NN input
*
*******************************************************************************/

/*Internal rescaling function declaration:*/
// Function to rescale the 112x112 image to a 28x28 image
void rescale_image(BitMatrix112x112 *raw_data, uint8_t input_data[28][28]) {

    // Calculate the scaling factor
    const int scaling_factor = 4;

    // Loop over the target 28x28 matrix
    for (int row = 0; row < 28; row++) {
        for (int col = 0; col < 28; col++) {
            int sum = 0;

            // Find the corresponding region in the source 128x128 matrix
            for (int i = 0; i < scaling_factor; i++) {
                for (int j = 0; j < scaling_factor; j++) {
                    int source_row = row * scaling_factor + i;
                    int source_col = col * scaling_factor + j;

                    // Check if the bit in the source matrix is set (white pixel)
                    if (READ_BIT(raw_data, source_row, source_col) == 1) {
                        sum++;
                    }
                }
            }

            // Determine the intensity value
            uint8_t intensity = intensity_table[sum];

            // Set the intensity value in the target 28x28 matrix
            input_data[row][col] = intensity;
        }
    }
}


void input_preprocessing(BitMatrix112x112* raw_data, uint8_t input_data[28][28]){

	/*Image preprocessing steps: rescaling, mirroring and rotating.*/
	rescale_image(raw_data, input_data);

	return;


}

