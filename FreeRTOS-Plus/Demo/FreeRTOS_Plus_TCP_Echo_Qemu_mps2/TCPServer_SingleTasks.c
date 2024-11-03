// /*
//  * FreeRTOS V202212.00
//  * Copyright (C) 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
//  *
//  * Permission is hereby granted, free of charge, to any person obtaining a copy of
//  * this software and associated documentation files (the "Software"), to deal in
//  * the Software without restriction, including without limitation the rights to
//  * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
//  * the Software, and to permit persons to whom the Software is furnished to do so,
//  * subject to the following conditions:
//  *
//  * The above copyright notice and this permission notice shall be included in all
//  * copies or substantial portions of the Software.
//  *
//  * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
//  * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
//  * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
//  * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
//  * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//  *
//  * https://www.FreeRTOS.org
//  * https://github.com/FreeRTOS
//  *
//  */

// /*
//  * A set of tasks are created that send TCP echo requests to the standard echo
//  * port (port 7) on the IP address set by the configECHO_SERVER_ADDR0 to
//  * configECHO_SERVER_ADDR3 constants, then wait for and verify the reply
//  * (another demo is available that demonstrates the reception being performed in
//  * a task other than that from with the request was made).
//  *
//  * See the following web page for essential demo usage and configuration
//  * details:
//  * https://www.FreeRTOS.org/FreeRTOS-Plus/FreeRTOS_Plus_TCP/examples_FreeRTOS_simulator.html
//  */

// /* Standard includes. */
// #include <stdint.h>
// #include <stdio.h>
// #include <stdlib.h>

// /* FreeRTOS includes. */
// #include "FreeRTOS.h"
// #include "task.h"
// #include "queue.h"
// #include "CMSIS/CMSDK_CM3.h"
// #include "CMSIS/core_cm3.h"

// /* FreeRTOS+TCP includes. */
// #include "FreeRTOS_IP.h"
// #include "FreeRTOS_Sockets.h"

// /* Exclude the whole file if FreeRTOSIPConfig.h is configured to use UDP only. */
// #if ( ipconfigUSE_TCP == 1 )

// /* The echo tasks create a socket, send out a number of echo requests, listen
//  * for the echo reply, then close the socket again before starting over.  This
//  * delay is used between each iteration to ensure the network does not get too
//  * congested. */
//     #define echoLOOP_DELAY                ( ( TickType_t ) 150 / portTICK_PERIOD_MS )

// /* The echo server is assumed to be on port 7, which is the standard echo
//  * protocol port. */
//     #define server_PORT                 ( 10000 )

// /* The size of the buffers is a multiple of the MSS - the length of the data
//  * sent is a pseudo random size between 20 and echoBUFFER_SIZES. */
//     #define serverBUFFER_SIZE_MULTIPLIER    ( 1 )
//     #define serverBUFFER_SIZES              ( ipconfigTCP_MSS * 1 )

// /* The number of instances of the echo client task to create. */
//     #define serverNUM_CLIENTS          ( 1 )

// /*-----------------------------------------------------------*/

// /*
//  * Uses a socket to send data to, then receive data from, the standard echo
//  * port number 7.
//  */
//     static void prvServerTask( void * pvParameters );


// /*-----------------------------------------------------------*/

// /* Rx and Tx time outs are used to ensure the sockets do not wait too long for
//  * missing data. */
//     static const TickType_t xSendTimeOut = pdMS_TO_TICKS( 2000 );

// /* Counters for each created task - for inspection only. */
//     static uint32_t ulTxRxCycles[ serverNUM_CLIENTS ] = { 0 },
//                     ulTxRxFailures[ serverNUM_CLIENTS ] = { 0 },
//                     ulConnections[ serverNUM_CLIENTS ] = { 0 };

// /* Rx and Tx buffers for each created task. */
//     static char cTxBuffers[ serverNUM_CLIENTS ][ serverBUFFER_SIZES ],
//                 cRxBuffers[ serverNUM_CLIENTS ][ serverBUFFER_SIZES ];

// /*-----------------------------------------------------------*/

