/*
 * FreeRTOS Kernel V10.2.0
 * Copyright (C) 2019 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/*
	NOTE : Tasks run in system mode and the scheduler runs in Supervisor mode.
	The processor MUST be in supervisor mode when vTaskStartScheduler is
	called.  The demo applications included in the FreeRTOS.org download switch
	to supervisor mode prior to main being called.  If you are not using one of
	these demo application projects then ensure Supervisor mode is used.
*/


/*
 * Creates all the demo application tasks, then starts the scheduler.  The WEB
 * documentation provides more details of the demo application tasks.
 *
 * Main.c also creates a task called "Check".  This only executes every three
 * seconds but has the highest priority so is guaranteed to get processor time.
 * Its main function is to check that all the other tasks are still operational.
 * Each task (other than the "flash" tasks) maintains a unique count that is
 * incremented each time the task successfully completes its function.  Should
 * any error occur within such a task the count is permanently halted.  The
 * check task inspects the count of each task to ensure it has changed since
 * the last time the check task executed.  If all the count variables have
 * changed all the tasks are still executing error free, and the check task
 * toggles the onboard LED.  Should any task contain an error at any time
 * the LED toggle rate will change from 3 seconds to 500ms.
 *
 */

/* Standard includes. */
#include <stdlib.h>
#include <stdio.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "lpc21xx.h"

/* Peripheral includes. */
#include "serial.h"
#include "GPIO.h"
/* Modify for Communication Queue */
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"

char runTimeStatsBuff[190]; //initialized by 0
int task1_in = 0,task1_out = 0,task1_total = 0,task2_in = 0, task2_out = 0, task2_total = 0, system_time = 0, cpu_load = 0;

/*-----------------------------------------------------------*/

/* Constants to setup I/O and processor. */
#define mainBUS_CLK_FULL	( ( unsigned char ) 0x01 )

/* Constants for the ComTest demo application tasks. */
#define mainCOM_TEST_BAUD_RATE	( ( unsigned long ) 115200 )

/* Periods of Tasks */

/*
 * Configure the processor for use with the Keil demo board.  This is very
 * minimal as most of the setup is managed by the settings in the project
 * file.
 */
static void prvSetupHardware( void );
/*-----------------------------------------------------------*/
/* Tick Hook function */
void vApplicationTickHook( void );

/* Idle Hook function */
void vApplicationIdleHook( void );
/*
 * Application entry point:
 * Starts all the other tasks, then starts the scheduler.
 */
/* Modify for Communication Queue */
/* Create Communication Queue between Producer Tasks and Consumer Task */
QueueHandle_t Comm_Queue_Handle = NULL;
/* Periodicity = 50 , Deadline = 50 */
#define BUTTON1_ON_ID      1
#define BUTTON1_OFF_ID     2

void Button_1_Monitor(void * pvParameters )
{
    volatile char flag = 0;
    volatile char Button1_ID = 0;
    volatile TickType_t xLastWakeTime = xTaskGetTickCount();
    vTaskSetApplicationTaskTag( NULL, ( void * ) 1 );
    for(;;)
    {
        /* Write your code here */
        if(GPIO_read(PORT_1, PIN0) == PIN_IS_HIGH)
        {
            flag = 1;
            Button1_ID = BUTTON1_ON_ID;
            /* Modify for Communication Queue */
            if(Comm_Queue_Handle  != 0)
            {
                xQueueSend( Comm_Queue_Handle, ( void * )&Button1_ID, (TickType_t) 0 );
            }
        }
        else if (GPIO_read(PORT_1, PIN0) == PIN_IS_LOW)
        {
            if(flag == 1 )
            {
                Button1_ID = BUTTON1_OFF_ID;
                flag = 0;
                /* Modify for Communication Queue */
                if(Comm_Queue_Handle  != 0)
                {
                    xQueueSend( Comm_Queue_Handle, ( void * )&Button1_ID, (TickType_t) 0 );
                }
            }
        }
        vTaskDelayUntil( (TickType_t *)&xLastWakeTime, 50 );
    }
}
#define BUTTON2_ON_ID      3
#define BUTTON2_OFF_ID     4
#define PERIODIC_STRING_ID 5
/* Periodicity = 50 , Deadline = 50 */

