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
    char *msg[2] = {
        "Hello World. Nice to meet you.",
        "This is a test message."
    };
    size_t msg_len[2] = {
        /* include the null terminator */
        strlen(msg[0]) + 1, 
        strlen(msg[1]) + 1
    };

    size_t buf_size = msg_len[0] + msg_len[1] - 1; // this should definitely wrap
    
    /* initialize ringbuffer */
    char* rbuf = malloc(buf_size);
    if (rbuf == NULL) {
        printf("Error: malloc failed\n");
        exit(1);
    }
    rbctx_t *ringbuffer_context = malloc(sizeof(rbctx_t));
    if (ringbuffer_context == NULL) {
        printf("Error: malloc failed\n");
        exit(1);
    }
    ringbuffer_init(ringbuffer_context, rbuf, buf_size);

    /* write frist message */
    if (write_and_check(ringbuffer_context, msg[0], msg_len[0]) != 0) {
        exit(1);
    }

    /* now we need to read, else we wont be able to write the second message */
    if (read_and_check(ringbuffer_context, msg[0], msg_len[0]) != 0) {
        exit(1);
    }

    /* write second message will wrap */
    if (write_and_check(ringbuffer_context, msg[1], msg_len[1]) != 0) {
        exit(1);
    }

    /* read second message */
    if (read_and_check(ringbuffer_context, msg[1], msg_len[1]) != 0) {
        exit(1);
    }

    free(rbuf);
    free(ringbuffer_context);

    printf("Test passed!\n");
    return 0;
}