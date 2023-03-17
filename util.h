
#include <time.h>

#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cassert>
#include <limits.h>

#include <omp.h>

#define TENSORSIZE_T unsigned long long int

typedef struct
{
    char *name;
    double start;
    double end;
} timer;

timer *timer_start(const char *name);
void timer_end(timer *tm);

void print_trace(void);
void print_vec(int n, int *vec, const char *name);

static inline void *safe_malloc(size_t size)
{
    void *loc = malloc(size);
    if (loc == NULL)
    {
        puts("Memory allocation failed.\n");
        exit(1);
    }

    return loc;
}

static inline void *safe_calloc(size_t count, size_t size)
{
    void *loc = calloc(count, size);
    if (loc == NULL)
    {
        puts("Memory (c)allocation failed.\n");
        exit(1);
    }

    return loc;
}