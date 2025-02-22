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

// What I want to be able to do:
// #include <evpp/tcp_server.h>
// #include <evpp/buffer.h>
// #include <evpp/tcp_conn.h>
// 
// #ifdef _WIN32
// #include "../../winmain-inl.h"
// #endif
// 
// void OnMessage(const evpp::TCPConnPtr& conn,
//                evpp::Buffer* msg) {
//     std::string s = msg->NextAllString();
//     LOG_INFO << "Received a message [" << s << "]";
//     conn->Send(s);
// 
//     if (s == "quit" || s == "exit") {
//         conn->Close();
//     }
// }
// 
// 
// void OnConnection(const evpp::TCPConnPtr& conn) {
//     if (conn->IsConnected()) {
//         LOG_INFO << "Accept a new connection from " << conn->remote_addr();
//     } else {
//         LOG_INFO << "Disconnected from " << conn->remote_addr();
//     }
// }
// 
// 
// int main(int argc, char* argv[]) {
//     std::string port = "9099";
//     if (argc == 2) {
//         port = argv[1];
//     }
//     std::string addr = std::string("0.0.0.0:") + port;
//     evpp::EventLoop loop;
//     evpp::TCPServer server(&loop, addr, "TCPEcho", 2);
//     server.SetMessageCallback(&OnMessage);
//     server.SetConnectionCallback(&OnConnection);
//     server.Init();
//     server.Start();
//     loop.Run();
//     return 0;
// }
// 


int main( void )
{
    printf("Inside the C main function\n");
    // printf("Free Heap: %u bytes\n", xPortGetFreeHeapSize());
    // printf("Min Ever Free Heap: %u bytes\n", xPortGetMinimumEverFreeHeapSize());
    /*test_print();*/
    /*app_main();*/

    // // Create event group
    // eventGroup = xEventGroupCreate();

    // // Create queue for event data
    // eventQueue = xQueueCreate(10, sizeof(uint32_t));

    // // Create tasks
    // xTaskCreate(eventLoopTask, "EventLoop", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    // xTaskCreate(CouldBePipeWatcher, "OtherTask", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);

    main_tcp_echo_client_tasks();
    // app_main();
    // TestTask task;
    // task.start();
    // Start the scheduler
    vTaskStartScheduler();

    // Should never reach here
    for (;;);

    /*main_tcp_echo_client_tasks();*/
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

#ifdef __cplusplus
}
#endif

