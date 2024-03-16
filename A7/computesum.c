#include<stdio.h> 
#include<stdlib.h>
#include<unistd.h>
#include<foothread.h> 

int n; 
int P[100];
int num_children[100];
int sum[100];
int root_node;
foothread_mutex_t csmtx;
foothread_barrier_t barrier[100];

int node(void* arg) {
    int i = (int)arg;

    if(num_children[i] == 0) {
        foothread_barrier_wait(&barrier[i]);

        foothread_mutex_lock(&csmtx);
        printf("Leaf node  %d :: Enter a positive integer: ", i);
        scanf("%d", &sum[i]);
        foothread_mutex_unlock(&csmtx);
    } 
    
    else {

        foothread_barrier_wait(&barrier[i]);

        foothread_mutex_lock(&csmtx);
        for(int j = 0; j < n; j++) {
            if(P[j] == i) {
                // printf("Adding sum[%d] to sum[%d]\n", j, i);
                sum[i] += sum[j];
            }
        }

        printf("Internal node  %d gets the partial sum %d from its children\n", i, sum[i]);
        if(i == root_node) {
            printf("Sum at root (node %d) = %d\n", i, sum[i]);
        }

        foothread_mutex_unlock(&csmtx);
    }

    foothread_barrier_wait(&barrier[P[i]]);

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
        if(a==b) {root_node = a;continue;}
        num_children[b]++;
    }
    
    foothread_t threads[n];
    foothread_attr_t attr = FOOTHREAD_ATTR_INITIALIZER;
    foothread_attr_setjointype(&attr, FOOTHREAD_JOINABLE);
    
    foothread_mutex_init(&csmtx);

    // 1 barrier is made for each node for it to wait for all its children to finish
    for(int i = 0; i < n; i++) {
        foothread_barrier_init(&barrier[i], num_children[i]+1);
    }

    // create n threads for each node
    for(int i = 0; i < n; i++) {
        foothread_create(&threads[i], &attr, node, (void*)i);
    }

    foothread_exit();

    foothread_mutex_destroy(&csmtx);
    for(int i = 0; i < n; i++) {
        foothread_barrier_destroy(&barrier[i]);
    }

    printf("Exiting from main\n");

    return 0;
}