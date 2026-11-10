#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "file_parser.h"

int read_config(const char *filename, Config *config)
{
    FILE *file = fopen(filename, "r");

    if (file == NULL) {
        printf("Error: Could not open configuration file.\n");
        return 0;
    }

    char line[256];

    while (fgets(line, sizeof(line), file) != NULL) {

        //remove leading whitespace
        char *start = line;
        while (isspace((unsigned char)*start)) {
            start++;
        }

        // ignore blank lines and comments
        if (*start == '\0' || *start == '\n' || *start == '#') {
            continue;
        }

        // fine the equals sign
        char *equals = strchr(start, '=');

        if (equals == NULL) {
            printf("Error: Invalid configuration line: %s", start);
            fclose(file);
            return 0;
        }

        // split key and value
        *equals = '\0';

        char *key = start;
        char *value = equals + 1;

        // remove trailing whitespace from key
        char *end = key + strlen(key) - 1;
        while (end >= key && isspace((unsigned char)*end)) {
            *end = '\0';
            end--;
        }

        // convert value into integer
        char *value_end;
        long number = strtol(value, &value_end, 10);

        if (value == value_end) {
            printf("Error: Invalid integer value for %s\n", key);
            fclose(file);
            return 0;
        }

        //assign the value to the appropriate field
        if (strcmp(key, "seed") == 0) {
            config->seed = number;
        }
        else if (strcmp(key, "jobs") == 0) {
            config->jobs = number;
        }
        else if (strcmp(key, "producers") == 0) {
            config->producers = number;
        }
        else if (strcmp(key, "consumers") == 0) {
            config->consumers = number;
        }
        else if (strcmp(key, "capacity") == 0) {
            config->capacity = number;
        }
        else if (strcmp(key, "delay_ms") == 0) {
            config->delay_ms = number;
        }
        else {
            printf("Error: Unknown configuration key: %s\n", key);
            fclose(file);
            return 0;
        }
    }

    fclose(file);
    return 1;
}