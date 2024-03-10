/*
    TODO: handle case of no more patients or sales reps allowed
*/

#define COLOR_RED     "\x1b[31m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_RESET   "\x1b[0m"

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
int num_served_patients = 0;
int num_served_salesreps = 0;

int num_waiting_reporters = 0;
int num_waiting_patients = 0;
int num_waiting_salesreps = 0;

int doc_done = 0; 
int rep_done = 0;
int pat_done = 0;
int srep_done = 0;
int asst_done = 0;

int doc_session_done = 0;

pthread_mutex_t doctor_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t patient_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t salesrep_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t reporter_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t asst_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t doctor_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t patient_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t salesrep_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t reporter_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t asst_cond = PTHREAD_COND_INITIALIZER;
pthread_barrier_t barrier;

pthread_attr_t attr;

char* timestr; 
char* timestr2; 

// struct to hold visitor information
typedef struct {
    int id;
    int duration;
} vinfo;

vinfo visitor_infos[100];
long num_visitors = 0;

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
        if(hours == 12)
            sprintf(timestr, "%02d:%02dpm", hours, minutes);
        else
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


        if(doc_session_done == 1){
            get_time(timestr, current_time);
            printf(COLOR_RED "[%s] Doctor leaves\n" COLOR_RESET, timestr);
            pthread_exit(NULL);
        }
        
        get_time(timestr, current_time);
        printf(COLOR_RED "[%s] Doctor has next visitor\n" COLOR_RESET, timestr);

        pthread_barrier_wait(&barrier);
    }
}

void* patient(void* parg){
    // wait to be woken up by assistant (main thread)
    int duration = visitor_infos[(long)parg].duration;
    int id = visitor_infos[(long)parg].id;

    // printf("\t\tPatient %d has duration %d\n", id, duration);
    pthread_mutex_lock(&patient_mutex);

    asst_done = 1;
    pthread_cond_signal(&asst_cond);

    while(pat_done == 0){
        pthread_cond_wait(&patient_cond, &patient_mutex);
    }
    pat_done = 0;


    char* start_time = (char*)malloc(10);
    get_time(start_time, current_time);
    char* end_time = (char*)malloc(10);
    get_time(end_time, current_time + duration);

    printf(COLOR_BLUE "[%s - %s] Patient %d is in doctor's chamber\n" COLOR_RESET, start_time, end_time, id);

    current_time += duration;

    pthread_mutex_unlock(&patient_mutex);

    pthread_barrier_wait(&barrier);

    pthread_exit(NULL);
}

void* salesrep(void* sarg){
    // wait to be woken up by assistant (main thread)
    int duration = visitor_infos[(long)sarg].duration;
    int id = visitor_infos[(long)sarg].id;

    // printf("\t\tSalesrep %d has duration %d\n", id, duration);

    pthread_mutex_lock(&salesrep_mutex);

    asst_done = 1;
    pthread_cond_signal(&asst_cond);

    while(srep_done == 0){
        pthread_cond_wait(&salesrep_cond, &salesrep_mutex);
    }
    srep_done = 0;


    char* start_time = (char*)malloc(10);
    get_time(start_time, current_time);
    char* end_time = (char*)malloc(10);
    get_time(end_time, current_time + duration);

    printf(COLOR_BLUE "[%s - %s] Salesrep %d is in doctor's chamber\n" COLOR_RESET, start_time, end_time, id);

    current_time += duration;

    pthread_mutex_unlock(&salesrep_mutex);


    pthread_barrier_wait(&barrier);

    pthread_exit(NULL);
}

void* reporter(void* rarg){
    // wait to be woken up by assistant (main thread)
    int duration = visitor_infos[(long)rarg].duration;
    int id = visitor_infos[(long)rarg].id;

    // printf("\t\tReporter %d has duration %d\n", id, duration);

    pthread_mutex_lock(&reporter_mutex);

    asst_done = 1;
    pthread_cond_signal(&asst_cond);

    while(rep_done == 0){
        pthread_cond_wait(&reporter_cond, &reporter_mutex);
    }
    rep_done = 0;


    char* start_time = (char*)malloc(10);
    get_time(start_time, current_time);
    char* end_time = (char*)malloc(10);
    get_time(end_time, current_time + duration);

    printf(COLOR_BLUE "[%s - %s] Reporter %d is in doctor's chamber\n" COLOR_RESET, start_time, end_time, id);

    current_time += duration;

    pthread_mutex_unlock(&reporter_mutex);


    pthread_barrier_wait(&barrier);

    pthread_exit(NULL);
}


