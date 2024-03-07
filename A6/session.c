/*
    Doctor does not wait on counting semaphore, but on a mutex + condition variable
    Issue - only same thread can unlock a locked mutex 
    wait MUST happen before signal, otherwise if wait comes later, signal is lost 
    would have been stored if it was a counting semaphore
    may use barriers to solve this issue

    Another issue - We assume that a visitor or sales rep comes in the form of a process. But here, 
    main thread - doctor's assistant - will manage arrivals sequentially. will send wake up to thread - equivalent to person entering
    how can i use barrier along with mutex and cond to create a counting semaphore, in this case
*/

#ifndef PTHREAD_BARRIER_H_
#define PTHREAD_BARRIER_H_

#include <pthread.h>
#include <errno.h>

typedef int pthread_barrierattr_t;
typedef struct
{
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int count;
    int tripCount;
} pthread_barrier_t;


int pthread_barrier_init(pthread_barrier_t *barrier, const pthread_barrierattr_t *attr, unsigned int count)
{
    if(count == 0)
    {
        errno = EINVAL;
        return -1;
    }
    if(pthread_mutex_init(&barrier->mutex, 0) < 0)
    {
        return -1;
    }
    if(pthread_cond_init(&barrier->cond, 0) < 0)
    {
        pthread_mutex_destroy(&barrier->mutex);
        return -1;
    }
    barrier->tripCount = count;
    barrier->count = 0;

    return 0;
}

int pthread_barrier_destroy(pthread_barrier_t *barrier)
{
    pthread_cond_destroy(&barrier->cond);
    pthread_mutex_destroy(&barrier->mutex);
    return 0;
}

int pthread_barrier_wait(pthread_barrier_t *barrier)
{
    pthread_mutex_lock(&barrier->mutex);
    ++(barrier->count);
    if(barrier->count >= barrier->tripCount)
    {
        barrier->count = 0;
        pthread_cond_broadcast(&barrier->cond);
        pthread_mutex_unlock(&barrier->mutex);
        return 1;
    }
    else
    {
        pthread_cond_wait(&barrier->cond, &(barrier->mutex));
        pthread_mutex_unlock(&barrier->mutex);
        return 0;
    }
}

#endif // PTHREAD_BARRIER_H_

#include <stdio.h> 
#include <stdlib.h> 
#include <pthread.h> 
#include <event.h>

#define MAX_PATIENTS 25
#define MAX_SALESREPS 3

int current_time = 0;
int num_reporters = 0;
int num_patients = 0;
int num_salesreps = 0;

int num_waiting_reporters = 0;
int num_waiting_patients = 0;
int num_waiting_salesreps = 0;

int doc_done = 0; 
int rep_done = 0;
int pat_done = 0;
int srep_done = 0;

pthread_mutex_t doctor_mutex, patient_mutex, salesrep_mutex, reporter_mutex;
pthread_cond_t doctor_cond, patient_cond, salesrep_cond, reporter_cond;
pthread_barrier_t barrier;

pthread_attr_t attr;

char* timestr; 

// struct to hold visitor information
typedef struct {
    int id;
    int duration;
} vinfo;

void get_time(char* timestr, int current_time){
    // taking base time as 9:00 am, return current time s
    int hours, minutes;
    if(current_time < 0){
        hours = 8 - (-current_time) / 60;
        minutes = 60 - (-current_time) % 60;
    } else {
        hours = 9 + current_time / 60;
        minutes = current_time % 60;
    }

    if(hours > 12){
        hours -= 12;
        sprintf(timestr, "%02d:%02dpm", hours, minutes);
    } else {
        sprintf(timestr, "%02d:%02dam", hours, minutes);
    }
}

void* doctor(void* darg){
    while(1){
        pthread_mutex_lock(&doctor_mutex);

        while(doc_done == 0){
            pthread_cond_wait(&doctor_cond, &doctor_mutex);
        }
        doc_done = 0;
        pthread_mutex_unlock(&doctor_mutex);

        get_time(timestr, current_time);
        printf("[%s] Doctor has next visitor\n", timestr);

        // todo: handle end of serving also - unlock mutex, exit
    }
}

void* patient(void* parg){
    // wait to be woken up by assistant (main thread)
    pthread_mutex_lock(&patient_mutex);

    num_waiting_patients--;

    int duration = ((vinfo*)parg)->duration;
    int id = ((vinfo*)parg)->id;

    char* start_time = (char*)malloc(10);
    get_time(start_time, current_time);
    char* end_time = (char*)malloc(10);
    get_time(end_time, current_time + duration);

    current_time += duration;

    printf("[%s - %s] Patient %d is being served\n", start_time, end_time, id);

    pthread_cond_wait(&patient_cond, &patient_mutex);
    pthread_barrier_wait(&barrier);

    pthread_exit(NULL);
}

