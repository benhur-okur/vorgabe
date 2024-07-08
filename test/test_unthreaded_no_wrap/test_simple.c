#include "../include/ringbuf.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

int write_and_check(rbctx_t *ringbuffer_context, char *msg, size_t msg_len) {
    if (ringbuffer_write(ringbuffer_context, msg, msg_len) != SUCCESS) {
        printf("Error: write failed\n");
        return 1;
    }

    return 0;
}

int read_and_check(rbctx_t *ringbuffer_context, char *msg, size_t msg_len) {
    size_t read_buf_len = 2 * msg_len; // give some extra space, just in case
    char read_buf[read_buf_len];

    if (ringbuffer_read(ringbuffer_context, read_buf, &read_buf_len) != SUCCESS) {
        printf("Error: read failed\n");
        return 1;
    }

    if (strlen(msg) != strlen(read_buf)) {
        printf("Error: read message length is not correct\n");
        printf("Expected: %zu, Got: %zu\n", strlen(msg), strlen(read_buf));
        return 1;
    }

    if (strncmp(msg, read_buf, msg_len) != 0) {
        printf("Error: read message does not match\n");
        printf("Expected: %s\n", msg);
        printf("Got: %s\n", read_buf);
        return 1;
    }

    return 0;
}

int main()
{
    char msg[] = "Hello World. Nice to meet you.";
    size_t msg_len = strlen(msg) + 1; // include the null terminator
    size_t rbuf_size = 2 * msg_len + sizeof(size_t); // more than enough space for 1 message, without wrapping
    
    /* initialize ringbuffer */
    char* rbuf = malloc(rbuf_size);
    if (rbuf == NULL) {
        printf("Error: malloc failed\n");
        exit(1);
    }
    rbctx_t *ringbuffer_context = malloc(sizeof(rbctx_t));
    if (ringbuffer_context == NULL) {
        printf("Error: malloc failed\n");
        exit(1);
    }
    ringbuffer_init(ringbuffer_context, rbuf, rbuf_size);

    /* write once to buffer */
    if (write_and_check(ringbuffer_context, msg, msg_len) != 0) {
        exit(1);
    }

    /* read once and check if the message is correct */
    if (read_and_check(ringbuffer_context, msg, msg_len) != 0) {
        exit(1);
    }

    free(rbuf);
    free(ringbuffer_context);

    printf("Test passed!\n");
    return 0;
}