int main() {
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    // initialize the event queue
    eventQ E = initEQ("arrival.txt");

    timestr = (char*)malloc(10);
    timestr2 = (char*)malloc(10);

    pthread_barrier_init(&barrier, NULL, 3);

    // create doctor thread
    pthread_t doctor_thread;
    if(pthread_create(&doctor_thread, &attr, doctor, NULL) != 0){
        fprintf(stderr, "Error creating doctor thread\n");
        exit(1);
    }

    int doc_arrived = 0;

    while(!emptyQ(E)){
        // printf("Served patients: %d, Served salesreps: %d\n", num_served_patients, num_served_salesreps);
        

        if (num_served_patients == MAX_PATIENTS && num_served_salesreps == MAX_SALESREPS) {
            // wait for all visitors to be served
            get_time(timestr, current_time);
            num_served_patients++; 
            num_served_salesreps++;
            // wake up doctor
            pthread_mutex_lock(&doctor_mutex);
            doc_session_done = 1;
            doc_done = 1;
            pthread_cond_signal(&doctor_cond);
            pthread_mutex_unlock(&doctor_mutex);
            pthread_join(doctor_thread, NULL);
            continue;
        }

        event next = nextevent(E);

        vinfo nextvinfo;
        nextvinfo.duration = next.duration;

        if(next.time > current_time){
            doc_arrived = 1;
        }

        if(doc_arrived == 0 || doc_arrived == 2) {
            if(next.type == 'R'){
                if(num_served_patients >= MAX_PATIENTS && num_served_salesreps >= MAX_SALESREPS){
                    num_reporters++;
                    get_time(timestr, next.time);
                    printf("[%s] Reporter %d arrives\n", timestr, num_reporters);
                    printf("[%s] Reporter %d leaves (quota full)\n", timestr, num_reporters);
                    E = delevent(E);
                    continue;
                }

                pthread_t report_thread;
                nextvinfo.id = num_reporters+1;
                visitor_infos[num_visitors] = nextvinfo;
                // printf("visitor_infos[%ld].id = %d, duration = %d\n", num_visitors, visitor_infos[num_visitors].id, visitor_infos[num_visitors].duration);
                if(pthread_create(&report_thread, NULL, reporter, (void*)(num_visitors))){
                    fprintf(stderr, "Error creating report thread\n");
                    exit(1);
                }

                // block the main thread using asst_mutex until the reporter thread is entered
                pthread_mutex_lock(&asst_mutex);
                while (asst_done == 0) {
                    pthread_cond_wait(&asst_cond, &asst_mutex);
                }
                asst_done = 0;
                pthread_mutex_unlock(&asst_mutex);

                num_waiting_reporters++;
                num_reporters++;

                get_time(timestr, next.time);
                printf("\t[%s] Reporter %d arrives\n", timestr, num_reporters);
            } 
            else if(next.type == 'P'){
                if(num_patients >= MAX_PATIENTS){
                    num_patients++;
                    get_time(timestr, next.time);
                    printf("[%s] Patient %d arrives\n", timestr, num_patients);
                    printf("[%s] Patient %d leaves (quota full)\n", timestr, num_patients);
                    E = delevent(E);
                    continue;
                }

                pthread_t patient_thread;
                nextvinfo.id = num_patients+1;
                visitor_infos[num_visitors] = nextvinfo;
                if(pthread_create(&patient_thread, NULL, patient, (void*)(num_visitors))){
                    fprintf(stderr, "Error creating patient thread\n");
                    exit(1);
                }
                pthread_mutex_lock(&asst_mutex);
                while (asst_done == 0) {
                    pthread_cond_wait(&asst_cond, &asst_mutex);
                }
                asst_done = 0;
                pthread_mutex_unlock(&asst_mutex);

                num_waiting_patients++;
                num_patients++;

                get_time(timestr, next.time);
                printf("\t[%s] Patient %d arrives\n", timestr, num_patients);
            } 
            else if(next.type == 'S'){
                if (num_salesreps >= MAX_SALESREPS) {
                    num_salesreps++;
                    get_time(timestr, next.time);
                    printf("[%s] Salesrep %d arrives\n", timestr, num_salesreps);
                    printf("[%s] Salesrep %d leaves (quota full)\n", timestr, num_salesreps);
                    E = delevent(E);
                    continue;
                }

                pthread_t salesrep_thread;
                nextvinfo.id = num_salesreps+1;
                visitor_infos[num_visitors] = nextvinfo;
                if(pthread_create(&salesrep_thread, NULL, salesrep, (void*)(num_visitors))){
                    fprintf(stderr, "Error creating salesrep thread\n");
                    exit(1);
                }
                pthread_mutex_lock(&asst_mutex);
                while (asst_done == 0) {
                    pthread_cond_wait(&asst_cond, &asst_mutex);
                }
                asst_done = 0;
                pthread_mutex_unlock(&asst_mutex);

                num_waiting_salesreps++;
                num_salesreps++;

                get_time(timestr, next.time);
                printf("\t[%s] Salesrep %d arrives\n", timestr, num_salesreps);
            }

            if(doc_arrived == 2) {
                get_time(timestr, current_time);
                // printf("[%s] [%s] waiting people: %d %d %d\n", timestr, timestr, num_waiting_reporters, num_waiting_patients, num_waiting_salesreps);

                if(current_time > next.time) {
                    // it is possible that multiple people arrive when doctor is not available
                    // printf("skipping time\n");
                    // printf("waiting people: %d %d %d\n", num_waiting_reporters, num_waiting_patients, num_waiting_salesreps);
                    doc_arrived = 0;
                    E = delevent(E);
                    continue;
                }

                if(num_waiting_reporters > 0 || num_waiting_patients > 0 || num_waiting_salesreps > 0){
                    pthread_mutex_lock(&doctor_mutex);
                    doc_done = 1;
                    pthread_cond_signal(&doctor_cond);
                    pthread_mutex_unlock(&doctor_mutex);
                }

                if(num_waiting_reporters > 0) {
                    // reporter has priority 
                    // printf("2 [%s] waiting people: %d %d %d, reporter has priority\n", timestr, num_waiting_reporters, num_waiting_patients, num_waiting_salesreps);
                    pthread_mutex_lock(&reporter_mutex);
                    rep_done = 1;
                    num_waiting_reporters--;
                    pthread_cond_signal(&reporter_cond);
                    pthread_mutex_unlock(&reporter_mutex);
                    pthread_barrier_wait(&barrier);
                } 
                else if(num_waiting_patients > 0) {
                    // patient has priority
                    // printf("2 [%s] waiting people: %d %d %d, patient has priority\n", timestr, num_waiting_reporters, num_waiting_patients, num_waiting_salesreps);
                    pthread_mutex_lock(&patient_mutex);
                    pat_done = 1;
                    num_waiting_patients--;
                    num_served_patients++;
                    pthread_cond_signal(&patient_cond);
                    pthread_mutex_unlock(&patient_mutex);
                    pthread_barrier_wait(&barrier);
                } 
                else if(num_waiting_salesreps > 0) {
                    // printf("2 [%s] waiting people: %d %d %d, salesrep has priority\n", timestr, num_waiting_reporters, num_waiting_patients, num_waiting_salesreps);
                    pthread_mutex_lock(&salesrep_mutex);
                    srep_done = 1;
                    num_waiting_salesreps--;
                    num_served_salesreps++;
                    pthread_cond_signal(&salesrep_cond);
                    pthread_mutex_unlock(&salesrep_mutex);
                    pthread_barrier_wait(&barrier);
                }

            }

            num_visitors++;
            if(next.time > current_time){
                current_time = next.time;
            }
            E = delevent(E);

        }

        if(doc_arrived == 1){
            // printf("waiting people: %d %d %d\n", num_waiting_reporters, num_waiting_patients, num_waiting_salesreps);
            // wait for previous visitors to be served
            while(current_time < next.time && (num_waiting_reporters > 0 || num_waiting_patients > 0 || num_waiting_salesreps > 0)){
                pthread_mutex_lock(&doctor_mutex);
                doc_done = 1;
                pthread_cond_signal(&doctor_cond);
                pthread_mutex_unlock(&doctor_mutex);

                if(num_waiting_reporters > 0) {
                    // reporter has priority 
                    // printf("1 [%s] waiting people: %d %d %d, reporter has priority\n", timestr, num_waiting_reporters, num_waiting_patients, num_waiting_salesreps);
                    pthread_mutex_lock(&reporter_mutex);
                    rep_done = 1;
                    num_waiting_reporters--;
                    pthread_cond_signal(&reporter_cond);
                    pthread_mutex_unlock(&reporter_mutex);
                } 
                else if(num_waiting_patients > 0) {
                    // patient has priority
                    // printf("1 [%s] waiting people: %d %d %d, patient has priority\n", timestr, num_waiting_reporters, num_waiting_patients, num_waiting_salesreps);
                    pthread_mutex_lock(&patient_mutex);
                    pat_done = 1;
                    num_waiting_patients--;
                    num_served_patients++;
                    pthread_cond_signal(&patient_cond);
                    pthread_mutex_unlock(&patient_mutex);
                } 
                else if(num_waiting_salesreps > 0) {
                    // printf("1 [%s] waiting people: %d %d %d, salesrep has priority\n", timestr, num_waiting_reporters, num_waiting_patients, num_waiting_salesreps);
                    pthread_mutex_lock(&salesrep_mutex);
                    srep_done = 1;
                    num_waiting_salesreps--;
                    num_served_salesreps++;
                    pthread_cond_signal(&salesrep_cond);
                    pthread_mutex_unlock(&salesrep_mutex);
                }

                pthread_barrier_wait(&barrier);
            }

            if(current_time < next.time){
                current_time = next.time;
            }

            doc_arrived = 2;
        }


    }

    // broken out of while loop, no more events
    // wait for all visitors to be served
    pthread_mutex_lock(&doctor_mutex);
    doc_done = 1;
    pthread_cond_signal(&doctor_cond);
    pthread_mutex_unlock(&doctor_mutex);


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