#include "../include/ringbuf.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

void ringbuffer_init(rbctx_t *context, void *buffer_location, size_t buffer_size)
{
    pthread_mutex_init(&(context->mutex_read), NULL);
    pthread_mutex_init(&(context->mutex_write), NULL);
    pthread_cond_init(&(context->signal_read), NULL);
    pthread_cond_init(&(context->signal_write), NULL);

    context->begin = buffer_location;
    context->end = buffer_location + buffer_size;
    context->read = context->begin;
    context->write = context->begin;
}

void write_to_buffer(rbctx_t *context, void *message, size_t message_len)
{
    size_t space_till_end = context->end - context->write;

    if(space_till_end < message_len) {
        size_t first_chunk_len = space_till_end;
        size_t second_chunk_len = message_len - first_chunk_len;

        memcpy(context->write, message, first_chunk_len);
        memcpy(context->begin, (uint8_t *)message + first_chunk_len, second_chunk_len);

        context->write = context->begin + second_chunk_len;
    } else {
        memcpy(context->write, message, message_len);
        context->write += message_len;
    }

    if(context->write == context->end) {
        context->write = context->begin;
    }
}

void read_from_buffer(rbctx_t *context, void *buffer, size_t message_len) {
    if (context->read + message_len > context->end) {
        // need to read by parts,
        size_t first_chunk_len = context->end - context->read;
        size_t second_chunk_len = message_len - first_chunk_len;

        memcpy(buffer, context->read, first_chunk_len );
        memcpy((uint8_t *)buffer + first_chunk_len, context->begin, second_chunk_len);
        context->read = context->begin + second_chunk_len;
    } else {
        memcpy(buffer, context->read, message_len);
        context->read += message_len;
    }

    if(context->read == context->end) {
        context->read = context->begin;
    }
}

size_t get_available_size(rbctx_t *context) {
    size_t available_size = context->read - context->write -1;

    if(context->read  <= context->write) {
        available_size = context->read - context->write + context->end - context->begin;
    }

    return available_size;
}
int ringbuffer_write(rbctx_t *context, void *message, size_t message_len)
{
    pthread_mutex_lock(&(context->mutex_write));

    printf("Write: Begin\n");
    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_sec = 0;
    timeout.tv_nsec = 100000000; // 0.1 second timeout

    size_t available_size = get_available_size(context);
    printf("Write: available_size %lu\n", available_size);

    if (available_size < message_len + sizeof(size_t)) {
        printf("Write: Full, wait for write signal, send read signal, available (%lu), msg (%lu)\n", available_size, message_len);
        int wait_result = pthread_cond_timedwait(&(context->signal_write), &(context->mutex_write), &timeout);
        if(wait_result == ETIMEDOUT) {
            printf("Write: Full, wait timeout, continue\n");
        }else {
            printf("Write: Full, signal received, continue\n");
        }
        pthread_mutex_unlock(&(context->mutex_write));
        return RINGBUFFER_FULL;
    }

    write_to_buffer(context, &message_len, sizeof(size_t));
    write_to_buffer(context, message, message_len);

    printf("Write: finished, available size : %lu\n", get_available_size(context));
    pthread_mutex_unlock(&(context->mutex_write));
    pthread_cond_signal(&(context->signal_write));
    // pthread_cond_signal(&(context->signal_read));
    // pthread_cond_signal(&(context->signal_read));

    return SUCCESS;
}

int ringbuffer_read(rbctx_t *context, void *buffer, size_t *buffer_len)
{
    pthread_mutex_lock(&(context->mutex_read));

    printf("Read: begin\n");
    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_sec = 0;
    timeout.tv_nsec = 100000000; // 0.1 second timeout


    size_t message_len;

    if (context->write == context->read) {
        // pthread_cond_signal(&(context->signal_write));
        int wait_result = pthread_cond_timedwait(&(context->signal_read), &(context->mutex_read), &timeout);
        if (wait_result == ETIMEDOUT) {
            printf("Read: Empty, wait timeout, continue\n");
        } else {
            printf("Read: Empty, signal received, continue\n");
        }

        pthread_mutex_unlock(&(context->mutex_read));
        pthread_cond_signal(&(context->signal_read));
        return RINGBUFFER_EMPTY;
    }

    read_from_buffer(context, &message_len, sizeof(size_t));
    *buffer_len = message_len;

    if (message_len > *buffer_len) {
        printf("READ: OUTPUT_BUFFER_TOO_SMALL, buffer_len(%lu), message_len(%lu)\n", *buffer_len, message_len);
        context->read -= sizeof(size_t);
        if(context->read < context->begin) {
            context->read += context->end - context->begin;
        }
        pthread_mutex_unlock(&(context->mutex_read));
        pthread_cond_signal(&(context->signal_read));
        return OUTPUT_BUFFER_TOO_SMALL;
    }

    read_from_buffer(context, buffer, message_len);

    printf("Read: finished, available size: %lu \n", get_available_size(context));
    // pthread_cond_signal(&(context->signal_read));
    pthread_mutex_unlock(&(context->mutex_read));
    pthread_cond_signal(&(context->signal_read));
    // pthread_cond_signal(&(context->signal_write));
    return SUCCESS;
}

void ringbuffer_destroy(rbctx_t *context)
{
    pthread_mutex_destroy(&(context->mutex_read));
    pthread_mutex_destroy(&(context->mutex_write));
    pthread_cond_destroy(&(context->signal_read));
    pthread_cond_destroy(&(context->signal_write));
}
