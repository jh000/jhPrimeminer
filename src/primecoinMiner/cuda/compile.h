/**
 * @file compile.h
 */
#ifndef __418_COMPILE_H__
#define __418_COMPILE_H__

#define true 1
#define false 0

#ifndef __CUDACC__ /* when compiling with g++ ... */

#define __device__
#define __host__

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

inline int max(int a, int b) { return (a > b) ? a : b; }
inline int min(int a, int b) { return (a < b) ? a : b; }
inline int abs(int a) { return (a < 0) ? -a : a; }

#else /* when compiling with nvcc ... */

#endif /* __CUDACC__ */

#endif /* __418_COMPILE_H__ */