//     void vStartTCPServerTasks_SingleTasks( uint16_t usTaskStackSize,
//                                                UBaseType_t uxTaskPriority )
//     {
//         // Initialize variables 
//         struct freertos_sockaddr xClient, xBindAddress;
//         Socket_t xListeningSocket, xConnectedSocket;
//         socklen_t xSize = sizeof(xClient);
//         static const TickType_t xReceiveTimeout = portMAX_DELAY;
//         const BaseType_t xBacklog = 20;

//         // Attempt to open a socket
//         xListeningSocket = FreeRTOS_socket(
//             FREERTOS_AF_INET4, 
//             FREERTOS_SOCK_STREAM, 
//             FREERTOS_IPPROTO_TCP
//         );

//         // Check if socket was created
//         configASSERT(xListeningSocket != FREERTOS_INVALID_SOCKET);

//         // Set socket timeout so accept will wait for connection
//         FreeRTOS_setsockopt (xListeningSocket, 0, FREERTOS_SO_RCVTIMEO, &xReceiveTimeout, sizeof(xReceiveTimeout));

//         // Set the listening port to be server_PORT
//         memset(&xBindAddress, 0, sizeof(xBindAddress));
//         xBindAddress.sin_port = (uint16_t) server_PORT;
//         xBindAddress.sin_port = FreeRTOS_htons(xBindAddress.sin_port);
//         xBindAddress.sin_family = FREERTOS_AF_INET;

//         // Bind socket to port 
//         FreeRTOS_bind(xListeningSocket, &xBindAddress, sizeof(xBindAddress));
//         printf("Bound the listening socket to address\n");

//         // Set the socket to listen
//         FreeRTOS_listen(xListeningSocket, xBacklog);

//         for ( ; ; ) {
//             // Wait for connections
//             xConnectedSocket = FreeRTOS_accept(xListeningSocket, &xClient, &xSize);
//             printf("Received a connection\n");
//             configASSERT(xConnectedSocket != FREERTOS_INVALID_SOCKET);

//             TaskHandle_t xHandle = NULL;
//             // Spawn a task to handle it
//             xTaskCreate(
//                 prvServerTask, 
//                 "ServerTask",
//                 usTaskStackSize, 
//                 (void *) xConnectedSocket,
//                 uxTaskPriority, 
//                 &xHandle 
//             );

//         FreeRTOS_closesocket(xConnectedSocket);
//         // vTaskDelete(xHandle);
//         }
//     }
// /*-----------------------------------------------------------*/

//     static void prvServerTask(void * pcParameters) {
//         // BaseType_t  xInstace = (BaseType_t) 
//         Socket_t xConnectedSocket = (Socket_t) pcParameters;
//         char * pvRecievedString = & ( cRxBuffers[0][0]);
        
//         for (;;) {
//             memset( (void * ) pvRecievedString, 0, serverBUFFER_SIZES);

//             int32_t lBytes = FreeRTOS_recv(xConnectedSocket, pvRecievedString, serverBUFFER_SIZES - 1, 0);

//             if (lBytes > 0) {
//                 pvRecievedString[lBytes] = '\0';
//                 printf("Recieved: %s\n", pvRecievedString);
//             } else if (lBytes == 0) {
//                 printf("Client Closed connection\n");  
//                 break;
//             } else {
//                 printf("Error\n");
//                 break;
//             }
//         }
//     }
    
    
// /*-----------------------------------------------------------*/

//     BaseType_t xAreSingleTaskTCPServerStillRunning( void )
//     {
//         static uint32_t ulLastEchoSocketCount[ serverNUM_CLIENTS ] = { 0 }, ulLastConnections[ serverNUM_CLIENTS ] = { 0 };
//         BaseType_t xReturn = pdPASS, x;

//         /* Return fail is the number of cycles does not increment between
//          * consecutive calls. */
//         for( x = 0; x < serverNUM_CLIENTS; x++ )
//         {
//             if( ulTxRxCycles[ x ] == ulLastEchoSocketCount[ x ] )
//             {
//                 xReturn = pdFAIL;
//             }
//             else
//             {
//                 ulLastEchoSocketCount[ x ] = ulTxRxCycles[ x ];
//             }

//             if( ulConnections[ x ] == ulLastConnections[ x ] )
//             {
//                 xReturn = pdFAIL;
//             }
//             else
//             {
//                 ulConnections[ x ] = ulLastConnections[ x ];
//             }
//         }