void Button_2_Monitor(void * pvParameters )
{
    volatile char flag = 0;
    volatile char Button2_ID = 0;
    volatile TickType_t xLastWakeTime = xTaskGetTickCount();
    vTaskSetApplicationTaskTag( NULL, ( void * ) 2 );
    for(;;)
    {
        /* Write your code here */
        if(GPIO_read(PORT_1, PIN1) == PIN_IS_HIGH)
        {
            flag = 1;
            Button2_ID = BUTTON2_ON_ID;
            /* Modify for Communication Queue */
            if(Comm_Queue_Handle  != 0)
            {
                xQueueSend( Comm_Queue_Handle, ( void * )&Button2_ID, (TickType_t) 0 );
            }
        }
        else if (GPIO_read(PORT_1, PIN1) == PIN_IS_LOW)
        {
            if(flag == 1 )
            {
                Button2_ID = BUTTON2_OFF_ID;
                flag = 0;
                /* Modify for Communication Queue */
                if(Comm_Queue_Handle  != 0)
                {
                    xQueueSend( Comm_Queue_Handle, ( void * )&Button2_ID, (TickType_t) 0 );
                }
            }
        }
        vTaskDelayUntil( (TickType_t *)&xLastWakeTime, 50 );
    }
}
/* Periodicity = 100 , Deadline = 100 */

void Periodic_Transmitter(void * pvParameters )
{
    volatile char	Periodic_String_Available_ID =0;
    volatile TickType_t xLastWakeTime = xTaskGetTickCount();
    vTaskSetApplicationTaskTag( NULL, ( void * ) 3 );
    for(;;)
    {
        Periodic_String_Available_ID = PERIODIC_STRING_ID;
        /* Modify for Communication Queue */
        if(Comm_Queue_Handle  != 0)
        {
            xQueueSend( Comm_Queue_Handle, ( void * )&Periodic_String_Available_ID, (TickType_t) 0 );
        }
        vTaskDelayUntil( (TickType_t *)&xLastWakeTime, 100 );
    }
}
void UART_Transmitter(void * pvParameters)
{
    char* Button1_ON_String = "Button1 ON\n\n";
    char* Button1_OFF_String = "Button1 OFF\n\n";
    char* Button2_ON_String = "Button2 ON\n\n";
    char* Button2_OFF_String = "Button2 OFF\n\n";
    char* Periodic_String = "Periodic String\n\n";
    char receiverBuffer = 0;
    volatile TickType_t xLastWakeTime = xTaskGetTickCount();
    vTaskSetApplicationTaskTag( NULL, ( void * ) 4 );
    for(;;)
    {
        /* Modify for Communication Queue */
        if(Comm_Queue_Handle!=NULL)
        {
            if ((xQueueReceive(Comm_Queue_Handle,(void *) &(receiverBuffer), (TickType_t)0))==pdPASS )
            {
                xSerialPutChar('\n');
                if(receiverBuffer == BUTTON1_ON_ID) vSerialPutString((const signed char * const)Button1_ON_String, 13);
                else if (receiverBuffer == BUTTON1_OFF_ID) vSerialPutString((const signed char * const)Button1_OFF_String, 14);
                else if (receiverBuffer == BUTTON2_ON_ID) vSerialPutString((const signed char * const)Button2_ON_String,13);
                else if (receiverBuffer == BUTTON2_OFF_ID) vSerialPutString((const signed char * const)Button2_OFF_String, 14);
                else if (receiverBuffer == PERIODIC_STRING_ID) vSerialPutString((const signed char * const)Periodic_String, 18);
            }
            vTaskDelayUntil( (TickType_t *)&xLastWakeTime, 20 );
        }
    }
}
/* Execution Time = 5*/
void Load_1_Simulation(void * pvParameters)
{
    volatile TickType_t xLastWakeTime = xTaskGetTickCount();
    volatile int i = 0;
    vTaskSetApplicationTaskTag( NULL, ( void * ) 5 );
    for(;;)
    {
        for(i = 0 ; i < 18519 ; i++   ) {}
        vTaskDelayUntil( (TickType_t *)&xLastWakeTime, 10 );
    }
}
/* Execution Time = 5*/

void Load_2_Simulation(void * pvParameters)
{
    volatile int i = 0;
    volatile TickType_t xLastWakeTime = xTaskGetTickCount();
    vTaskSetApplicationTaskTag( NULL, ( void * ) 6 );
    for(;;)
    {
        for(i = 0 ; i < 44444 ; i++   ) {}
        vTaskDelayUntil( (TickType_t *)&xLastWakeTime, 100 );
    }

}

/* Tick's Pin is pin 0 */
void vApplicationTickHook( void )
{
    /* Write Code Here */
    GPIO_write(PORT_0, PIN0, PIN_IS_HIGH);
    GPIO_write(PORT_0, PIN0, PIN_IS_LOW);
}

