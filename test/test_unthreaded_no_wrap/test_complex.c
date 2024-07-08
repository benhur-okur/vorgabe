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
        printf("Expected: %s\n", msg);
        printf("Got: %s\n", read_buf);
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
    /* an array of 5 messages */
    char *msg[5] = {
        "Hello World. Nice to meet you.",
        "This is a test message.",
        "Another message.",
        "Yet another message.",
        "The last message."
    };

    size_t msg_len[5] = {
        /* include the null terminator */
        strlen(msg[0]) + 1, 
        strlen(msg[1]) + 1,
        strlen(msg[2]) + 1,
        strlen(msg[3]) + 1,
        strlen(msg[4]) + 1
    };

    /* allocate more than enough space for 5 messages, without wrapping */
    size_t rbuf_size = 0; 
    for (int i = 0; i < 5; i++) {
        rbuf_size += 2 * (msg_len[i] + sizeof(size_t));
    }

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

    /* write first 2 messages to buffer */
    for (int i = 0; i < 2; i++) {
        if (write_and_check(ringbuffer_context, msg[i], msg_len[i]) != 0) {
            exit(1);
        }
    }

    /* read first 2 messages and check if the messages are correct */
    for (int i = 0; i < 2; i++) {
        if (read_and_check(ringbuffer_context, msg[i], msg_len[i]) != 0) {
            exit(1);
        }
    }

    /* write one more message to buffer */
    if (write_and_check(ringbuffer_context, msg[2], msg_len[2]) != 0) {
        exit(1);
    }

    /* read one more message and check if the message is correct */
    if (read_and_check(ringbuffer_context, msg[2], msg_len[2]) != 0) {
        exit(1);
    }

    /* write the rest of the messages to buffer */
    for (int i = 3; i < 5; i++) {
        if (write_and_check(ringbuffer_context, msg[i], msg_len[i]) != 0) {
            exit(1);
        }
    }

    /* read the rest of the messages and check if the messages are correct */
    for (int i = 3; i < 5; i++) {
        if (read_and_check(ringbuffer_context, msg[i], msg_len[i]) != 0) {
            exit(1);
        }
    }

    free(rbuf);
    free(ringbuffer_context);

    printf("Test passed!\n");
    return 0;
}