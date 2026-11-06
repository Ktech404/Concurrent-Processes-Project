#include <stdio.h>
#include <pthread.h>

//worker function to print IDs

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

    return 0;


}
