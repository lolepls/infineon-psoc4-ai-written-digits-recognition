/******************************************************************************
* File Name: main.c
*
* Description: This is the source code for the PSoC 4 AI-Based Written Digits Recognition Example
*              for ModusToolbox.
*
* Related Document: See README.md
*
*******************************************************************************
* Copyright 2020-2022, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/

/*******************************************************************************
 * Include header files
 ******************************************************************************/
#include "cybsp.h"
#include "cy_pdl.h"
#include "cyhal.h"

#include "cy_retarget_io.h"
#include "cycfg_capsense.h"


#include "raw_data_size.h"
#include "written-digit-recognition-cnn-8bit.h"
#include "capsense_input_preprocessing.h"
#include "bitmatrix_data.h"
#include "config.h"

#include "tensorflow/lite/core/c/common.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_log.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_profiler.h"
#include "tensorflow/lite/micro/recording_micro_interpreter.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/micro/cortex_m_generic/debug_log_callback.h"



/*******************************************************************************
* Macros
*******************************************************************************/
#define CLEAR_SCREEN "\x1b[2J\x1b[;H"

#define CAPSENSE_MSC0_INTR_PRIORITY      (3u)
#define CAPSENSE_MSC1_INTR_PRIORITY      (3u)
#define CY_ASSERT_FAILED                 (0u)


/*Number of different operations used by your model. This includes both layer operations (i.e. Conv2D, Dense...),
 * activation operations (i.e. SOFTMAX) and quantization operations (i.e. QUANTIZE).*/
#define OPNUM 7


/*Name of your model as defined in the .h file*/
#define MODEL_NAME written_digit_recognition_cnn_8bit_tflite


/*******************************************************************************
* Global Definitions
*******************************************************************************/
static uint8_t input_data[28][28];
BitMatrix112x112 raw_data;

static bool data_ready = false;

bool timer_done = false;
// Timer object used
cyhal_timer_t timer_obj;


/*******************************************************************************
* Function Prototypes
*******************************************************************************/
static void initialize_capsense(void);
static void capsense_msc0_isr(void);
static void capsense_msc1_isr(void);
static void printSerialData(uint8_t* output, uint8_t prediction);
//static void acquireDataset(uint8_t* output);
cy_rslt_t timer_initialization(void);


/*******************************************************************************
* Namespace
*
* Insert here the operations used by your model.
*******************************************************************************/
namespace {
using ModelOpResolver = tflite::MicroMutableOpResolver<OPNUM>;

TfLiteStatus RegisterOps(ModelOpResolver& op_resolver) {
  TF_LITE_ENSURE_STATUS(op_resolver.AddFullyConnected());
  TF_LITE_ENSURE_STATUS(op_resolver.AddConv2D());
  TF_LITE_ENSURE_STATUS(op_resolver.AddMaxPool2D());
  TF_LITE_ENSURE_STATUS(op_resolver.AddQuantize());
  TF_LITE_ENSURE_STATUS(op_resolver.AddSoftmax());
  TF_LITE_ENSURE_STATUS(op_resolver.AddReshape());
  TF_LITE_ENSURE_STATUS(op_resolver.AddMean());
  return kTfLiteOk;
}
}  // namespace

/*******************************************************************************
* Function Name: debug_log_printf
********************************************************************************
* Summary:
* Sets up the standard debug log interface for TFLite Micro
*
* Parameters:
*  s: debug string to be printed
*
* Return:
*  void
*
*******************************************************************************/
void debug_log_printf(const char* s){
    /* Send a string over serial terminal */
    printf(s);
}

