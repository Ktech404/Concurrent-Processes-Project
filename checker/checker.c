#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "../src/file_parser.h"


// input files
const char *config_file = "config/public_case.txt";
const char *trace_file = "traces/cv_public_case_events.csv";


//helper function to separate col vals
int split_fields(char *line, char *fields[]) {
    int count = 0;
    char *start = line;

    for (int i = 0; ; i++) {
        if (line[i] == ',' || line[i] == '\0') {
            if (count >= 6) {
                return 0;
            }

            fields[count] = start;
            count++;

            if (line[i] == '\0') {
                break;
            }

            line[i] = '\0';
            start = &line[i + 1];
        }
    }

    return count == 6;
}



// convert a string into an integer
int parse_integer(const char *value, int *number) {
    char *end;
    long parsed;

    if (value[0] == '\0') {
        return 0;
    }

    parsed = strtol(value, &end, 10);

    if (*end != '\0' || end == value ||
        parsed < INT_MIN || parsed > INT_MAX) {
        return 0;
    }

    *number = (int)parsed;
    return 1;
}



// validate one event and its fields
int validate_event(char *fields[], Config *config, int expected_seq,
                   int event_number) {
    int seq, job_id, occupancy, result;
    int producer_id;
    int expected_result;
    int payload, units;

    // validate integer fields
    if (!parse_integer(fields[0], &seq)) {
        printf("FAIL: event %d: invalid sequence number.\n", event_number);
        return 0;
    }

    if (!parse_integer(fields[3], &job_id)) {
        printf("FAIL: event %d: invalid job ID.\n", event_number);
        return 0;
    }

    if (!parse_integer(fields[4], &occupancy)) {
        printf("FAIL: event %d: invalid occupancy.\n", event_number);
        return 0;
    }

    //validate sequence number
    if (seq != expected_seq) {
        printf("FAIL: event %d: expected sequence %d, found %d.\n",
               event_number, expected_seq, seq);
        return 0;
    }

    //validate occupancy range
    if (occupancy < 0 || occupancy > config->capacity) {
        printf("FAIL: event %d: occupancy is outside queue capacity.\n",
               event_number);
        return 0;
    }

    // validate event type and actor combination
    if (strcmp(fields[2], "ENQ") == 0) {
        if (fields[1][0] != 'P' ||
            fields[1][1] < '0' || fields[1][1] > '9' ||
            fields[1][2] != '\0') {
            printf("FAIL: event %d: ENQ must have a producer actor.\n",
                   event_number);
            return 0;
        }

        producer_id = fields[1][1] - '0';

        if (producer_id >= config->producers) {
            printf("FAIL: event %d: invalid producer actor.\n", event_number);
            return 0;
        }

        if (job_id < 1 || job_id > config->jobs) {
            printf("FAIL: event %d: ENQ job ID is outside the valid range.\n",
                   event_number);
            return 0;
        }

        if (strcmp(fields[5], "-") != 0) {
            printf("FAIL: event %d: ENQ result must be '-'.\n", event_number);
            return 0;
        }

        // verify producer ownership
        if (producer_id != (job_id - 1) % config->producers) {
            printf("FAIL: event %d: producer %s does not own job %d.\n",
                   event_number, fields[1], job_id);
            return 0;
        }
    }
    else if (strcmp(fields[2], "DEQ") == 0 ||
             strcmp(fields[2], "DONE") == 0) {
        if (fields[1][0] != 'C' ||
            fields[1][1] < '0' || fields[1][1] > '9' ||
            fields[1][2] != '\0') {
            printf("FAIL: event %d: event must have a consumer actor.\n",
                   event_number);
            return 0;
        }

        if (fields[1][1] - '0' >= config->consumers) {
            printf("FAIL: event %d: invalid consumer actor.\n", event_number);
            return 0;
        }

        if (job_id < 1 || job_id > config->jobs) {
            printf("FAIL: event %d: real job ID is outside the valid range.\n",
                   event_number);
            return 0;
        }

        if (strcmp(fields[2], "DEQ") == 0) {
            if (strcmp(fields[5], "-") != 0) {
                printf("FAIL: event %d: DEQ result must be '-'.\n",
                       event_number);
                return 0;
            }
        }
        else {
            if (!parse_integer(fields[5], &result)) {
                printf("FAIL: event %d: DONE requires an integer result.\n",
                       event_number);
                return 0;
            }

            // calculate the expected result
            payload = (37 * job_id + 11 * config->seed) % 1000;
            units = 1 + ((job_id + config->seed) % 5);
            expected_result = payload * units + job_id;

            if (result != expected_result) {
                printf("FAIL: event %d: incorrect result for job %d. "
                       "Expected %d, found %d.\n",
                       event_number, job_id, expected_result, result);
                return 0;
            }
        }
    }

    else if (strcmp(fields[2], "STOP_ENQ") == 0) {
        if (strcmp(fields[1], "MAIN") != 0) {
            printf("FAIL: event %d: STOP_ENQ must be performed by MAIN.\n",
                   event_number);
            return 0;
        }

        if (job_id > -1 || job_id < -config->consumers) {
            printf("FAIL: event %d: invalid STOP job ID.\n", event_number);
            return 0;
        }

        if (strcmp(fields[5], "-") != 0) {
            printf("FAIL: event %d: STOP_ENQ result must be '-'.\n",
                   event_number);
            return 0;
        }
    }
    else if (strcmp(fields[2], "STOP_GET") == 0) {
        if (fields[1][0] != 'C' ||
            fields[1][1] < '0' || fields[1][1] > '9' ||
            fields[1][2] != '\0') {
            printf("FAIL: event %d: STOP_GET must have a consumer actor.\n",
                   event_number);
            return 0;
        }

        if (fields[1][1] - '0' >= config->consumers) {
            printf("FAIL: event %d: invalid consumer actor.\n", event_number);
            return 0;
        }

        if (job_id > -1 || job_id < -config->consumers) {
            printf("FAIL: event %d: invalid STOP job ID.\n", event_number);
            return 0;
        }

        if (strcmp(fields[5], "-") != 0) {
            printf("FAIL: event %d: STOP_GET result must be '-'.\n",
                   event_number);
            return 0;
        }
    }
    else {
        printf("FAIL: event %d: unknown event type '%s'.\n",
               event_number, fields[2]);
        return 0;
    }

    return 1;
}


