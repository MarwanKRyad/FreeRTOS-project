/*
 * This file is part of the µOS++ distribution.
 *   (https://github.com/micro-os-plus)
 * Copyright (c) 2014 Liviu Ionescu.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

// ----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include "diag/trace.h"
#include <time.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"

#define CCM_RAM __attribute__((section(".ccmram")))

/*--------------- software timers declarations section ---------------------*/

static void prvAutoReloadTimerCallbackS1( TimerHandle_t xTimer );
static void prvAutoReloadTimerCallbackS2( TimerHandle_t xTimer );
static void prvAutoReloadTimerCallbackS3( TimerHandle_t xTimer );
static void prvAutoReloadTimerCallbackR( TimerHandle_t xTimer );
static TimerHandle_t xTimerS1 = NULL;
static TimerHandle_t xTimerS2 = NULL;
static TimerHandle_t xTimerS3 = NULL;
static TimerHandle_t xTimerR = NULL;
BaseType_t xTimerS1Started, xTimerS2Started;
BaseType_t xTimerS3Started, xTimerRStarted;

// Sample pragmas to cope with warnings. Please note the related line at
// the end of this function, used to pop the compiler diagnostics status.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wreturn-type"

/*--------------------semaphore declarations section------------*/
SemaphoreHandle_t xSemaphore1;
SemaphoreHandle_t xSemaphore2;
SemaphoreHandle_t xSemaphore3;
SemaphoreHandle_t xSemaphore4;
/*--------------------semaphore declarations and Macros section------------*/
QueueHandle_t xQueue1;
#define Queue_size_config 10

/*--------------------variable declarations section------------*/
int Counter_Transimited1=0,Counter_Transimited2=0,Counter_Transimited3=0;
int Counter_blocked1=0, Counter_blocked2=0, Counter_blocked3=0;
int Counter_Recivied=0;
int iteration=0;
int lower_bound_array[]={50, 80 ,110,140,170,200};
int upper_bound_array[]={150,200,250,300,350,400};
/*Sleeping time by each sendr*/
double SleepS1,SleepS2,SleepS3;
char str1[28]="Time is ";
char str2[28]="Time is ";
char str3[28]="Time is ";
int avgTime1=0;
int avgTime2=0;
int avgTime3=0;
double range=0;
double upper_bound=150;
double lower_bound=50;



#define genrateRandomTime(lower_bound,upper_bound) (lower_bound + ((double)rand() / RAND_MAX) * (upper_bound - lower_bound))


 /*-------------------- Reset functions------------*/
void Reset_function()
{	/* first time called when the program start execution*/
	if(iteration==0)
	{	/*set the initial lower bound*/
		lower_bound=lower_bound_array[0];
		/*set the initial upper bound*/
		upper_bound=upper_bound_array[0];
		iteration++;
	}
	else
	{
		/*............step1...........*/
		trace_puts("");
		trace_printf("iteration number %d",iteration);

		trace_puts("");
		trace_printf("total number of successfully sent messages %d",
		Counter_Transimited1+Counter_Transimited2+Counter_Transimited3);
		trace_puts("");
		trace_printf("total number of blocked sent messages %d",
		Counter_blocked1+Counter_blocked2+Counter_blocked3);
		/*............step2...........*/
		trace_puts("");
		trace_printf("number of successfully sent messages for sender1 %d"
						, Counter_Transimited1);
		trace_puts("");
		trace_printf("number of successfully sent messages for sender2 %d"
						, Counter_Transimited2);
		trace_puts("");
		trace_printf("number of successfully sent messages for sender3 %d"
						, Counter_Transimited3);
		trace_puts("");
		trace_printf("number of blocked messages for sender1 %d"
						, Counter_blocked1);
		trace_puts("");
		trace_printf("number of blocked messages for sender2 %d"
						, Counter_blocked2);
		trace_puts("");
		trace_printf("number of blocked messages for sender3 %d"
						, Counter_blocked3);
		/*assuming the average sender timer period
		 * =total sent message by the task/total sent messages (Successfully sent+ blocked)
		 * */
		trace_puts("");
		trace_printf("average time for task 1 %d"
						, avgTime1/(Counter_blocked1+Counter_Transimited1));
		trace_puts("");
		trace_printf("average time for task 2 %d"
						, avgTime2/(Counter_blocked2+Counter_Transimited2));
		trace_puts("");
		trace_printf("average time for task 3 %d"
						, avgTime3/(Counter_blocked3+Counter_Transimited3));
		trace_puts("");
		trace_printf("-------------------------------------");

		/*............step3...........*/
		Counter_Transimited1=0;
		Counter_Transimited2=0;
		Counter_Transimited3=0;
		Counter_blocked1=0;
		Counter_blocked2=0;
		Counter_blocked3=0;
		Counter_Recivied=0;
		avgTime1=0;
		avgTime2=0;
		avgTime3=0;
		/*............step4...........*/
		xQueueReset(xQueue1);
		/*............step5...........*/
		/* check if the 6 iterations end or not */
		if(iteration<6)
		{
		lower_bound=lower_bound_array[iteration];
		upper_bound=upper_bound_array[iteration];
		}
		iteration++;
		/* check if it's the last iteration */
		if(iteration==7)
			{
				trace_puts("\n end of scheduler");
				/*end the scheduler*/
				vTaskEndScheduler();
			}
	}
}

