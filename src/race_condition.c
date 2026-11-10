#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <string.h>
#include "race_condition.h"

atomic_int counter = 0;

pthread_barrier_t read_barrier;
pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t trace_mutex = PTHREAD_MUTEX_INITIALIZER;

int event_number = 0;

// print each event in order without interleaved output
void print_event(int id, const char *action, int value){
    pthread_mutex_lock(&trace_mutex);

    event_number++;
    printf("Event %d: Worker %d %s %d\n",
           event_number, id, action, value);
    pthread_mutex_unlock(&trace_mutex);
}

// forced incorrect increment
void *forced_worker(void *arg){
    int id = *(int *)arg;

    // each worker reads the shared counter into its private variable
    int private_value = atomic_load_explicit(&counter, memory_order_relaxed);
    print_event(id, "READ private counter =", private_value);

    // both workers must finish reading before either can write
    pthread_barrier_wait(&read_barrier);

    // local addition
    private_value++;

    // write the private value back to the shared counter
    atomic_store_explicit(&counter, private_value, memory_order_relaxed);
    print_event(id, "WRITE private counter =", private_value);

    return NULL;
}

// repaired increment
void *repaired_worker(void *arg){
    int id = *(int *)arg;

    // protects the entire read-modify-write operation with shared mutex
    pthread_mutex_lock(&counter_mutex);

    int private_value = atomic_load_explicit(&counter, memory_order_relaxed);
    print_event(id, "READ private counter =", private_value);

    private_value++;

    atomic_store_explicit(&counter, private_value, memory_order_relaxed);
    print_event(id, "WRITE private counter =", private_value);
   
    //release the lock
    pthread_mutex_unlock(&counter_mutex);
    return NULL;
}

// create two workers and wait for both to finish
void run_workers(void *(*worker_function)(void *)){
    pthread_t workers[2];
    int ids[2] = {1, 2};

    int result;

    result = pthread_create(&workers[0], NULL, worker_function, &ids[0]);
    if (result != 0) {
        printf("Error creating worker 1: %s\n", strerror(result));
        exit(EXIT_FAILURE);
    }

    result = pthread_create(&workers[1], NULL, worker_function, &ids[1]);
    if (result != 0) {
        printf("Error creating worker 2: %s\n", strerror(result));
        exit(EXIT_FAILURE);
    }

    pthread_join(workers[0], NULL);
    pthread_join(workers[1], NULL);
}

void run_race_condition(void){
    //forced execution
    printf("\n_Forced race condition_:\n");

    atomic_store(&counter, 0);
    event_number = 0;

    pthread_barrier_init(&read_barrier, NULL, 2);
    run_workers(forced_worker);
    printf("Final counter after forced run: %d\n",
           atomic_load(&counter));
    pthread_barrier_destroy(&read_barrier);

    //fixed execution
    printf("\n_Mutex repair_:\n");

    atomic_store(&counter, 0);
    event_number = 0;

    run_workers(repaired_worker);
    printf("Final counter after repaired run: %d\n",
           atomic_load(&counter));
}