/*******************************************************************************
* Function Name: main
********************************************************************************
* Summary:
*  System entrance point. This function performs
*  - initial setup of device
*  - initial setup of CAPSENSE
*  - initial setup of TensorFlow Lite Micro
*  - timers setup
*  - starts the acquisition loop
*  - runs the Neural Network upon an image acquisition
*
* Parameters:
*  none
*
* Return:
*  int
*
*******************************************************************************/
int main(void)
{
    cy_rslt_t result;

    /* Initialize the device and board peripherals */
    result = cybsp_init();

    /* Board init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(CY_ASSERT_FAILED);
    }

    /* Enable global interrupts */
    __enable_irq();

    cy_retarget_io_init(P0_5, P0_4, 115200);

    /* Initialize MSC CapSense */
    initialize_capsense();

    timer_initialization();

    /*TFLite registration of DebugLog*/
    RegisterDebugLogCallback(debug_log_printf);

    /*Define and load model in memory: */
    const tflite::Model* model =::tflite::GetModel(MODEL_NAME);
    TFLITE_CHECK_EQ(model->version(), TFLITE_SCHEMA_VERSION);

    /*Resolution of model operations*/
    ModelOpResolver op_resolver;
    TF_LITE_ENSURE_STATUS(RegisterOps(op_resolver));

    // Manual setting of TFArena. The exact arena usage can be determined
    // using the RecordingMicroInterpreter.
    //constexpr int kTensorArenaSize = 4000;
    constexpr int kTensorArenaSize = 10000;
    uint8_t tensor_arena[kTensorArenaSize];

    /*Interpreter allocation:*/
    tflite::MicroInterpreter interpreter(model, op_resolver, tensor_arena, kTensorArenaSize);
    TF_LITE_ENSURE_STATUS(interpreter.AllocateTensors());


    for(;;)
    {
        if(CY_CAPSENSE_NOT_BUSY == Cy_CapSense_IsBusy(&cy_capsense_context))
        {
            /* Process all widgets */
            Cy_CapSense_ProcessAllWidgets(&cy_capsense_context);

            /* Acquire input data  */
            acquire_data(&raw_data, input_data, &data_ready, &timer_obj, &timer_done);

            if(data_ready){

            	uint16_t input_index = 0;

            	for(int x = 0; x < 28; x++){
            		for(int y = 0; y < 28; y++){

            			interpreter.input(0)->data.uint8[input_index] = input_data[x][y];
            			input_index++;

            			}
            		}

            	printf("***");

                /*Calling inference engine*/
                TF_LITE_ENSURE_STATUS(interpreter.Invoke());

                uint8_t max_output = 0;
                uint8_t prediction_index = 11;

            	/*Checking max output*/
            	for(int k = 0; k<10; k++){

            		uint8_t prediction = interpreter.output(0)->data.uint8[k];

            		if(prediction > max_output){
            			max_output = prediction;
            			prediction_index = k;
            		}
            	}

            	if(max_output < CONFIDENCE_THRESHOLD){
            		prediction_index = 11;
            	}

            	/*Reset the inference flag*/
            	data_ready = false;

            	//printf("\n\r");
            	printSerialData(interpreter.output(0)->data.uint8, prediction_index);
            	//acquireDataset(interpreter.output(0)->data.uint8);

            }

            /* Start the next scan */
            Cy_CapSense_ScanAllSlots(&cy_capsense_context);

        }
    }
}

/*******************************************************************************
* Function Name: printSerialData
********************************************************************************
* Summary:
*  Function that prints data to operate the external GUI listening to the UART.
*
*******************************************************************************/
static void printSerialData(uint8_t* output, uint8_t prediction)
{


    for(int x = 0; x < 28; x++){
    	for(int y = 0; y < 28; y++){
    		if(y == 27 && x == 27){
    			printf("%d", input_data[x][y]);
    		}else{
    		printf("%d,", input_data[x][y]);
    		}
    	}
    }

    printf("*");

    for(int i = 0; i < 9; i++){
    	printf("%d,", output[i]);
    }

    printf("%d", output[9]);

    printf("*");

    printf("%d", prediction);
    printf("completed");
    printf("\n\r");

}

/*******************************************************************************
* Function Name: acquireDataset
********************************************************************************
* Summary:
*  Function that prints data to collect it for building a dataset.
*
*******************************************************************************/
//int label = 0;
//int acquired = 0;
//#define SAMPLES_PER_DIGIT 30
//
//static void acquireDataset(uint8_t* output)
//{
//
//	if(acquired == SAMPLES_PER_DIGIT){
//		acquired = 0;
//		label = label+1;
//	}
//
//	if(label==10){
//		printf("###\n");
//		return;
//	}
//
//	printf("*");
//	printf("%d" , label);
//	printf("*");
//
//    for(int x = 0; x < 28; x++){
//    	for(int y = 0; y < 28; y++){
//    		if(y == 27 && x == 27){
//    			printf("%d", input_data[x][y]);
//    		}else{
//    		printf("%d,", input_data[x][y]);
//    		}
//    	}
//    }
//
//    printf("*");
//    acquired = acquired+1;
//
//    printf("\n");
//
//}


/*******************************************************************************
* Function Name: capsense_msc0_isr
********************************************************************************
* Summary:
*  Wrapper function for handling interrupts from CapSense MSC0 block.
*
*******************************************************************************/
static void capsense_msc0_isr(void)
{
    Cy_CapSense_InterruptHandler(CY_MSC0_HW, &cy_capsense_context);
}


