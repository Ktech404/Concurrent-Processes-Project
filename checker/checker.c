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
            fclose(trace);
            return 1;
        }
    }

    fclose(trace);

    printf("\nPASS: all %d event rows passed field validation.\n",
           event_count);

    return 0;
} 
//in progress