void* salesrep(void* sarg){
    // wait to be woken up by assistant (main thread)
    pthread_mutex_lock(&salesrep_mutex);

    num_waiting_salesreps--;

    int duration = ((vinfo*)sarg)->duration;
    int id = ((vinfo*)sarg)->id;

    char* start_time = (char*)malloc(10);
    get_time(start_time, current_time);
    char* end_time = (char*)malloc(10);
    get_time(end_time, current_time + duration);

    printf("[%s - %s] Salesrep %d is being served\n", start_time, end_time, id);

    current_time += duration;

    pthread_cond_wait(&salesrep_cond, &salesrep_mutex);
    pthread_barrier_wait(&barrier);

    pthread_exit(NULL);
}

void* reporter(void* rarg){
    // wait to be woken up by assistant (main thread)
    pthread_mutex_lock(&reporter_mutex);

    num_waiting_reporters--;


    int duration = ((vinfo*)rarg)->duration;
    int id = ((vinfo*)rarg)->id;

    char* start_time = (char*)malloc(10);
    get_time(start_time, current_time);
    char* end_time = (char*)malloc(10);
    get_time(end_time, current_time + duration);

    printf("[%s - %s] Reporter %d is being served\n", start_time, end_time, id);

    current_time += duration;

    pthread_cond_wait(&reporter_cond, &reporter_mutex);
    pthread_barrier_wait(&barrier);


    pthread_exit(NULL);
}


int main() {
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    // initialize the event queue
    eventQ E = initEQ("arrival.txt");

    timestr = (char*)malloc(10);

    pthread_mutex_init(&doctor_mutex, NULL);
    pthread_mutex_trylock(&doctor_mutex);

    pthread_mutex_init(&patient_mutex, NULL);
    pthread_mutex_trylock(&patient_mutex);

    pthread_mutex_init(&salesrep_mutex, NULL);
    pthread_mutex_trylock(&salesrep_mutex);

    pthread_mutex_init(&reporter_mutex, NULL);
    pthread_mutex_trylock(&reporter_mutex);

    pthread_barrier_init(&barrier, NULL, 3);

    // create doctor thread
    pthread_t doctor_thread;
    if(pthread_create(&doctor_thread, NULL, doctor, NULL) != 0){
        fprintf(stderr, "Error creating doctor thread\n");
        exit(1);
    }

    while(1) {

        // printf("visitors: %d %d %d\n", num_reporters, num_patients, num_salesreps);

        if(num_patients >= MAX_PATIENTS || num_salesreps >= MAX_SALESREPS){
            break;
        }

        event next = nextevent(E);
        vinfo nextvinfo;
        nextvinfo.duration = next.duration;

        get_time(timestr, next.time);

        if(next.type == 'R'){
            pthread_t report_thread;
            nextvinfo.id = num_reporters+1;
            if(pthread_create(&report_thread, NULL, reporter, (void*)(&nextvinfo))){
                fprintf(stderr, "Error creating report thread\n");
                exit(1);
            }
            num_waiting_reporters++;
            num_reporters++;

            printf("\t[%s] Reporter %d arrives\n", timestr, num_reporters);
        } 
        else if(next.type == 'P'){
            pthread_t patient_thread;
            nextvinfo.id = num_patients+1;
            if(pthread_create(&patient_thread, NULL, patient, (void*)(&nextvinfo))){
                fprintf(stderr, "Error creating patient thread\n");
                exit(1);
            }
            num_waiting_patients++;
            num_patients++;

            printf("\t[%s] Patient %d arrives\n", timestr, num_patients);
        } 
        else if(next.type == 'S'){
            pthread_t salesrep_thread;
            nextvinfo.id = num_salesreps+1;
            if(pthread_create(&salesrep_thread, NULL, salesrep, (void*)(&nextvinfo))){
                fprintf(stderr, "Error creating salesrep thread\n");
                exit(1);
            }
            num_waiting_salesreps++;
            num_salesreps++;

            printf("\t[%s] Salesrep %d arrives\n", timestr, num_salesreps);
        }

        if (current_time > 0 || (current_time==0 && next.time == 0)) {

            printf("Waiting people: %d %d %d\n", num_waiting_reporters, num_waiting_patients, num_waiting_salesreps);

            pthread_mutex_lock(&doctor_mutex);


            if(num_waiting_reporters == 0){
                if(num_waiting_patients == 0){
                    if(num_waiting_salesreps == 0){
                        continue;
                    } else {
                        pthread_cond_signal(&salesrep_cond);
                        pthread_mutex_unlock(&salesrep_mutex);
                    }
                } else {
                    pthread_cond_signal(&patient_cond);
                    pthread_mutex_unlock(&patient_mutex);
                }
            } else {
                pthread_cond_signal(&reporter_cond);
                pthread_mutex_unlock(&reporter_mutex);
            }

            doc_done = 1;
            pthread_cond_signal(&doctor_cond);
            pthread_mutex_unlock(&doctor_mutex);

            pthread_barrier_wait(&barrier);

            }

        if(next.time > current_time){
            current_time = next.time;
        }

        E = delevent(E);
    }


    // wait for all threads to finish
    pthread_join(doctor_thread, NULL);
    pthread_mutex_destroy(&doctor_mutex);
    pthread_mutex_destroy(&patient_mutex);
    pthread_mutex_destroy(&salesrep_mutex);
    pthread_mutex_destroy(&reporter_mutex);
    pthread_barrier_destroy(&barrier);

    pthread_exit(NULL);
    return 0;
}