//         return xReturn;
//     }


//     #if ( ipconfigUSE_DHCP_HOOK != 0 )

//         #if ( ipconfigIPv4_BACKWARD_COMPATIBLE == 1 )
//             eDHCPCallbackAnswer_t xApplicationDHCPHook( eDHCPCallbackPhase_t eDHCPPhase,
//                                                         uint32_t ulIPAddress )
//             {
//                 ( void ) eDHCPPhase;
//                 ( void ) ulIPAddress;

//                 return eDHCPContinue;
//             }
//         #else /* ( ipconfigIPv4_BACKWARD_COMPATIBLE == 1 ) */
//             eDHCPCallbackAnswer_t xApplicationDHCPHook_Multi( eDHCPCallbackPhase_t eDHCPPhase,
//                                                               struct xNetworkEndPoint * pxEndPoint,
//                                                               IP_Address_t * pxIPAddress )
//             {
//                 ( void ) eDHCPPhase;
//                 ( void ) pxEndPoint;
//                 ( void ) pxIPAddress;

//                 return eDHCPContinue;
//             }
//         #endif /* ( ipconfigIPv4_BACKWARD_COMPATIBLE == 1 ) */

//     #endif /* if ( ipconfigUSE_DHCP_HOOK != 0 )*/

// #endif /* ipconfigUSE_TCP */


/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <limits.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* FreeRTOS+TCP includes. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#include "TCPServer_SingleTasks.h"

/* Remove the whole file if FreeRTOSIPConfig.h is set to exclude TCP. */
#if ( ipconfigUSE_TCP == 1 )

/* The maximum time to wait for a closing socket to close. */
    #define tcpechoSHUTDOWN_DELAY    ( pdMS_TO_TICKS( 5000 ) )

/* The standard echo port number. */
    #define tcpechoPORT_NUMBER       9999

/* If ipconfigUSE_TCP_WIN is 1 then the Tx sockets will use a buffer size set by
 * ipconfigTCP_TX_BUFFER_LENGTH, and the Tx window size will be
 * configECHO_SERVER_TX_WINDOW_SIZE times the buffer size.  Note
 * ipconfigTCP_TX_BUFFER_LENGTH is set in FreeRTOSIPConfig.h as it is a standard TCP/IP
 * stack constant, whereas configECHO_SERVER_TX_WINDOW_SIZE is set in
 * FreeRTOSConfig.h as it is a demo application constant. */
    #ifndef configECHO_SERVER_TX_WINDOW_SIZE
        #define configECHO_SERVER_TX_WINDOW_SIZE    2
    #endif

/* If ipconfigUSE_TCP_WIN is 1 then the Rx sockets will use a buffer size set by
 * ipconfigTCP_RX_BUFFER_LENGTH, and the Rx window size will be
 * configECHO_SERVER_RX_WINDOW_SIZE times the buffer size.  Note
 * ipconfigTCP_RX_BUFFER_LENGTH is set in FreeRTOSIPConfig.h as it is a standard TCP/IP
 * stack constant, whereas configECHO_SERVER_RX_WINDOW_SIZE is set in
 * FreeRTOSConfig.h as it is a demo application constant. */
    #ifndef configECHO_SERVER_RX_WINDOW_SIZE
        #define configECHO_SERVER_RX_WINDOW_SIZE    2
    #endif

    static void prvConnectionListeningTask( void * pvParameters );
    static void prvServerConnectionInstance( void * pvParameters );
/*-----------------------------------------------------------*/

/* Stores the stack size passed into vStartSimpleTCPServerTasks() so it can be
 * reused when the server listening task creates tasks to handle connections. */
    static uint16_t usUsedStackSize = 0;

/* Create task stack and buffers for use in the Listening and Server connection tasks */
    static StaticTask_t listenerTaskBuffer;
    static StackType_t listenerTaskStack[ configMINIMAL_STACK_SIZE ];

    static StaticTask_t echoServerTaskBuffer;
    static StackType_t echoServerTaskStack[ configMINIMAL_STACK_SIZE ];

