#ifndef _FOOTHREAD_H
#define _FOOTHREAD_H

#include <sys/sem.h>

#define FOOTHREAD_THREADS_MAX 10
#define FOOTHREAD_DEFAULT_STACK_SIZE 2097152 // (2MB)

#define FOOTHREAD_JOINABLE 100
#define FOOTHREAD_DETACHED 101

#define FOOTHREAD_ATTR_INITIALIZER { FOOTHREAD_DETACHED, FOOTHREAD_DEFAULT_STACK_SIZE }

#define P(s) semop(s, &pop, 1)
#define V(s) semop(s, &vop, 1)



typedef struct _foothread_t {
    int id;
    int stack_size;
} foothread_t;

typedef struct _foothread_attr_t {
    int join_type;
    int stack_size;
} foothread_attr_t;

typedef struct _foothread_mutex_t {
    int semid;
    int tid;
} foothread_mutex_t;

// typedef struct _foothread_barrier_t {
//     int semid;
//     int count;
//     int count_max;
// } foothread_barrier_t;

typedef struct {
    foothread_mutex_t mtx;   // Mutex for mutual exclusion
    int semid;            // Semaphore ID
    int tripCount; // Count of threads to synchronize
    int count;     // Counter to track arrived threads
} foothread_barrier_t;

void foothread_create(foothread_t*, foothread_attr_t*, int(*)(void*), void*);
void foothread_exit();

void foothread_attr_setjointype(foothread_attr_t*, int);
void foothread_attr_setstacksize(foothread_attr_t*, int);

void foothread_mutex_init(foothread_mutex_t*);
void foothread_mutex_lock(foothread_mutex_t*);
void foothread_mutex_unlock(foothread_mutex_t*);
void foothread_mutex_destroy(foothread_mutex_t*);

void foothread_barrier_init(foothread_barrier_t*, int);
void foothread_barrier_wait(foothread_barrier_t*);
void foothread_barrier_destroy(foothread_barrier_t*);

#endif