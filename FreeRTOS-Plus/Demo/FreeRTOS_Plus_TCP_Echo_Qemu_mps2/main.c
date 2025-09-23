/*
 * FreeRTOS V202212.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <FreeRTOS.h>
#include <task.h>

#include <FreeRTOSConfig.h>
#include <FreeRTOSIPConfig.h>

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <event_groups.h>
#include <queue.h>
// #include "EventLoop.h"
// #include "tcp_server.h"
// #include "evpp_buffer.h"

void vApplicationStackOverflowHook( TaskHandle_t pxTask,
                          char * pcTaskName );
void vApplicationMallocFailedHook( void );
void main_tcp_echo_client_tasks( void );
void vApplicationIdleHook( void );
void vApplicationTickHook( void );
void vAssertCalled( void );

/* Idle Task memory allocation */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize)
{
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
    *ppxIdleTaskStackBuffer = &xIdleStack[0];
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

int main( void )
{
    printf("Inside the C main function\n");
    main_tcp_echo_client_tasks();
    
    // Start the scheduler
    vTaskStartScheduler();

    // Should never reach here
    for (;;);

    return 0;
}

/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook( void )
{
    /* Called if a call to pvPortMalloc() fails because there is insufficient
     * free memory available in the FreeRTOS heap.  pvPortMalloc() is called
     * internally by FreeRTOS API functions that create tasks, queues, software
     * timers, and semaphores.  The size of the FreeRTOS heap is set by the
     * configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */
    taskDISABLE_INTERRUPTS();

    for( ; ; )
    {
    }
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( TaskHandle_t pxTask,
                                    char * pcTaskName )
{
    ( void ) pcTaskName;
    ( void ) pxTask;

    /* Run time stack overflow checking is performed if
     * configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
     * function is called if a stack overflow is detected. */
    taskDISABLE_INTERRUPTS();

    for( ; ; )
    {
    }
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
    /* This is just a trivial example of an idle hook.  It is called on each
     * cycle of the idle task.  It must *NOT* attempt to block.  In this case the
     * idle task just queries the amount of FreeRTOS heap that remains.  See the
     * memory management section on the https://www.FreeRTOS.org web site for memory
     * management options.  If there is a lot of heap memory free then the
     * configTOTAL_HEAP_SIZE value in FreeRTOSConfig.h can be reduced to free up
     * RAM. */
}
/*-----------------------------------------------------------*/

void vApplicationTickHook( void )
{
}
/*-----------------------------------------------------------*/

void iptraceNETWORK_INTERFACE_RECEIVE(uint32_t ulIPAddress) {
    FreeRTOS_debug_printf(("%lu\n", (unsigned long) xTaskGetTickCount()));
}

void iptraceNETWORK_INTERFACE_TRANSMIT(uint32_t ulIPAddress) {
    FreeRTOS_debug_printf(("%lu\n", (unsigned long) xTaskGetTickCount()));
    // FreeRTOS_debug_printf((unsigned long) (xTaskGetTickCount()));  // Record reply send time
}

// Defined in Inner_pre
void vAssertCalled( void )
{
    volatile unsigned long looping = 0;

    taskENTER_CRITICAL();
    {
        /* Use the debugger to set ul to a non-zero value in order to step out
         *      of this function to determine why it was called. */
        while( looping == 0LU )
        {
            portNOP();
        }
    }
    taskEXIT_CRITICAL();
}
/*-----------------------------------------------------------*/
void vLoggingPrintf( const char * pcFormat,
                     ... )
{
    va_list arg;

    va_start( arg, pcFormat );
    vprintf( pcFormat, arg );
    va_end( arg );
}


// Overwrite stub
int __wrap_swprintf(wchar_t *s, size_t n, const wchar_t *fmt, ...) {
    (void)s; (void)n; (void)fmt;
    return 0;
}


#ifdef __cplusplus
}
#endif