/*******************************************************************************
* Function Name: capsense_msc1_isr
********************************************************************************
* Summary:
*  Wrapper function for handling interrupts from CapSense MSC1 block.
*
*******************************************************************************/
static void capsense_msc1_isr(void)
{
    Cy_CapSense_InterruptHandler(CY_MSC1_HW, &cy_capsense_context);
}


/*******************************************************************************
* Function Name: initialize_capsense
********************************************************************************
* Summary:
*  This function initializes the CapSense blocks and configures the CapSense
*  interrupt.
*
*******************************************************************************/
static void initialize_capsense(void)
{
    cy_capsense_status_t status = CY_CAPSENSE_STATUS_SUCCESS;

    /* CapSense interrupt configuration MSC 0 */
    const cy_stc_sysint_t capsense_msc0_interrupt_config =
    {
        .intrSrc = CY_MSC0_IRQ,
        .intrPriority = CAPSENSE_MSC0_INTR_PRIORITY,
    };


    /* CapSense interrupt configuration MSC 1 */
    const cy_stc_sysint_t capsense_msc1_interrupt_config =
    {
        .intrSrc = CY_MSC1_IRQ,
        .intrPriority = CAPSENSE_MSC1_INTR_PRIORITY,
    };



    /* Capture the MSC HW block and initialize it to the default state. */
    status = Cy_CapSense_Init(&cy_capsense_context);

    if (status != CY_CAPSENSE_STATUS_SUCCESS)
    {
        /* CapSense initialization failed, the middleware may not operate
         * as expected, and repeating of initialization is required.*/
        CY_ASSERT(CY_ASSERT_FAILED);
    }

    if (CY_CAPSENSE_STATUS_SUCCESS == status)
    {
        /* Initialize CapSense interrupt for MSC 0 */
        Cy_SysInt_Init(&capsense_msc0_interrupt_config, capsense_msc0_isr);
        NVIC_ClearPendingIRQ(capsense_msc0_interrupt_config.intrSrc);
        NVIC_EnableIRQ(capsense_msc0_interrupt_config.intrSrc);


        /* Initialize CapSense interrupt for MSC 1 */
        Cy_SysInt_Init(&capsense_msc1_interrupt_config, capsense_msc1_isr);
        NVIC_ClearPendingIRQ(capsense_msc1_interrupt_config.intrSrc);
        NVIC_EnableIRQ(capsense_msc1_interrupt_config.intrSrc);

        /* Initialize the CapSense firmware modules. */
        status = Cy_CapSense_Enable(&cy_capsense_context);
    }

    if(status != CY_CAPSENSE_STATUS_SUCCESS)
    {
        /* This status could fail before tuning the sensors correctly.
         * Ensure that this function passes after the CapSense sensors are tuned
         * as per procedure give in the Readme.md file */
    }
}

static void isr_timer(void* callback_arg, cyhal_timer_event_t event)
{
    (void)callback_arg;
    (void)event;
    // Set the interrupt flag and process it from the application
    timer_done = true;
}


cy_rslt_t timer_initialization()
{
    cy_rslt_t rslt;
    const cyhal_timer_cfg_t timer_cfg =
    {
    	.is_continuous = true,               // Run the timer indefinitely
        .direction     = CYHAL_TIMER_DIR_UP, // Timer counts up
        .is_compare    = false,              // Don't use compare mode
        .period        = 6999,               // Defines the timer period
        .compare_value = 0,                  // Timer compare value, not used
        .value         = 0                   // Initial value of counter
    };

    // Initialize the timer object. Does not use pin output ('pin' is NC) and does not use a
    // pre-configured clock source ('clk' is NULL).
    rslt = cyhal_timer_init(&timer_obj, NC, NULL);
    // Apply timer configuration such as period, count direction, run mode, etc.
    if (CY_RSLT_SUCCESS == rslt)
    {
        rslt = cyhal_timer_configure(&timer_obj, &timer_cfg);
    }
    // Set the frequency of timer to 10000 Hz
    if (CY_RSLT_SUCCESS == rslt)
    {
        rslt = cyhal_timer_set_frequency(&timer_obj, 10000);
    }
    if (CY_RSLT_SUCCESS == rslt)
    {
        // Assign the ISR to execute on timer interrupt
        cyhal_timer_register_callback(&timer_obj, isr_timer, NULL);
        // Set the event on which timer interrupt occurs and enable it
        cyhal_timer_enable_event(&timer_obj, CYHAL_TIMER_IRQ_TERMINAL_COUNT, 3, true);
    }
    return rslt;
}
