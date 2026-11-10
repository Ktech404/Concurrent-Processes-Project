#include <stdio.h>
#include <pthread.h>
#include "file_parser.h"
#include "race_condition.h"

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

    /* just commenting this out for now
    Config config;

    //load and validate configuration
    if (!read_config("config/config_vals.txt", &config)) {
        printf("Program cannot start due to invalid configuration.\n");
        return 1;
    }
    printf("Configuration successfully loaded.\n");

    //starting workers
    pthread_t worker1, worker2;

    pthread_create(&worker1, NULL, worker_message, &ID1);
    pthread_create(&worker2, NULL, worker_message, &ID2);

    pthread_join(worker1, NULL);
    pthread_join(worker2, NULL);

    printf("Both workers have successfully finished.\n");
    */

    run_race_condition();
}
