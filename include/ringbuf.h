#ifndef RINGBUF_H
#define RINGBUF_H

#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#define SUCCESS 0
#define RINGBUFFER_FULL 1
#define RINGBUFFER_EMPTY 2
#define OUTPUT_BUFFER_TOO_SMALL 3

#define RBUF_TIMEOUT 1

typedef struct {
    uint8_t* read;
    uint8_t* write;
    uint8_t* begin;
    uint8_t* end; //1 step AFTER the last readable address
    pthread_mutex_t mutex_read;
    pthread_mutex_t mutex_write;
    pthread_cond_t signal_read;
    pthread_cond_t signal_write;
} rbctx_t;

/**
 * Initialize a thread-safe lock-free ringbuffer.
 * Generate ringbuffer context and memory before initialization.
 * 
 * @param context ringbuffer context.
 * @param buffer_location the first byte location of the ringbuffer in memory
 * @param buffer_size size of the ringbuffer (and memory)
 */
void ringbuffer_init(rbctx_t *context, void *buffer_location, size_t buffer_size);

/**
 * Write to the ringbuffer.
 * 
 * @param context ringbuffer context
 * @param message The message to be placed in the ringbuffer
 * @param message_len size of the message
 * @return SUCESS on succes, RINGBUFFER_FULL when message doesn't fit
 */
int ringbuffer_write(rbctx_t *context, void *message, size_t message_len);

/**
 * Read from the ringbuffer.
 * 
 * @param context ringbuffer context
 * @param buffer reads to this location
 * @param buffer_len_ptr size of the message buffer. Size of message received from ringbuffer is stored here
 * @return SUCCESS on succes, RINGBUFFER_EMPTY if no data to read, OUTPUT_BUFFER_TOO_SMALL when read message doesn't fit
 */
int ringbuffer_read(rbctx_t *context, void *buffer, size_t *buffer_len_ptr);

/**
 * Frees all memory allocated and syncronization variables created during initialization.
 * 
 * @param context ringbuffer context
 */
void ringbuffer_destroy(rbctx_t *context);

#endif //RINGBUF_H