#define IDLE 0
#define PROCESSING 1
#define STOPPED 2

// validate the order of events for each consumer
int validate_consumer_state(char *fields[], int consumer_state[],
                            int current_job[], int event_number) {
    int consumer_id = fields[1][1] - '0';
    int job_id;
    const char *event = fields[2];

    // ignore events that do not belong to consumers
    if (strcmp(event, "DEQ") != 0 &&
        strcmp(event, "DONE") != 0 &&
        strcmp(event, "STOP_GET") != 0) {
        return 1;
    }

    parse_integer(fields[3], &job_id);

    // reject any event after the consumer has stopped
    if (consumer_state[consumer_id] == STOPPED) {
        printf("FAIL: event %d: consumer C%d performed an event after stopping.\n",
               event_number, consumer_id);
        return 0;
    }

    // consumer starts processing a job
    if (strcmp(event, "DEQ") == 0) {
        if (consumer_state[consumer_id] == PROCESSING) {
            printf("FAIL: event %d: consumer C%d dequeued job %d "
                   "before completing job %d.\n",
                   event_number, consumer_id, job_id,
                   current_job[consumer_id]);
            return 0;
        }

        consumer_state[consumer_id] = PROCESSING;
        current_job[consumer_id] = job_id;
    }

    // consumer completes its current job
    else if (strcmp(event, "DONE") == 0) {
        if (consumer_state[consumer_id] != PROCESSING) {
            printf("FAIL: event %d: consumer C%d completed a job "
                   "without dequeuing one.\n",
                   event_number, consumer_id);
            return 0;
        }

        if (current_job[consumer_id] != job_id) {
            printf("FAIL: event %d: consumer C%d completed job %d "
                   "but was processing job %d.\n",
                   event_number, consumer_id, job_id,
                   current_job[consumer_id]);
            return 0;
        }

        consumer_state[consumer_id] = IDLE;
        current_job[consumer_id] = 0;
    }

    // consumer receives its STOP
    else if (strcmp(event, "STOP_GET") == 0) {
        if (consumer_state[consumer_id] == PROCESSING) {
            printf("FAIL: event %d: consumer C%d received STOP "
                   "before completing job %d.\n",
                   event_number, consumer_id,
                   current_job[consumer_id]);
            return 0;
        }

        consumer_state[consumer_id] = STOPPED;
    }

    return 1;
}



