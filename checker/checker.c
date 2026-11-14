#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/file_parser.h"


// input files
const char *config_file = "config/public_case.txt";
const char *trace_file = "traces/cv_public_case_events.csv";

int run_checker(void) {
    Config config;

    // read configuration
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

        char header[256];

    if (fgets(header, sizeof(header), trace) == NULL) {
        printf("ERROR: trace file is empty.\n");
        fclose(trace);
        return 1;
    }

    // remove newline from header
    header[strcspn(header, "\r\n")] = '\0';

    //makes sure is valid
    if (strcmp(header, "seq,actor,event,job_id,occupancy,result") != 0) {
        printf("ERROR: invalid CSV header.\n");
        fclose(trace);
        return 1;
    }

    printf("CSV header validated successfully.\n");

    fclose(trace);

    return 0;
} 
//in progress