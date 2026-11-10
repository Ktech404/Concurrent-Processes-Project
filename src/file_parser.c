
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "file_parser.h"

int read_config(const char *filename, Config *config){
    FILE *file = fopen(filename, "r");

    if (file == NULL) {
        printf("Error: Could not open configuration file.\n");
        return 0;
    }

    // use a temporary config so invalid values are never returned
    Config temp = {0};

    // keep track of which keys have been found
    int seen[6] = {0};

    char line[256];
    int line_number = 0;

    while (fgets(line, sizeof(line), file) != NULL) {
        line_number++;

        // reject lines that are too long
        if (strchr(line, '\n') == NULL && !feof(file)) {
            printf("Error: Line %d is too long.\n", line_number);
            fclose(file);
            return 0;
        }

        // remove leading whitespace
        char *start = line;

        while (isspace((unsigned char)*start)) {
            start++;
        }

        // ignore blank lines and comments
        if (*start == '\0' || *start == '#' || *start == '\n') {
            continue;
        }

        // find the equals sign
        char *equals = strchr(start, '=');

        if (equals == NULL) {
            printf("Error: Invalid configuration line %d. Expected key=value.\n", line_number);
            fclose(file);
            return 0;
        }

        // split key and value
        *equals = '\0';

        char *key = start;
        char *value = equals + 1;

        // remove trailing whitespace from key
        char *end = key + strlen(key);

        while (end > key && isspace((unsigned char)*(end - 1))) {
            *(--end) = '\0';
        }

        // reject empty keys
        if (*key == '\0') {
            printf("Error: Empty key on line %d.\n", line_number);
            fclose(file);
            return 0;
        }

        // identify the key and check for duplicates
        int index = -1;

        if (strcmp(key, "seed") == 0) {
            index = 0;
        }
        else if (strcmp(key, "jobs") == 0) {
            index = 1;
        }
        else if (strcmp(key, "producers") == 0) {
            index = 2;
        }
        else if (strcmp(key, "consumers") == 0) {
            index = 3;
        }
        else if (strcmp(key, "capacity") == 0) {
            index = 4;
        }
        else if (strcmp(key, "delay_ms") == 0) {
            index = 5;
        }
        else {
            printf("Error: Unknown configuration key '%s'.\n", key);
            fclose(file);
            return 0;
        }

        // reject duplicate keys
        if (seen[index]) {
            printf("Error: Duplicate configuration key '%s'.\n", key);
            fclose(file);
            return 0;
        }

        // convert the value into an integer
        char *value_end;
        errno = 0;

        long number = strtol(value, &value_end, 10);

        // reject values that are not valid integers
        if (value == value_end || errno == ERANGE) {
            printf("Error: Invalid integer value for '%s'.\n", key);
            fclose(file);
            return 0;
        }

        // allow trailing whitespace, but reject other characters
        while (isspace((unsigned char)*value_end)) {
            value_end++;
        }

        if (*value_end != '\0') {
            printf("Error: Invalid characters after value for '%s'.\n", key);
            fclose(file);
            return 0;
        }

        // validate the range and assign the value
        if (index == 0) {
            if (number < 0 || number > 9999) {
                printf("Error: seed must be between 0 and 9999.\n");
                fclose(file);
                return 0;
            }
            temp.seed = (int)number;
        }
        else if (index == 1) {
            if (number < 0 || number > 10000) {
                printf("Error: jobs must be between 0 and 10000.\n");
                fclose(file);
                return 0;
            }
            temp.jobs = (int)number;
        }
        else if (index == 2) {
            if (number < 1 || number > 8) {
                printf("Error: producers must be between 1 and 8.\n");
                fclose(file);
                return 0;
            }
            temp.producers = (int)number;
        }
        else if (index == 3) {
            if (number < 1 || number > 8) {
                printf("Error: consumers must be between 1 and 8.\n");
                fclose(file);
                return 0;
            }
            temp.consumers = (int)number;
        }
        else if (index == 4) {
            if (number < 1 || number > 64) {
                printf("Error: capacity must be between 1 and 64.\n");
                fclose(file);
                return 0;
            }
            temp.capacity = (int)number;
        }
        else if (index == 5) {
            if (number < 0 || number > 10) {
                printf("Error: delay_ms must be between 0 and 10.\n");
                fclose(file);
                return 0;
            }
            temp.delay_ms = (int)number;
        }

        // mark this key as found
        seen[index] = 1;
    }

    // check for file reading errors
    if (ferror(file)) {
        printf("Error: Failed while reading configuration file.\n");
        fclose(file);
        return 0;
    }

    fclose(file);

    // check that all six required keys were provided
    for (int i = 0; i < 6; i++) {
        if (!seen[i]) {
            printf("Error: Configuration file is missing a required key.\n");
            return 0;
        }
    }

    // only copy the validated values into the supplied config
    *config = temp;

    return 1;
}
