#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include "sem_mode.h"
#include <unistd.h>
#include "trace.h"
#include <time.h>


//job, queue, and the two worker types used in this porject
//represents one job moving through the queue
typedef struct {
    int id;
    int payload;
    int units;
    int result;
} Job;

typedef struct {
    Job *items;
    int front;
    int rear;
    int occupancy;
    int capacity;

    pthread_mutex_t mutex;
    sem_t empty;
    sem_t full;

    int produced;
    int dequeued;
    int completed;
    int result_sum;
    int max_occupancy;

    TraceEvent *events;
    int event_count;
    int event_capacity;
} Queue;

typedef struct {
    Queue *queue;
    const Config *config;
    int producer_id;
} ProducerArgs;

typedef struct {
    Queue *queue;
    const Config *config;
    int consumer_id;
} ConsumerArgs;




// this defines producer put function for adding jobs to queue
void sem_put(Queue *queue, Job job, const char *actor){
    // wait for an available empty slot before locking the queue
    sem_wait(&queue->empty);

    pthread_mutex_lock(&queue->mutex);

    // adding job to back of the queue
    queue->items[queue->rear] = job;

    //wrap pointer back to start after adding job
    queue->rear = (queue->rear + 1) % queue->capacity;
    queue->occupancy++;

    if (job.id > 0) {
        queue->produced++;
    }

    //keep track of max occupancy for the trace
    if (queue->occupancy > queue->max_occupancy) {
        queue->max_occupancy = queue->occupancy;
    }

    // record the enqueueing
    if (job.id > 0) {
        record_event(queue->events, &queue->event_count,
                     actor, "ENQ", job.id, queue->occupancy, 0);
    } else {
        record_event(queue->events, &queue->event_count,
                     actor, "STOP_ENQ", job.id, queue->occupancy, 0);
    }

    pthread_mutex_unlock(&queue->mutex);

    // announce that a new item is available
    sem_post(&queue->full);
}



// this defines the consumer get function for getting jobs from the queue
Job sem_get(Queue *queue, const char *actor){
    //wait for an available item before locking the queue
    sem_wait(&queue->full);

    pthread_mutex_lock(&queue->mutex);

    // removing job from front of the queue
    Job job = queue->items[queue->front];
    queue->front = (queue->front + 1) % queue->capacity;
    queue->occupancy--;

    // job IDs start at 1, >1 means STOP so we exclude it
    if (job.id > 0) {
        queue->dequeued++;
        record_event(queue->events, &queue->event_count,
                     actor, "DEQ", job.id, queue->occupancy, 0);
    } else {
        record_event(queue->events, &queue->event_count,
                     actor, "STOP_GET", job.id, queue->occupancy, 0);
    }

    pthread_mutex_unlock(&queue->mutex);

    // announce that a queue slot is now available
    sem_post(&queue->empty);

    return job;
}


//producer worker function!! these are same as CV mode
void *sem_producer_worker(void *arg){
    ProducerArgs *args = (ProducerArgs *)arg;

    int producer_id = args->producer_id;
    int producers = args->config->producers;
    int jobs = args->config->jobs;
    int seed = args->config->seed;
    char actor[16];
    
    snprintf(actor, sizeof(actor), "P%d", producer_id);

    //each producer handles every p+1+producers job - based on the info in the job defnition
    for (int j = producer_id + 1; j <= jobs; j += producers) {
        Job job;

        // generate the job's values
        job.id = j;
        job.payload = (37 * j + 11 * seed) % 1000;
        job.units = 1 + ((j + seed) % 5);
        job.result = 0;

        // insert the job into the shared queue
        sem_put(args->queue, job, actor);
    }

    return NULL;
}



// consumer worker function!!
void *sem_consumer_worker(void *arg){
    ConsumerArgs *args = (ConsumerArgs *)arg;
    int delay_ms = args->config->delay_ms;
    char actor[16];
    snprintf(actor, sizeof(actor), "C%d", args->consumer_id);

    while (1) {
        // retrieve the next job (one at a time)
        Job job = sem_get(args->queue, actor);

        // stop when a STOP record (job id negative)is received
        if (job.id <0) {
            break;
        }

        // representative processing delay
        if (delay_ms > 0) {
            sleep(job.units * delay_ms * 1);
        }

        //calculate the job result
        job.result = job.payload * job.units + job.id;

        // update completed job and sum stats
        pthread_mutex_lock(&args->queue->mutex);

        args->queue->completed++;
        args->queue->result_sum += job.result;

        record_event(args->queue->events, &args->queue->event_count,
             actor, "DONE", job.id, args->queue->occupancy, job.result);


        pthread_mutex_unlock(&args->queue->mutex);
    }

    return NULL;
}





