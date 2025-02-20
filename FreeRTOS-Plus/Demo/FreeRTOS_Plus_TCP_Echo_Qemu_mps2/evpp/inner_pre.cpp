#include "inner_pre.h"
#include "FreeRTOS.h"
#include "task.h"
#include <cstddef>

// Defined in the main.c file
// {
//     volatile unsigned long looping = 0;

//     taskENTER_CRITICAL();
//     {
//         /* Use the debugger to set ul to a non-zero value in order to step out
//          *      of this function to determine why it was called. */
//         while( looping == 0LU )
//         {
//             portNOP();
//         }
//     }
//     taskEXIT_CRITICAL();
// }

extern "C" void __ssputws_r() {
    return;
}

void * operator new(size_t size) {
    return pvPortMalloc(size); 
} 

void * operator new[](size_t size) {
    return pvPortMalloc( size ); 
}

void operator delete(void * ptr) noexcept { 
    vPortFree( ptr ); 
} 

void operator delete[](void * ptr) noexcept {
    vPortFree( ptr ); 
}

// These functions are just to keep the compiler happy :(
void operator delete(void * ptr, unsigned int size) noexcept {
    (void) size;
    vPortFree( ptr );
}

void operator delete[](void * ptr, unsigned int size) noexcept {
    (void) size;
    vPortFree( ptr );
}
