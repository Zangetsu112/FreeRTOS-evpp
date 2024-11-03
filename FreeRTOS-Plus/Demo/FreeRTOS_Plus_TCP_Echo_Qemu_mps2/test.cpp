#include <stdio.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <event_groups.h>
#include "test.h"

//  NOTE: Function works. Compilation directives: PASS SAME PARAMS AS FREERTOS_C
//  TODO: Write it into the Makefile -- Doesnt work


/*class HelloWorldTask {*/
/*private:*/
/*    static void taskWrapper(void* pvParameters) {*/
/*        HelloWorldTask* instance = static_cast<HelloWorldTask*> (pvParameters);*/
/*        instance -> run();*/
/*    }*/
/**/
/*    void run() {*/
/*        for(;;) {*/
/*            printf("Hello, World from the C++ program\n");*/
/*            vTaskDelay(1000 / portTICK_PERIOD_MS);*/
/*        }*/
/*    }*/
/**/
/*public:*/
/*    void start() {*/
/*        xTaskCreate(taskWrapper, "HelloWorldTask", 2048, this, 1, NULL);*/
/*    }*/
/**/
/*};*/
/**/
/**/
/*extern "C" void app_main(void) {*/
/*    HelloWorldTask task;*/
/*    task.start();*/
/*}*/
extern "C" void test_print(void) {
    printf("Hello, World from the C++ prorgam just to be sure\n");
}
