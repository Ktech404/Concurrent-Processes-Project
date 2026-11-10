#ifndef FILE_PARSER_H
#define FILE_PARSER_H

typedef struct {
    int seed;
    int jobs;
    int producers;
    int consumers;
    int capacity;
    int delay_ms;
} Config;

int read_config(const char *filename, Config *config);

#endif