// replay queue events and verify fifo order and occupancy
int validate_queue_event(char *fields[], int queue[], int *front,
                         int *count, int capacity, int event_number) {
    int job_id;
    int occupancy;

    parse_integer(fields[3], &job_id);
    parse_integer(fields[4], &occupancy);

    // enqueue a real job or STOP
    if (strcmp(fields[2], "ENQ") == 0 ||
        strcmp(fields[2], "STOP_ENQ") == 0) {

        // check whether the queue is full
        if (*count >= capacity) {
            printf("FAIL: event %d: enqueue attempted while queue is full.\n",
                   event_number);
            return 0;
        }

        // calculate the rear position
        int rear = (*front + *count) % capacity;

        // add the item to the queue
        queue[rear] = job_id;
        (*count)++;
    }

    // dequeue a real job or STOP
    else if (strcmp(fields[2], "DEQ") == 0 ||
             strcmp(fields[2], "STOP_GET") == 0) {

        // check whether the queue is empty
        if (*count == 0) {
            printf("FAIL: event %d: dequeue attempted while queue is empty.\n",
                   event_number);
            return 0;
        }

        // check whether the requested item is at the front
        if (queue[*front] != job_id) {
            printf("FAIL: event %d: FIFO violation. Expected job ID %d, "
                   "but found %d at the front of the queue.\n",
                   event_number, queue[*front], job_id);
            return 0;
        }

        // remove the front item
        *front = (*front + 1) % capacity;
        (*count)--;
    }

    // verify recorded occupancy against reconstructed occupancy
    if (occupancy != *count) {
        printf("FAIL: event %d: incorrect occupancy. "
               "Expected %d, found %d.\n",
               event_number, *count, occupancy);
        return 0;
    }

    return 1;
}


// validate shutdown events and track the final result sum
int validate_shutdown_event(char *fields[],
                            int stop_enq[], int stop_get[],
                            int consumer_stop[], int *first_stop,
                            int *result_sum, int event_number) {
    const char *event = fields[2];
    int job_id;
    int result;
    int consumer_id;
    int stop_id;

    parse_integer(fields[3], &job_id);

    // no real jobs may be enqueued after the first STOP
    if (strcmp(event, "ENQ") == 0) {
        if (*first_stop) {
            printf("FAIL: event %d: real job enqueued after STOP began.\n",
                   event_number);
            return 0;
        }
    }

    // track STOP records inserted by MAIN
    else if (strcmp(event, "STOP_ENQ") == 0) {
        *first_stop = 1;
        stop_id = -job_id;

        if (stop_enq[stop_id] > 0) {
            printf("FAIL: event %d: duplicate STOP_ENQ for STOP%d.\n",
                   event_number, stop_id);
            return 0;
        }

        stop_enq[stop_id]++;
    }

    // track STOP records removed by consumers
    else if (strcmp(event, "STOP_GET") == 0) {
        consumer_id = fields[1][1] - '0';
        stop_id = -job_id;

        if (stop_get[stop_id] > 0) {
            printf("FAIL: event %d: STOP%d was removed more than once.\n",
                   event_number, stop_id);
            return 0;
        }

        stop_get[stop_id]++;
        consumer_stop[consumer_id]++;
    }

    // accumulate completed job results
    else if (strcmp(event, "DONE") == 0) {
        if (!parse_integer(fields[5], &result)) {
            printf("FAIL: event %d: invalid DONE result.\n", event_number);
            return 0;
        }

        *result_sum += result;
    }

    return 1;
}