TaskHandle_t Button1_Monitor_Handle = NULL;
TaskHandle_t Button2_Monitor_Handle = NULL;
TaskHandle_t Periodic_Transmitter_Handle = NULL;
TaskHandle_t UART_Transmitter_Handle = NULL;
TaskHandle_t Load1_Simulation_Handle = NULL;
TaskHandle_t Load2_Simulation_Handle = NULL;

int main( void )
{
    /* Setup the hardware for use with the Keil demo board. */
    prvSetupHardware();
    /* Modify for Communication Queue */
    Comm_Queue_Handle = xQueueCreate(3, sizeof(char*));
    /* Create Tasks here */
    xTaskPeriodicCreate(
        Button_1_Monitor,            /* Function that implements the task. */
        "Button 1 Monitor",          /* Text name for the task. */
        100,                         /* Stack size in words, not bytes. */
        ( void * ) 0,                /* Parameter passed into the task. */
        1,                           /* Priority at which the task is created. */
        &Button1_Monitor_Handle,     /* Used to pass out the created task's handle. */
        50                           /* Task Period */
    );
    xTaskPeriodicCreate(
        Button_2_Monitor,            /* Function that implements the task. */
        "Button 2 Monitor",          /* Text name for the task. */
        100,                         /* Stack size in words, not bytes. */
        ( void * ) 0,                /* Parameter passed into the task. */
        1,                           /* Priority at which the task is created. */
        &Button2_Monitor_Handle,     /* Used to pass out the created task's handle. */
        50                           /* Task Period */
    );
    xTaskPeriodicCreate(
        Periodic_Transmitter,        /* Function that implements the task. */
        "Periodic Transmitter",      /* Text name for the task. */
        100,                         /* Stack size in words, not bytes. */
        ( void * ) 0,                /* Parameter passed into the task. */
        1,                           /* Priority at which the task is created. */
        &Periodic_Transmitter_Handle,/* Used to pass out the created task's handle. */
        100                          /* Task Period */
    );
    xTaskPeriodicCreate(
        UART_Transmitter,            /* Function that implements the task. */
        "UART Transmitter",          /* Text name for the task. */
        100,                         /* Stack size in words, not bytes. */
        ( void * ) 0,                /* Parameter passed into the task. */
        1,                           /* Priority at which the task is created. */
        &UART_Transmitter_Handle,    /* Used to pass out the created task's handle. */
        20                           /* Task Period */
    );
    xTaskPeriodicCreate(
        Load_1_Simulation,           /* Function that implements the task. */
        "Load 1 Simulation",         /* Text name for the task. */
        100,                         /* Stack size in words, not bytes. */
        ( void * ) 0,                /* Parameter passed into the task. */
        1,                           /* Priority at which the task is created. */
        &Load1_Simulation_Handle,    /* Used to pass out the created task's handle. */
        10                           /* Task Period */
    );
    xTaskPeriodicCreate(
        Load_2_Simulation,           /* Function that implements the task. */
        "Load 2 Simulation",         /* Text name for the task. */
        100,                         /* Stack size in words, not bytes. */
        ( void * ) 0,                /* Parameter passed into the task. */
        1,                           /* Priority at which the task is created. */
        &Load2_Simulation_Handle,    /* Used to pass out the created task's handle. */
        100                          /* Task Period */
    );
    /* Now all the tasks have been started - start the scheduler.

    NOTE : Tasks run in system mode and the scheduler runs in Supervisor mode.
    The processor MUST be in supervisor mode when vTaskStartScheduler is
    called.  The demo applications included in the FreeRTOS.org download switch
    to supervisor mode prior to main being called.  If you are not using one of
    these demo application projects then ensure Supervisor mode is used here. */
    vTaskStartScheduler();

    /* Should never reach here!  If you do then there was not enough heap
    available for the idle task to be created. */
    for( ;; );
}
/*-----------------------------------------------------------*/

/* Function to reset timer 1 */
void timer1Reset(void)
{
    T1TCR |= 0x2;
    T1TCR &= ~0x2;
}

/* Function to initialize and start timer 1 */
static void configTimer1(void)
{
    T1PR = 1000;
    T1TCR |= 0x1;
}

static void prvSetupHardware( void )
{
    /* Perform the hardware setup required.  This is minimal as most of the
    setup is managed by the settings in the project file. */

    /* Configure UART */
    xSerialPortInitMinimal(mainCOM_TEST_BAUD_RATE);

    /* Configure GPIO */
    GPIO_init();

    /* Config trace timer 1 and read T1TC to get current tick */
    configTimer1();

    /* Setup the peripheral bus to be the same as the PLL output. */
    VPBDIV = mainBUS_CLK_FULL;
}
/*-----------------------------------------------------------*/


