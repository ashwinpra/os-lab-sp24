#include<stdio.h> 
#include<stdlib.h>
#include<unistd.h>
#include<foothread.h> 

int n; 
int P[100];
int num_children[100];
int sum[100];
foothread_mutex_t csmtx;
foothread_barrier_t barrier[100];

int node(void* arg) {
    int i = (int)arg;

    if(num_children[i] == 0) {
        foothread_mutex_lock(&csmtx);
        printf("Leaf node \t%d (%d):: Enter a positive integer: ", i , gettid());
        scanf("%d", &sum[i]);
        foothread_mutex_unlock(&csmtx);
        
        foothread_barrier_wait(&barrier[i]);
    } 
    
    else {
        for(int j = 0; j < num_children[i]; j++) {
            printf("Waiting for barrier[%d] to open\n", i);
            foothread_barrier_wait(&barrier[i]);
        }

        foothread_mutex_lock(&csmtx);
        for(int j = 0; j < n; j++) {
            if(P[j] == i) {
                printf("Adding sum[%d] to sum[%d]\n", j, i);
                sum[i] += sum[j];
            }
        }
        printf("Non-leaf node \t%d (%d):: Sum of children: %d\n", i, gettid(), sum[i]);
        printf("\n");
        foothread_mutex_unlock(&csmtx);
    }

    foothread_exit();
}

int main() {
    FILE* fp = fopen("tree.txt", "r");

    fscanf(fp, "%d", &n);

    for(int i=0;i<n;i++) {num_children[i] = 0; sum[i] = 0; P[i] = -1;}
    
    for(int i = 0; i < n; i++) {
        int a, b;
        fscanf(fp, "%d %d", &a, &b);
        P[a] = b;
        num_children[b]++;
    }

    for(int i=0;i<n;i++) {printf("P[%d] = %d\n", i, P[i]);}

    // for(int i = 0; i < n; i++) {
    //     printf("Node %d has %d children\n", i, num_children[i]);
    // }

    foothread_t threads[n];
    foothread_attr_t attr = FOOTHREAD_ATTR_INITIALIZER;
    
    foothread_mutex_init(&csmtx);

    // printf("Mutex: semid = %d\n", csmtx.semid);

    // 1 barrier is made for each node for it to wait for all its children to finish
    for(int i = 0; i < n; i++) {
        foothread_barrier_init(&barrier[i], num_children[i]+1);
    }

    for (int i = 0; i < n; i++) {
        printf("barrier[%d] = %d, %d\n", i, barrier[i].tripCount, barrier[i].semid);
    }

    // create n threads for each node
    for(int i = 0; i < n; i++) {
        foothread_create(&threads[i], &attr, node, (void*)i);
    }

    // printf("ALL THREADS CREATED\n");

    // printf("Main fn: tid = %d\n", gettid());

    foothread_exit();

    foothread_mutex_destroy(&csmtx);
    for(int i = 0; i < n; i++) {
        foothread_barrier_destroy(&barrier[i]);
    }

    return 0;
}