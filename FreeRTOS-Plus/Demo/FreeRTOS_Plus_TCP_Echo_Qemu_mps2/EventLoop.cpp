#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#include "EventLoop.h"
#include "FreeRTOSConfig.h"
#include <map>
#include <functional>


// Defined in the main.c file
void vAssertCalled(void);
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

void * operator new(size_t size) {
    return pvPortMalloc(size); 
} 

void * operator new[](size_t size) {
    return pvPortMalloc( size ); 
}

void operator delete(void * ptr) { 
    vPortFree( ptr ); 
} 

void operator delete[](void * ptr) {
    vPortFree( ptr ); 
}

// These functions are just to keep the compiler happy :(
void operator delete(void * ptr, unsigned int size) {
    (void) size;
    vPortFree( ptr );
}

void operator delete[](void * ptr, unsigned int size) {
    (void) size;
    vPortFree( ptr );
}

typedef void (*SocketCallback)(void*);
class EventLoop;

class PipeWatcher {
private:
    Socket_t xClientSocket;
    Socket_t xListeningSocket;

    bool createSocketPair() {
        struct freertos_sockaddr xBindAddress;
        const TickType_t xRxBlockTime_master = 0;
        // const TickType_t xTxBlockTime = 0;
        // BaseType_t xNonBlocking = pdTRUE;
        // socklen_t xSize = sizeof(xBindAddress);

        xBindAddress.sin_family = FREERTOS_AF_INET;
        xBindAddress.sin_port = FreeRTOS_htons(0);

        
        xListeningSocket = FreeRTOS_socket(FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP);
        if (xListeningSocket == FREERTOS_INVALID_SOCKET) return false;
        FreeRTOS_setsockopt(xListeningSocket, 0, FREERTOS_SO_RCVTIMEO, &xRxBlockTime_master, sizeof(TickType_t));

        if (FreeRTOS_bind(xListeningSocket, &xBindAddress, sizeof(xBindAddress)) != 0) {
            printf("Couldn't bind the listening socket \n");
            FreeRTOS_closesocket(xListeningSocket);
            return false;
        }

        FreeRTOS_GetLocalAddress(xListeningSocket, &xBindAddress);

        if (FreeRTOS_listen(xListeningSocket, 1) != 0) {
            printf("Couldn't listen on the listening socket \n");
            FreeRTOS_closesocket(xListeningSocket);
            return false;
        }

        // xClientSocket = FreeRTOS_socket(FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP);
        // if (xClientSocket == FREERTOS_INVALID_SOCKET) {
        //    printf("Client socket skadoosh\n");
        //     FreeRTOS_closesocket(xListeningSocket);
        //     return false;
        // }
        // // FreeRTOS_setsockopt(xClientSocket, 0, FREERTOS_SO_SNDTIMEO, &xTxBlockTime, sizeof(TickType_t));
        // Initiate a semaphore and block on that semaphore? 
        // FreeRTOS_setsockopt(xClientSocket, 0, FREERTOS_SO_SET_SEMAPHORE, &xNonBlocking, sizeof(BaseType_t));
        // FreeRTOS_connect(xClientSocket, &xBindAddress, sizeof(xBindAddress));
        return true;
    }

public:
    PipeWatcher() {
        if (!createSocketPair()) {
            printf("Could not initialize Pipe Watcher\n");
            configASSERT(false);
        }
    }  

    ~PipeWatcher() {
        // FreeRTOS_closesocket(xClientSocket);
        FreeRTOS_closesocket(xListeningSocket);
    }

    void Notify() {
        printf("Called Notify\n");
        // Send a signal to socket -- will cause Select to return an eSELECT_INTR
        if (FreeRTOS_SignalSocket(xListeningSocket) != 0) {
            printf("Couldn't send signal to socket -- check if it's active\n");
            configASSERT(false);
        }

        // uint8_t notifyByte = 0xFF;
        // if (FreeRTOS_send(xClientSocket, &notifyByte, 1, 0) != 1) {
        //     configASSERT(false);
        // }
    }

    Socket_t getServerSocket() const { return xListeningSocket; }
};

// struct SocketCallbackPair {
//     Socket_t socket;
//     SocketCallback callback;
// };

class EventLoop {
private:
    SocketSet_t xSocketSet;
    // SocketCallbackPair socket_callbacks[32];
    QueueHandle_t pending_functors;
    int socket_callback_count;
    std::map<Socket_t, std::function<void(void*)>> socket_callbacks;

    static void eventLoopTaskWrapper(void* pvParameters) {
        static_cast<EventLoop*>(pvParameters)->eventLoopTask();
    }