//producer+put and consumer+get all together in queue
void run_sem_mode(const Config *config){
    Queue queue;

    // initialize the queue
    queue.items = malloc(config->capacity * sizeof(Job));
    if (queue.items == NULL) {
        printf("Unable to allocate queue memory.\n");
        return;
    }

    //init the trace events array
    queue.event_capacity = 3 * config->jobs + 2 * config->consumers;
    queue.events = malloc(queue.event_capacity * sizeof(TraceEvent));

    if (queue.events == NULL) {
        printf("Unable to allocate memory for the event log.\n");
        free(queue.items);
        return;
    }

    queue.event_count = 0;
    queue.front = 0;
    queue.rear = 0;
    queue.occupancy = 0;
    queue.capacity = config->capacity;

    // initialize statistics
    queue.produced = 0;
    queue.dequeued = 0;
    queue.completed = 0;
    queue.result_sum = 0;
    queue.max_occupancy = 0;

    // initialize sempahores synchronization tools
    pthread_mutex_init(&queue.mutex, NULL);
    sem_init(&queue.empty, 0, config->capacity);
    sem_init(&queue.full, 0, 0);

    // create thread and argument arrays, max 8 because of range
    pthread_t producers[8];
    pthread_t consumers[8];

    ProducerArgs producer_args[8];
    ConsumerArgs consumer_args[8];


    // starting the timer for printed info
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);


    // start consumer threads and set args
    for (int i = 0; i < config->consumers; i++) {
        consumer_args[i].queue = &queue;
        consumer_args[i].config = config;
        consumer_args[i].consumer_id = i;

        pthread_create(&consumers[i], NULL, sem_consumer_worker, &consumer_args[i]);
    }

    // start producer threads
    for (int i = 0; i < config->producers; i++) {
        producer_args[i].queue = &queue;
        producer_args[i].config = config;
        producer_args[i].producer_id = i;

        pthread_create(&producers[i], NULL, sem_producer_worker, &producer_args[i]);
    }

    // wait for all producers to finish
    // this is so we don't insert STOP records while producers are running
    for (int i = 0; i < config->producers; i++) {
        pthread_join(producers[i], NULL);
    }

    // create one unique STOP record for each consumer
    // using negative ID cause real job ids >1
    for (int i = 0; i < config->consumers; i++) {
        Job stop_job = {-1 - i, 0, 0, 0};
        sem_put(&queue, stop_job, "MAIN");
    }

    // wait for all consumers to finish
    for (int i = 0; i < config->consumers; i++) {
        pthread_join(consumers[i], NULL);
    }

    //Stop timer calculate elapsed_ms time
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed_ms = (end.tv_sec - start.tv_sec) * 1000.0;
    elapsed_ms += (end.tv_nsec - start.tv_nsec) / 1000000.0;

    
    // save the completed event log
    write_trace("trace/sem_public_case_events.csv", queue.events, queue.event_count);

    
    // print the final statistics to scroll back for now
    printf("\n========== SEM MODE RESULTS ==========\n");
    printf("Jobs: %d\n", config->jobs);
    printf("Produced: %d\n", queue.produced);
    printf("Dequeued: %d\n", queue.dequeued);
    printf("Completed: %d\n", queue.completed);
    printf("Result sum: %d\n", queue.result_sum);
    printf("Maximum occupancy: %d\n", queue.max_occupancy);
    printf("Elapsed time: %.2f ms\n", elapsed_ms);
    printf("=====================================\n");

    // clean hygience for synchronization tools and memory
   // clean hygiene for synchronization tools and memory
    pthread_mutex_destroy(&queue.mutex);
    sem_destroy(&queue.empty);
    sem_destroy(&queue.full);
    free(queue.events);

    free(queue.items);
}
