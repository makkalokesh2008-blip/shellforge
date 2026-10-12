#ifndef NB_COMMON_H
#define NB_COMMON_H

#include <stdint.h>
#include <stddef.h>

#define NELEM(x) (sizeof(x) / sizeof((x)[0]))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

#endif