/*-----------------------------------------------------------*/

    void vStartSimpleTCPServerTasks( uint16_t usStackSize,
                                     UBaseType_t uxPriority )
    {
        /* Create the TCP echo server. */
        xTaskCreateStatic( prvConnectionListeningTask,
                           "ServerListener",
                           configMINIMAL_STACK_SIZE, 
                           NULL,
                           uxPriority + 1,
                           listenerTaskStack,
                           &listenerTaskBuffer );

        // xTaskCreate( prvConnectionListeningTask,
        //                    "ServerListener",
        //                    configMINIMAL_STACK_SIZE, 
        //                    NULL,
        //                    uxPriority + 1,
        //                    NULL);

        /* Remember the requested stack size so it can be re-used by the server
         * listening task when it creates tasks to handle connections. */
        usUsedStackSize = usStackSize;
    }
/*-----------------------------------------------------------*/

static void prvConnectionListeningTask( void * pvParameters )
    {
        struct freertos_sockaddr xClient, xBindAddress;
        Socket_t xListeningSocket, xConnectedSocket;
        socklen_t xSize = sizeof( xClient );
        static const TickType_t xReceiveTimeOut = portMAX_DELAY;
        const BaseType_t xBacklog = 20;

        #if ( ipconfigUSE_TCP_WIN == 1 )
            WinProperties_t xWinProps;

            /* Fill in the buffer and window sizes that will be used by the socket. */
            xWinProps.lTxBufSize = ipconfigTCP_TX_BUFFER_LENGTH;
            xWinProps.lTxWinSize = configECHO_SERVER_TX_WINDOW_SIZE;
            xWinProps.lRxBufSize = ipconfigTCP_RX_BUFFER_LENGTH;
            xWinProps.lRxWinSize = configECHO_SERVER_RX_WINDOW_SIZE;
        #endif /* ipconfigUSE_TCP_WIN */

        /* Just to prevent compiler warnings. */
        ( void ) pvParameters;

        /* Attempt to open the socket. */
        xListeningSocket = FreeRTOS_socket( FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP );
        configASSERT( xListeningSocket != FREERTOS_INVALID_SOCKET );

        /* Set a time out so accept() will just wait for a connection. */
        FreeRTOS_setsockopt( xListeningSocket, 0, FREERTOS_SO_RCVTIMEO, &xReceiveTimeOut, sizeof( xReceiveTimeOut ) );

        // /* Set the window and buffer sizes. */
        #if ( ipconfigUSE_TCP_WIN == 1 )
        {
            FreeRTOS_setsockopt( xListeningSocket, 0, FREERTOS_SO_WIN_PROPERTIES, ( void * ) &xWinProps, sizeof( xWinProps ) );
        }
        #endif /* ipconfigUSE_TCP_WIN */

        /* Bind the socket to the port that the client task will send to, then
         * listen for incoming connections. */
        xBindAddress.sin_port = tcpechoPORT_NUMBER;
        xBindAddress.sin_port = FreeRTOS_htons( xBindAddress.sin_port );
        xBindAddress.sin_family = FREERTOS_AF_INET;
        FreeRTOS_bind( xListeningSocket, &xBindAddress, sizeof( xBindAddress ) );
        FreeRTOS_listen( xListeningSocket, xBacklog );

        for( ; ; )
        {
            /* Wait for a client to connect. */
            xConnectedSocket = FreeRTOS_accept( xListeningSocket, &xClient, &xSize );
            configASSERT( xConnectedSocket != FREERTOS_INVALID_SOCKET );

            /* Spawn a task to handle the connection. */
            xTaskCreateStatic( prvServerConnectionInstance,
                               "EchoServer",
                              configMINIMAL_STACK_SIZE,
                               ( void * ) xConnectedSocket,
                               tskIDLE_PRIORITY,
                               echoServerTaskStack,
                               &echoServerTaskBuffer );

            // xTaskCreate(prvServerConnectionInstance,
            //         "EchoServer",
            //         usUsedStackSize,
            //         (void *)xConnectedSocket,
            //         tskIDLE_PRIORITY,
            //         NULL);
        }
    }