/*................Sender1Task .............. */
void Sender1Task( void *parameters )
{
	int Temp1;
	char* tem1;
	tem1=&str1[0];
	TickType_t currentTicks1;
	while(1)
	{	/*wait for the semaphore to be released by timer's call back function*/
		xSemaphoreTake( xSemaphore1,portMAX_DELAY);
		/*calculate the current time in ticks */
		currentTicks1 = xTaskGetTickCount();
		/* insert the value of current tick after Time is in the array*/
	    sprintf(str1 + 8, "%ld", currentTicks1);
		/* try to send the current time in ticks then chick if it sent successfully or blocked*/
	    Temp1=xQueueSend(xQueue1,(void*)&(tem1),0);
		if(Temp1!=pdTRUE)
			{
				Counter_blocked1++;
			}
		else
			{
				Counter_Transimited1++;
			}
	}
}

/*................Sender2Task .............. */
void Sender2Task( void *parameters )
{
	int Temp2;
	char* tem2;
	tem2=&str2[0];
	TickType_t currentTicks2;
	while(1)
	{	/*wait for the semaphore to be released by timer's call back function*/
		xSemaphoreTake( xSemaphore2,portMAX_DELAY);
		/*calculate the current time in ticks */
		currentTicks2 = xTaskGetTickCount();
		/* insert the value of current tick after Time is in the array*/
		sprintf(str2 + 8, "%ld", currentTicks2);
		/* try to send the current time in ticks then chick if it sent successfully or blocked*/
		Temp2=xQueueSend(xQueue1,(void*)&(tem2),0);
		if(Temp2!=pdTRUE)
			{
				Counter_blocked2++;
			}
		else
			{
				Counter_Transimited2++;
			}
	}
}

/*................Sender3Task .............. */
void Sender3Task( void *parameters )
{
	int Temp3;
	char* tem3;
	tem3=&str3[0];
	TickType_t currentTicks3;
	while(1)
	{	/*wait for the semaphore to be released by timer's call back function*/
		xSemaphoreTake( xSemaphore3,portMAX_DELAY);
		/*calculate the current time in ticks */
		currentTicks3 = xTaskGetTickCount();
		/* insert the value of current tick after Time is in the array*/
		sprintf(str3 + 8, "%ld", currentTicks3);
		/* try to send the current time in ticks then chick if it sent successfully or blocked*/
		Temp3=xQueueSend(xQueue1,(void*)&(tem3),0);
		if(Temp3!=pdTRUE)
			{
				Counter_blocked3++;
			}
		else
			{
				Counter_Transimited3++;
			}
	}
}