    void eventLoopTask() {
        while (1) {
            // Listen for the pipe watcher socket always
            FreeRTOS_FD_SET(pipe_watcher->getServerSocket(), xSocketSet, eSELECT_READ);

            BaseType_t xResult = FreeRTOS_select(xSocketSet, portMAX_DELAY);

            if (xResult == eSELECT_INTR) {
                    printf("Custom event received from PipeWatcher\n");
                    
                    // void (*functor)(void);
                    // while (xQueueReceive(pending_functors, &functor, 0) == pdTRUE) {
                    //     functor();
                    // }
            }

            else if (xResult > 0) {
                // This is if ClientSocket business would've worked:
                // if (FreeRTOS_FD_ISSET(pipe_watcher->getServerSocket(), xSocketSet)) {
                //     uint8_t receivedByte;
                //     FreeRTOS_recv(pipe_watcher->getServerSocket(), &receivedByte, 1, 0);
                    
                // }

                for(auto pairs: socket_callbacks) {
                    if (FreeRTOS_FD_ISSET(pairs.first, xSocketSet)) {
                        void* arg = nullptr;
                        pairs.second(arg);
                    }
                }

                // for (int i = 0; i < socket_callback_count; ++i) {
                //     if (socket_callbacks[i].socket != pipe_watcher->getServerSocket() && 
                //         FreeRTOS_FD_ISSET(socket_callbacks[i].socket, xSocketSet)) {
                //         socket_callbacks[i].callback(NULL);
                //     }
                // }
            }
        }
    }

public:
    PipeWatcher* pipe_watcher;
    EventLoop() : socket_callback_count(0) {
        xSocketSet = FreeRTOS_CreateSocketSet();
        pending_functors = xQueueCreate(10, sizeof(void (*)(void)));
        pipe_watcher = new PipeWatcher;        
    }

        void addSocketEvent(Socket_t socket, std::function<void(void*)> callback) {
            // if (socket_callback_count < 32) {
            //     socket_callbacks[socket_callback_count].socket = socket;
            //     socket_callbacks[socket_callback_count].callback = callback;
            //     socket_callback_count++;
            //     FreeRTOS_FD_SET(socket, xSocketSet, eSELECT_READ);
            // }
            FreeRTOS_FD_SET(socket, xSocketSet, eSELECT_READ);
            if (callback) {
                socket_callbacks[socket] = callback;
            }
        }

        void postFunctor(void (*functor)(void)) {
            xQueueSend(pending_functors, &functor, portMAX_DELAY);
            pipe_watcher->Notify();
        }

        void start() {
            xTaskCreate(eventLoopTaskWrapper, "EventLoop", configMINIMAL_STACK_SIZE, this, tskIDLE_PRIORITY, NULL);
        }

        ~EventLoop() {
            delete pipe_watcher;
            FreeRTOS_DeleteSocketSet(xSocketSet);
            vQueueDelete(pending_functors);
        }
    };


    class TestTask {
    private:
        static void taskWrapper(void* pvParameters) {
            TestTask* instance = static_cast<TestTask*> (pvParameters);
            instance -> run();
    }

    static void try_callback(void * none) {
        printf("Recieved data on socket\n");
    }

    void run() {
        printf("Before EventLoop\n");
        EventLoop eventLoop;
        
        // Test with a socket
        struct freertos_sockaddr xBindAddress;
        // Make socket non blocking
        const TickType_t xRxBlockTime_master = 0;
        xBindAddress.sin_family = FREERTOS_AF_INET;
        xBindAddress.sin_port = FreeRTOS_htons(9999);

        
        Socket_t socket = FreeRTOS_socket(FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP);
        // if (socket == FREERTOS_INVALID_SOCKET) return false;
        FreeRTOS_setsockopt(socket, 0, FREERTOS_SO_RCVTIMEO, &xRxBlockTime_master, sizeof(TickType_t));

        if (FreeRTOS_bind(socket, &xBindAddress, sizeof(xBindAddress)) != 0) {
            printf("Couldn't bind the listening socket \n");
            FreeRTOS_closesocket(socket);
            // return false;
        }

        FreeRTOS_GetLocalAddress(socket, &xBindAddress);

        if (FreeRTOS_listen(socket, 1) != 0) {
            printf("Couldn't listen on the listening socket \n");
            FreeRTOS_closesocket(socket);
            // return false;
        }

        eventLoop.addSocketEvent(socket, try_callback);



        eventLoop.start();
        // std::map<int, int> my_map;
        // my_map[1] = 2;
        // my_map[4] = 5;
        // printf("%d %d", my_map[1], my_map[4]);
        // my_map.erase(1);
        // auto it = my_map.find(1);
        // if (it == my_map.end()) 
        //     printf("1 removed from the map");
        printf("Started Eventloop\n");


        // Periodically trigger the PipeWatcher
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(5000));  // Every 5 seconds
            eventLoop.pipe_watcher->Notify();
            printf("Sent a Notify to pipewatcher\n");
        }
    }

public:
    void start() {
        xTaskCreate(taskWrapper, "TestTask", 2048, this, 1, NULL);
    }

};

extern "C" void app_main(void) {
    TestTask task;
    task.start();
}


