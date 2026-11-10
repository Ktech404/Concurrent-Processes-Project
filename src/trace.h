#ifndef TRACE_H
#define TRACE_H

//init header vals
typedef struct {
    int seq;
    char actor[16];
    char event[16];
    int job_id;
    int occupancy;
    int result;
} TraceEvent;

void record_event(TraceEvent events[], int *event_count, const char *actor,
                  const char *event, int job_id, int occupancy, int result);

void write_trace(const char *filename, TraceEvent events[], int event_count);

#endif
