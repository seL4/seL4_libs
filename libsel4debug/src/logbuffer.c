/*
 * Copyright 2020, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

/* Utilities for working with the debug log buffer */

#include <sel4debug/logbuffer.h>
#include <utils/base64.h>
#include <utils/cbor64.h>

/* Strings tracked and compressed in the string domain */
char *identifiers[] = {
    /* Event type */
    "type",
    "Unknown",

    /* None event */
    "None",

    /* Entry and exit events */
    "Entry",
    "Exit",
    "cpu-id",
    "timestamp",

    /* NULL array terminator */
    NULL,
};

/* Number of fields in event other than the type fields */
size_t field_count[seL4_NumLogTypeIds] = {
    [seL4_Log_TypeId(None)] = 0,
    [seL4_Log_TypeId(Entry)] = 2,
    [seL4_Log_TypeId(Exit)] = 2,
};

/*
 * Dump a single event as JSON
 */
static int event_cbor64(seL4_LogEvent *event, cbor64_domain_t *domain, base64_t *streamer)
{
    int event_type = seL4_LogEvent_type(event);

    /* Display the type */
    cbor64_map_length(streamer, field_count[event_type] + 1);
    cbor64_utf8_ref(streamer, domain, "type");

    switch (event_type) {
    case seL4_Log_TypeId(None): {
        cbor64_utf8_ref(streamer, domain, "None");
        break;
    }

    case seL4_Log_TypeId(Entry): {
        seL4_Log_Type(Entry) *entry = seL4_Log_Cast(Entry)event;
        cbor64_utf8_ref(streamer, domain, "Entry");

        cbor64_utf8_ref(streamer, domain, "cpu-id");
        cbor64_uint(streamer, event->data);

        cbor64_utf8_ref(streamer, domain, "timestamp");
        cbor64_uint(streamer, entry->timestamp);
        break;
    }

    case seL4_Log_TypeId(Exit): {
        seL4_Log_Type(Exit) *exit = seL4_Log_Cast(Exit)event;
        cbor64_utf8_ref(streamer, domain, "Exit");

        cbor64_utf8_ref(streamer, domain, "cpu-id");
        cbor64_uint(streamer, event->data);

        cbor64_utf8_ref(streamer, domain, "timestamp");
        cbor64_uint(streamer, exit->timestamp);
        break;
    }

    default: {
        cbor64_utf8_ref(streamer, domain, "Unknown");
        break;
    }
    }

    return 0;
}

/*
 * Dump the debug log to the given output
 */
int debug_log_buffer_dump_cbor64(seL4_LogBuffer *buffer, base64_t *streamer)
{
    /* Start a new string domain */
    cbor64_domain_t domain;
    cbor64_string_ref_domain(streamer, identifiers, &domain);

    /* Stop logging events */
    debug_log_buffer_finalise(buffer);

    /* Create a copy of the log buffer to traverse the events */
    seL4_LogBuffer cursor = *buffer;

    cbor64_array_start(streamer);
    seL4_LogEvent *event = seL4_LogBuffer_next(&cursor);
    while (event != NULL) {
        int err = event_cbor64(event, &domain, streamer);
        if (err != 0) {
            return err;
        }

        event = seL4_LogBuffer_next(&cursor);
    }
    return cbor64_array_end(streamer);
}