/*................ReceiverTask .............. */
void ReceiverTask( void *parameters )
{	char* received_data;
	while(1)
		{	/*wait for the semaphore to be released by timer's call back function*/
		    xSemaphoreTake( xSemaphore4,portMAX_DELAY);
		    /*check if the Queue contains data or not and receive it */
			if(xQueueReceive(xQueue1,&(received_data),0) == pdTRUE )
				{
					Counter_Recivied++;
				/* to print the current time of ticks */

					  /*
					  trace_puts("");
					  trace_printf(" %s",received_data );
					  */


				}
			else{/* do nothing */}
			/*check if the Queue received the 1000 message or not yet */
			if(Counter_Recivied==100)
			{

				Reset_function();
			}
		}
}

/*.................main function..................*/
int
main(int argc, char* argv[])
{	/* initializing the array that contain Time is XYZ for 3 senders to be 0s */
	for(int i=8;i<=27;i++)
		{
			str1[i]=0; str2[i]=0;str3[i]=0;
		}

	 /*calling Rest function at the begin of the program */
	 Reset_function();

	/* calculate initial sleep time for sender1 */
	SleepS1=genrateRandomTime(lower_bound,upper_bound);
	/* calculate initial sleep time for sender2 */
	SleepS2=genrateRandomTime(lower_bound,upper_bound);
	/* calculate initial sleep time for sender3 */
	SleepS3=genrateRandomTime(lower_bound,upper_bound);

	/*Create Timer1 for sender 1 with variable periodic time */
	xTimerS1 = xTimerCreate( "Timer1", ( pdMS_TO_TICKS(SleepS1) ),
				pdTRUE, ( void * ) 0, prvAutoReloadTimerCallbackS1);
	/*Create Timer2 for sender 2 with variable periodic time */
	xTimerS2 = xTimerCreate( "Timer2", ( pdMS_TO_TICKS(SleepS2) ),
				pdTRUE, ( void * ) 1, prvAutoReloadTimerCallbackS2);
	/*Create Timer3 for sender 3 with variable periodic time */
	xTimerS3 = xTimerCreate( "Timer3", ( pdMS_TO_TICKS(SleepS3) ),
				pdTRUE, ( void * ) 2, prvAutoReloadTimerCallbackS3);
	/*Create TimerR for Receiver with fixed periodic time 100ms */
	xTimerR =  xTimerCreate( "Timer4", ( pdMS_TO_TICKS(100) ),
				pdTRUE, ( void * ) 3, prvAutoReloadTimerCallbackR);

	/*Create sender1Task  with priority  1 */
	xTaskCreate(Sender1Task, "Sender1", 1000 ,( void * ) 100, 1, NULL );
	/*Create sender2Task  with priority  1 */
	xTaskCreate(Sender2Task, "Sender2", 1000 ,( void * ) 100 , 1, NULL );
	/*Create sender3Task  with priority  2 */
	xTaskCreate(Sender3Task, "Sender3", 1000 ,( void * ) 100 , 2, NULL );
	/*Create ReceiverTask  with priority  3 */
	xTaskCreate(ReceiverTask,"Receiver", 1000 ,( void * ) 100 , 3, NULL );
	/*Create Semaphore1 */
	xSemaphore1 = xSemaphoreCreateBinary();
	/*Create Semaphore2 */
	xSemaphore2 = xSemaphoreCreateBinary();
	/*Create Semaphore3 */
	xSemaphore3 = xSemaphoreCreateBinary();
	/*Create Semaphore4 */
	xSemaphore4 = xSemaphoreCreateBinary();
	/*Create Queue1 with defined size */
	xQueue1 = xQueueCreate(Queue_size_config, sizeof(int));
	/* check the handler for the 4 timer if they created start them all */
	if( ( xTimerS1 != NULL ) && ( xTimerS2 != NULL )
		&&( xTimerS3 != NULL ) && ( xTimerR != NULL))
	{
		xTimerS1Started = xTimerStart( xTimerS1,0);
		xTimerS2Started = xTimerStart( xTimerS2,0);
		xTimerS3Started = xTimerStart( xTimerS3,0);
		xTimerRStarted  = xTimerStart( xTimerR ,0);
	}
	/* if the timers started then start the Scheduler*/
	if( xTimerS1Started == pdPASS && xTimerS2Started == pdPASS
	  && xTimerS3Started == pdPASS && xTimerRStarted == pdPASS)
	{
		vTaskStartScheduler();
	}

	return 0;
}

#pragma GCC diagnostic pop