/*-----------------------------------------------------------*/

    static void prvServerConnectionInstance( void * pvParameters )
    {
        int32_t lBytes, lSent, lTotalSent;
        Socket_t xConnectedSocket;
        static const TickType_t xReceiveTimeOut = pdMS_TO_TICKS( 5000 );
        static const TickType_t xSendTimeOut = pdMS_TO_TICKS( 5000 );
        TickType_t xTimeOnShutdown;
        uint8_t * pucRxBuffer;

        xConnectedSocket = ( Socket_t ) pvParameters;

        /* Attempt to create the buffer used to receive the string to be echoed
         * back.  This could be avoided using a zero copy interface that just returned
         * the same buffer. */
        pucRxBuffer = ( uint8_t * ) pvPortMalloc( ipconfigTCP_MSS );

        if( pucRxBuffer != NULL )
        {
            FreeRTOS_setsockopt( xConnectedSocket, 0, FREERTOS_SO_RCVTIMEO, &xReceiveTimeOut, sizeof( xReceiveTimeOut ) );
            FreeRTOS_setsockopt( xConnectedSocket, 0, FREERTOS_SO_SNDTIMEO, &xSendTimeOut, sizeof( xReceiveTimeOut ) );

            for( ; ; )
            {
                /* Zero out the receive array so there is NULL at the end of the string
                 * when it is printed out. */
                memset( pucRxBuffer, 0x00, ipconfigTCP_MSS );

                /* Receive data on the socket. */
                lBytes = FreeRTOS_recv( xConnectedSocket, pucRxBuffer, ipconfigTCP_MSS, 0 );
                printf("Received data size: %ld\n", lBytes);
                printf("Recieved data: ");
                puts(pucRxBuffer);
                /* If data was received, echo it back. */
                if( lBytes >= 0 )
                {
                    lSent = 0;
                    lTotalSent = 0;

                    /* Call send() until all the data has been sent. */
                    while( ( lSent >= 0 ) && ( lTotalSent < lBytes ) )
                    {
                        lSent = FreeRTOS_send( xConnectedSocket, pucRxBuffer, lBytes - lTotalSent, 0 );
                        lTotalSent += lSent;
                    }

                    if( lSent < 0 )
                    {
                        /* Socket closed? */
                        break;
                    }
                }
                else
                {
                    /* Socket closed? */
                    break;
                }
            }
        }

        /* Initiate a shutdown in case it has not already been initiated. */
        FreeRTOS_shutdown( xConnectedSocket, FREERTOS_SHUT_RDWR );

        /* Wait for the shutdown to take effect, indicated by FreeRTOS_recv()
         * returning an error. */
        xTimeOnShutdown = xTaskGetTickCount();

        do
        {
            if( FreeRTOS_recv( xConnectedSocket, pucRxBuffer, ipconfigTCP_MSS, 0 ) < 0 )
            {
                break;
            }
        } while( ( xTaskGetTickCount() - xTimeOnShutdown ) < tcpechoSHUTDOWN_DELAY );

        /* Finished with the socket, buffer, the task. */
        vPortFree( pucRxBuffer );
        FreeRTOS_closesocket( xConnectedSocket );

        vTaskDelete( NULL );
    }
/*-----------------------------------------------------------*/

    #if ( ipconfigUSE_DHCP_HOOK != 0 )

        #if ( ipconfigIPv4_BACKWARD_COMPATIBLE == 1 )
            eDHCPCallbackAnswer_t xApplicationDHCPHook( eDHCPCallbackPhase_t eDHCPPhase,
                                                        uint32_t ulIPAddress )
            {
                ( void ) eDHCPPhase;
                ( void ) ulIPAddress;

                return eDHCPContinue;
            }
        #else /* ( ipconfigIPv4_BACKWARD_COMPATIBLE == 1 ) */
            eDHCPCallbackAnswer_t xApplicationDHCPHook_Multi( eDHCPCallbackPhase_t eDHCPPhase,
                                                              struct xNetworkEndPoint * pxEndPoint,
                                                              IP_Address_t * pxIPAddress )
            {
                ( void ) eDHCPPhase;
                ( void ) pxEndPoint;
                ( void ) pxIPAddress;

                return eDHCPContinue;
            }
        #endif /* ( ipconfigIPv4_BACKWARD_COMPATIBLE == 1 ) */

    #endif /* if ( ipconfigUSE_DHCP_HOOK != 0 )*/
/* The whole file is excluded if TCP is not compiled in. */
#endif /* ipconfigUSE_TCP */