int run_checker(void) {
    Config config;

    //read configuration
    if (!read_config(config_file, &config)) {
        printf("ERROR: could not read configuration file.\n");
        return 1;
    }

    printf("Configuration loaded successfully.\n");
    printf("Seed: %d\n", config.seed);
    printf("Jobs: %d\n", config.jobs);
    printf("Producers: %d\n", config.producers);
    printf("Consumers: %d\n", config.consumers);
    printf("Capacity: %d\n", config.capacity);

    // open trace file
    FILE *trace = fopen(trace_file, "r");

    if (trace == NULL) {
        printf("ERROR: could not open trace file.\n");
        return 1;
    }

    char header[256];

    if (fgets(header, sizeof(header), trace) == NULL) {
        printf("ERROR: trace file is empty.\n");
        fclose(trace);
        return 1;
    }

    // remove newline from header
    header[strcspn(header, "\r\n")] = '\0';

    // validate csv header
    if (strcmp(header, "seq,actor,event,job_id,occupancy,result") != 0) {
        printf("ERROR: invalid CSV header.\n");
        fclose(trace);
        return 1;
    }

    printf("CSV header validated successfully.\n");


    // track how many times each job appears in each event
    int *enq_count = calloc(config.jobs + 1, sizeof(int));
    int *deq_count = calloc(config.jobs + 1, sizeof(int));
    int *done_count = calloc(config.jobs + 1, sizeof(int));
    // track the state of each consumer
    int consumer_state[8] = {IDLE};
    int current_job[8] = {0};
    
    // reconstructed queue
    int *queue = malloc(config.capacity * sizeof(int));
    int front = 0;
    int queue_count = 0;

    // track STOP creation and consumption
    int stop_enq[9] = {0};
    int stop_get[9] = {0};
    int consumer_stop[8] = {0};

    int first_stop = 0;
    int result_sum = 0;

    if (queue == NULL) {
        printf("ERROR: could not allocate reconstructed queue.\n");
        free(enq_count);
        free(deq_count);
        free(done_count);
        fclose(trace);
        return 1;
    }

    if (enq_count == NULL || deq_count == NULL || done_count == NULL) {
        printf("ERROR: could not allocate job counters.\n");
        free(enq_count);
        free(deq_count);
        free(done_count);
        fclose(trace);
        return 1;
    }


    char line[256];
    int event_count = 0;

    while (fgets(line, sizeof(line), trace) != NULL) {
        event_count++;

        // remove newline
        line[strcspn(line, "\r\n")] = '\0';

        // reject empty rows
        if (line[0] == '\0') {
            printf("FAIL: event %d: empty row.\n", event_count);
            fclose(trace);
            return 1;
        }

        //split row into six fields
        char *fields[6];

        if (!split_fields(line, fields)) {
            printf("FAIL: event %d: row must contain exactly six fields.\n",
                   event_count);
            fclose(trace);
            return 1;
        }

        //reject empty fields
        for (int i = 0; i < 6; i++) {
            if (fields[i][0] == '\0') {
                printf("FAIL: event %d: field %d is empty.\n",
                       event_count, i + 1);
                fclose(trace);
                return 1;
            }
        }

        // validate event fields and values
        if (!validate_event(fields, &config, event_count, event_count)) {
            free(enq_count);
            free(deq_count);
            free(done_count);
            fclose(trace);
            return 1;
        }

        // validate consumer event order
        if (!validate_consumer_state(fields, consumer_state,
                                     current_job, event_count)) {
            free(enq_count);
            free(deq_count);
            free(done_count);
            fclose(trace);
            return 1;
        }

        // validate fifo order and occupancy
        if (!validate_queue_event(fields, queue, &front,
                                  &queue_count, config.capacity,
                                  event_count)) {
            free(enq_count);
            free(deq_count);
            free(done_count);
            free(queue);
            fclose(trace);
            return 1;
        }

        // validate shutdown events and result sum
        if (!validate_shutdown_event(fields, stop_enq,
                                    stop_get, consumer_stop, &first_stop,
                                    &result_sum, event_count)) {
            free(enq_count);
            free(deq_count);
            free(done_count);
            free(queue);
            fclose(trace);
            return 1;
        }

        // get the job ID
        int job_id;
        parse_integer(fields[3], &job_id);

        // count real job events
        if (job_id > 0) {
            if (strcmp(fields[2], "ENQ") == 0) {
                if (enq_count[job_id] > 0) {
                    printf("FAIL: event %d: duplicate ENQ for job %d.\n",
                           event_count, job_id);
                    free(enq_count);
                    free(deq_count);
                    free(done_count);
                    fclose(trace);
                    return 1;
                }

                enq_count[job_id]++;
            }
            else if (strcmp(fields[2], "DEQ") == 0) {
                if (deq_count[job_id] > 0) {
                    printf("FAIL: event %d: duplicate DEQ for job %d.\n",
                           event_count, job_id);
                    free(enq_count);
                    free(deq_count);
                    free(done_count);
                    fclose(trace);
                    return 1;
                }

                deq_count[job_id]++;
            }
            else if (strcmp(fields[2], "DONE") == 0) {
                if (done_count[job_id] > 0) {
                    printf("FAIL: event %d: duplicate DONE for job %d.\n",
                           event_count, job_id);
                    free(enq_count);
                    free(deq_count);
                    free(done_count);
                    fclose(trace);
                    return 1;
                }

                done_count[job_id]++;
            }
        }
        
    }


     // verify that every job has exactly one of each event
    for (int i = 1; i <= config.jobs; i++) {
        if (enq_count[i] != 1) {
            printf("FAIL: EOF: job %d is missing an ENQ event.\n", i);
            free(enq_count);
            free(deq_count);
            free(done_count);
            fclose(trace);
            return 1;
        }

        if (deq_count[i] != 1) {
            printf("FAIL: EOF: job %d is missing a DEQ event.\n", i);
            free(enq_count);
            free(deq_count);
            free(done_count);
            fclose(trace);
            return 1;
        }

        if (done_count[i] != 1) {
            printf("FAIL: EOF: job %d is missing a DONE event.\n", i);
            free(enq_count);
            free(deq_count);
            free(done_count);
            fclose(trace);
            return 1;
        }
    }

    for (int i = 0; i < config.consumers; i++) {
        if (consumer_stop[i] != 1) {
            printf("FAIL: EOF: consumer C%d did not receive exactly one STOP.\n",
                i);
            free(enq_count);
            free(deq_count);
            free(done_count);
            free(queue);
            fclose(trace);
            return 1;
        }

        if (consumer_state[i] != STOPPED) {
            printf("FAIL: EOF: consumer C%d did not stop.\n", i);
            free(enq_count);
            free(deq_count);
            free(done_count);
            free(queue);
            fclose(trace);
            return 1;
        }
    }

    // calculate the expected result sum
    int expected_result_sum = 0;

    for (int i = 1; i <= config.jobs; i++) {
        int payload = (37 * i + 11 * config.seed) % 1000;
        int units = 1 + ((i + config.seed) % 5);

        expected_result_sum += payload * units + i;
    }

    // compare actual and expected result sums
    if (result_sum != expected_result_sum) {
        printf("FAIL: EOF: incorrect result sum. "
            "Expected %d, found %d.\n",
            expected_result_sum, result_sum);

        free(enq_count);
        free(deq_count);
        free(done_count);
        free(queue);
        fclose(trace);
        return 1;
    }
    
    // verify queue is empty at the end of the trace
        if (queue_count != 0) {
            printf("FAIL: EOF: queue is not empty. "
                "%d items remain.\n", queue_count);
            free(enq_count);
            free(deq_count);
            free(done_count);
            free(queue);
            fclose(trace);
            return 1;
        }
    free(enq_count);
    free(deq_count);
    free(done_count);
    free(queue);

    fclose(trace);

    printf("\nPASS: all %d event rows passed field validation.\n",
           event_count);

    return 0;
} 
