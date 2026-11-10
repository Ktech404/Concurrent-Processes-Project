#include <stdio.h>
#include <pthread.h>
#include "file_parser.h"

// worker function to print IDs

void *worker_message(void *arg){
    int id = *(int *)arg; 
    printf("I am a worker and my ID is %d\n", id);
    return NULL;
};

//integer values of the worker IDs
int ID1 = 0;
int ID2 = 1;

int main(){

    pthread_t worker1, worker2;

    pthread_create(&worker1, NULL, worker_message, &ID1);
    pthread_create(&worker2, NULL, worker_message, &ID2);

    pthread_join(worker1, NULL);
    pthread_join(worker2, NULL);

    printf("Both workers have successfully finished.\n");
    

    //values loading
    Config config;

    if (!read_config("config/config_vals.txt", &config)) {
        printf("Configuration loading failed.\n");
        return 1;
    }

    printf("\nConfiguration loaded:\n");
    printf("Seed: %d\n", config.seed);
    printf("Jobs: %d\n", config.jobs);
    printf("Producers: %d\n", config.producers);
    printf("Consumers: %d\n", config.consumers);
    printf("Capacity: %d\n", config.capacity);
    printf("Delay: %d ms\n", config.delay_ms);

    return 0;

}
