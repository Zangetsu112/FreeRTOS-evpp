// #ifdef __cplusplus
// extern "C" {
// #endif

// void vAssertCalled(void);

// #ifdef __cplusplus
// }

#include <cstddef>
// #include "FreeRTOS.h"
// #include "FreeRTOSConfig.h"
extern "C" void vAssertCalled( void );
extern "C" void vLoggingPrintf( const char * pcFormatString,
                            ... );
void* operator new(size_t size);
void* operator new[](size_t size);
void operator delete(void* ptr) noexcept;
void operator delete[](void* ptr) noexcept;
void operator delete(void* ptr, unsigned int size) noexcept;
void operator delete[](void* ptr, unsigned int size) noexcept;