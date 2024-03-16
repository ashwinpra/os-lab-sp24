#define _GNU_SOURCE
#include<sched.h>
#include<unistd.h>
#include<sys/sem.h>
#include<stdio.h> 
#include<stdlib.h> 
#include<foothread.h>

static struct sembuf vop = {0, 1, 0};
static struct sembuf pop = {0, -1, 0};

int exit_sem = -1;
int mtx = -1;
int num_threads = 0;
int exited_threads = 0;

void foothread_attr_setjointype(foothread_attr_t *attr, int jointype) {
    attr->join_type = jointype;
}

void foothread_attr_setstacksize(foothread_attr_t *attr, int stacksize) {
    attr->stack_size = stacksize;
}

void foothread_create(foothread_t *thread, foothread_attr_t *attr, int (*fn)(void *), void *arg) {

    thread = (foothread_t*)malloc(sizeof(foothread_t));
    thread->id = gettid();
    thread->stack_size = FOOTHREAD_DEFAULT_STACK_SIZE;

    if (attr != NULL) {
        thread->stack_size = attr->stack_size;
    }

    if(mtx == -1) {
        mtx = semget(IPC_PRIVATE, 1, 0777 | IPC_CREAT);
        semctl(mtx, 0, SETVAL, 1);
    }

    if(exit_sem == -1) {
        exit_sem = semget(IPC_PRIVATE, 1, 0777 | IPC_CREAT);
        semctl(exit_sem, 0, SETVAL, 0);
    }

    P(mtx);
    if(attr->join_type == FOOTHREAD_JOINABLE) {
        num_threads++;
    }

    void* stack = malloc(thread->stack_size);
    clone(fn, stack + thread->stack_size , CLONE_PARENT | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | CLONE_VM | CLONE_THREAD | CLONE_SYSVSEM, arg);

    V(mtx);
}

void foothread_exit() {
    // printf("Thread %d exited\n", gettid());
    if(mtx == -1) {
        mtx = semget(IPC_PRIVATE, 1, 0777 | IPC_CREAT);
        semctl(mtx, 0, SETVAL, 1);
    }

    if(exit_sem == -1) {
        exit_sem = semget(IPC_PRIVATE, 1, 0777 | IPC_CREAT);
        semctl(exit_sem, 0, SETVAL, 0);
    }

    // wait for all threads to exit
    P(mtx);
    exited_threads++;
    if(exited_threads != num_threads) {
        V(mtx);
        P(exit_sem);
    } else {
        V(mtx);
        for(int i = 0; i < num_threads; i++) {
            V(exit_sem);
        }
    }
}

void foothread_mutex_init(foothread_mutex_t *mutex) {
    mutex->semid = semget(IPC_PRIVATE, 1, 0777 | IPC_CREAT);
    semctl(mutex->semid, 0, SETVAL,1);
    mutex->tid = 0;
}

void foothread_mutex_lock(foothread_mutex_t *mutex) {     
    // printf("Thread %d is trying to lock\n", gettid());
    P(mutex->semid);
    mutex->tid = gettid();
    // printf("Mutex locked by thread %d\n", mutex->tid);
}

void foothread_mutex_unlock(foothread_mutex_t *mutex) {
    if(mutex->tid != gettid()) {
        printf("Error: Mutex can only be unlocked by the same thread that locked it\n");
        return;
    }
    if(semctl(mutex->semid, 0, GETVAL) == 1) {
        printf("Error: Mutex is already unlocked\n");
        return;
    }

    // printf("Mutex unlocked by thread %d\n", mutex->tid);
    V(mutex->semid);
    mutex->tid = 0;
}

void foothread_mutex_destroy(foothread_mutex_t *mutex) {
    semctl(mutex->semid, 0, IPC_RMID);
}

void foothread_barrier_init(foothread_barrier_t *barrier, int count) {
    foothread_mutex_init(&barrier->mtx);
    barrier->semid = semget(IPC_PRIVATE, 1, 0777 | IPC_CREAT|0666);
    semctl(barrier->semid, 0, SETVAL, 0);

    barrier->tripCount = count;
    barrier->count = 0;
}

void foothread_barrier_wait(foothread_barrier_t *barrier) {
    foothread_mutex_lock(&barrier->mtx);
    barrier->count++;
    if(barrier->count == barrier->tripCount) {
        for(int i = 0; i < barrier->tripCount-1; i++) {
            V(barrier->semid);
        }

        barrier->count = 0;
        foothread_mutex_unlock(&barrier->mtx);
    }
    else {
        foothread_mutex_unlock(&barrier->mtx);
        P(barrier->semid);
    }
}

void foothread_barrier_destroy(foothread_barrier_t *barrier) {
    semctl(barrier->semid, 0, IPC_RMID);
}