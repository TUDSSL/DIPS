#ifndef PLATFORMS_PLATFORM_H_
#define PLATFORMS_PLATFORM_H_

#include "main.h"
#include "stdbool.h"

#define CONCAT_(a, b) a##b
#define CONCAT(a, b) CONCAT_(a, b)
#define CATSTR(A, B) A B

#if HWREV == 1
#include "int-debug-v1-0.h"
#else
#error "Please define a platform the hardware revision."
#endif

#endif /* PLATFORMS_PLATFORM_H_ */
