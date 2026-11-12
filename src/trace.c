#include <stdio.h>
#include <string.h>
#include "trace.h"

void record_event(TraceEvent events[], int *event_count, const char *actor,
                  const char *event, int job_id, int occupancy, int result)
{
    // save the event in the next available position
    TraceEvent *entry = &events[*event_count];

    entry->seq = *event_count + 1;

    snprintf(entry->actor, sizeof(entry->actor), "%s", actor);
    snprintf(entry->event, sizeof(entry->event), "%s", event);

    entry->job_id = job_id;
    entry->occupancy = occupancy;
    entry->result = result;

    (*event_count)++;
}

void write_trace(const char *filename, TraceEvent events[], int event_count)
{
    FILE *file = fopen(filename, "w");

    if (file == NULL) {
        printf("Unable to create trace file.\n");
        return;
    }

    // write the CSV header
    fprintf(file, "seq,actor,event,job_id,occupancy,result\n");

    // write each recorded event
    for (int i = 0; i < event_count; i++) {
        fprintf(file, "%d,%s,%s,%d,%d,",
                events[i].seq,
                events[i].actor,
                events[i].event,
                events[i].job_id,
                events[i].occupancy);

        if (strcmp(events[i].event, "DONE") == 0) {
            fprintf(file, "%d\n", events[i].result);
        } else {
            fprintf(file, "-\n");
        }
    }

    fclose(file);
    printf("Trace saved to %s\n", filename);
}