/*.......................Timers callback Functions.......................*/

/* .............call back function of timer one...............*/
static void prvAutoReloadTimerCallbackS1( TimerHandle_t xTimer )
{	/*Calculate the Sleep time for task 1*/
	SleepS1=genrateRandomTime(lower_bound,upper_bound);
	/*Change the period to the random value which stored in SleepS1 */
	xTimerChangePeriod(xTimerS1,(pdMS_TO_TICKS(SleepS1)),0);
	/*calculate the total sleeping time for sent messages by task 1*/
	avgTime1+=SleepS1;
	/*Release the Semaphore for sender1  */
	xSemaphoreGive( xSemaphore1);
}
/* .............call back function of timer two...............*/
static void prvAutoReloadTimerCallbackS2( TimerHandle_t xTimer )
{	/*Calculate the Sleep time for task 2*/
	SleepS2=genrateRandomTime(lower_bound,upper_bound);
	/*Change the period to the random value which stored in SleepS2 */
	xTimerChangePeriod(xTimerS2,(pdMS_TO_TICKS(SleepS2)),0);
	/*calculate the total sleeping time for sent messages by task 2*/
	avgTime2+=SleepS2;
	/*Release the Semaphore for sender2  */
	xSemaphoreGive( xSemaphore2);
}
/* .............call back function of timer three...............*/
static void prvAutoReloadTimerCallbackS3( TimerHandle_t xTimer )
{	/*Calculate the Sleep time for task 3*/
	SleepS3=genrateRandomTime(lower_bound,upper_bound);
	/*Change the period to the random value which stored in SleepS3 */
	xTimerChangePeriod(xTimerS3,(pdMS_TO_TICKS(SleepS3)),0);
	/*calculate the total sleeping time for sent messages by task 3*/
	avgTime3+=SleepS3;
	/*Release the Semaphore for sender3  */
	xSemaphoreGive( xSemaphore3);
}
/* .............call back function of receiver timer...............*/
static void prvAutoReloadTimerCallbackR( TimerHandle_t xTimer )
{	/*Release the Semaphore for Receiver  */
	xSemaphoreGive( xSemaphore4);
}

/*-----------------------Hook functions section-------------------------*/

void vApplicationMallocFailedHook( void )
{
	/* Called if a call to pvPortMalloc() fails because there is insufficient
	free memory available in the FreeRTOS heap.  pvPortMalloc() is called
	internally by FreeRTOS API functions that create tasks, queues, software
	timers, and semaphores.  The size of the FreeRTOS heap is set by the
	configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
	( void ) pcTaskName;
	( void ) pxTask;

	/* Run time stack overflow checking is performed if
	configconfigCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	function is called if a stack overflow is detected. */
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
	//HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON,PWR_SLEEPENTRY_WFI);

volatile size_t xFreeStackSpace;

	/* This function is called on each cycle of the idle task.  In this case it
	does nothing useful, other than report the amout of FreeRTOS heap that
	remains unallocated. */
	xFreeStackSpace = xPortGetFreeHeapSize();

	if( xFreeStackSpace > 100 )
	{
		/* By now, the kernel has allocated everything it is going to, so
		if there is a lot of heap remaining unallocated then
		the value of configTOTAL_HEAP_SIZE in FreeRTOSConfig.h can be
		reduced accordingly. */
	}
}

void vApplicationTickHook(void) {
}

StaticTask_t xIdleTaskTCB CCM_RAM;
StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE] CCM_RAM;

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize) {
  /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
  state will be stored. */
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

  /* Pass out the array that will be used as the Idle task's stack. */
  *ppxIdleTaskStackBuffer = uxIdleTaskStack;

  /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
  Note that, as the array is necessarily of type StackType_t,
  configMINIMAL_STACK_SIZE is specified in words, not bytes. */
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

static StaticTask_t xTimerTaskTCB CCM_RAM;
static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH] CCM_RAM;

/* configUSE_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the
application must provide an implementation of vApplicationGetTimerTaskMemory()
to provide the memory that is used by the Timer service task. */
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize) {
  *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
  *ppxTimerTaskStackBuffer = uxTimerTaskStack;
  *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
