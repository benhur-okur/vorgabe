#include "../include/ringbuf.h"
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#define NUMBER_OF_STRINGS 1000
#define BUF_SIZE 50  // bytes
#define RBUF_SIZE 500  // bytes
#define WAIT_TIME 1000  // usec

int READER_ONE_COUNT = 0;
int READER_TWO_COUNT = 0;

typedef struct {
    rbctx_t *rb;
    char** strings;
    int responsible_for_from;
    int responsible_for_to;
} args_t;

typedef struct {
    rbctx_t *rb;
    char** write_result;
    int idx;
    int writing_direction;
} args_reader_t;

void *writer(void *arg)
{
    rbctx_t *rb = ((args_t *)arg)->rb;
    char** strings = ((args_t *)arg)->strings;
    int from = ((args_t *)arg)->responsible_for_from;
    int to = ((args_t *)arg)->responsible_for_to;

    for (int i = from; i < to; i++) {
        size_t str_len = strlen(strings[i]) + 1;
        while (ringbuffer_write(rb, strings[i], str_len) != SUCCESS) {
            usleep(WAIT_TIME);
        }
    }

    return NULL;
}

void *reader(void *arg)
{
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

    rbctx_t *rbctx = ((args_reader_t *)arg)->rb;
    char** write_result = ((args_reader_t *)arg)->write_result;
    int idx = ((args_reader_t *)arg)->idx;
    int writing_direction = ((args_reader_t *)arg)->writing_direction;

    unsigned char buf[BUF_SIZE];
    size_t read = BUF_SIZE;
    do {
        while (ringbuffer_read(rbctx, buf, &read) == RINGBUFFER_EMPTY) {
            read = BUF_SIZE;
            pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
            usleep(WAIT_TIME);
            pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
        }
        /* write result into array */
        assert(idx >= 0 && idx < NUMBER_OF_STRINGS);
        write_result[idx] = malloc(read);
        strncpy(write_result[idx], (char*) buf, read);
        idx += writing_direction;

        if (writing_direction == 1) {
            READER_ONE_COUNT++;
        } else {
            READER_TWO_COUNT++;
        }

        /* reset read */
        read = BUF_SIZE;
    } while(1);
    return NULL;
}

int main()
{   
    /* array of random strings */
    char* strings[NUMBER_OF_STRINGS];
    for (int i = 0; i < NUMBER_OF_STRINGS; i++) {
        int len = rand() % BUF_SIZE;
        char* str = malloc(len + 1);
        if (str == NULL) {
            printf("Error: malloc failed\n");
            exit(1);
        }
        for (int j = 0; j < len; j++) {
            str[j] = 'a' + (rand() % 26);
        }
        str[len] = '\0';
        strings[i] = str;
    }

    /* initialize ringer buffer */
    size_t rbuf_size = 500; // can hold more than one string and produces plenty of wrap arounds
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

    /****************************************************************
    * WRITER THREADS 
    * one thread writes the first half of strings from
    * strings array, the other thread writes the second half
    * ***************************************************************/

    /* prepare writer thread arguments */
    args_t w_args = {ringbuffer_context, (char**) strings, 0, (int) (NUMBER_OF_STRINGS / 2)};
    args_t w2_args = {ringbuffer_context, (char**) strings, (int) (NUMBER_OF_STRINGS / 2), NUMBER_OF_STRINGS};
    
    /* start writer threads */    
    printf("creating writer threads\n");
    pthread_t w_id, w2_id;
    pthread_create(&w_id, NULL, writer, &w_args); // first thread writes first half of strings
    pthread_create(&w2_id, NULL, writer, &w2_args); // second thread writes last half of strings

    /****************************************************************
    * READER THREADS 
    * two threads read from ringbuffer
    * one writes results into read_strings beginning from 
    * index 0 going up, the other beginning from index 
    * NUMBER_OF_STRINGS - 1 going down (if everything works correctly
    * they should never overwrite each other)
    * ***************************************************************/

    /* prepare reader thread arguments */
    char* result_strings[NUMBER_OF_STRINGS];
    args_reader_t r_args = {ringbuffer_context, (char**) result_strings, 0, 1};
    args_reader_t r2_args = {ringbuffer_context, (char**) result_strings, NUMBER_OF_STRINGS - 1, -1};

    /* start reader threads */
    printf("creating reader threads\n");
    pthread_t r_id, r2_id;
    pthread_create(&r_id, NULL, reader, &r_args); // first thread writes results into read_strings beginning from index 0 going up
    pthread_create(&r2_id, NULL, reader, &r2_args); // second thread writes results into read_strings beginning from index NUMBER_OF_STRINGS - 1 going down

    /****************************************************************
    * CLEANUP
    * ***************************************************************/

    /* join writer threads */
    printf("joining writer threads\n");
    pthread_join(w_id, NULL);
    pthread_join(w2_id, NULL);
    printf("writer threads joined\n");

    sleep(3); // give reader threads some time to finish

    /* join reader threads */
    printf("canceling reader threads and joining them\n");
    pthread_cancel(r_id);
    pthread_cancel(r2_id);
    pthread_join(r_id, NULL);
    pthread_join(r2_id, NULL);
    printf("reader threads joined\n");

    /****************************************************************
    * COMPARE RESULTS
    * ***************************************************************/

    printf("comparing results\n");
    if (READER_ONE_COUNT + READER_TWO_COUNT != NUMBER_OF_STRINGS) {
        printf("Error: the incorrect number of strings was read\n");
        exit(1);
    }
    
    for (int i = 0; i < NUMBER_OF_STRINGS; i++) {
        int found = 0;
        for (int j = 0; j < NUMBER_OF_STRINGS; j++) {
            if (strncmp(strings[i], result_strings[j], strlen(strings[i])) == 0) {
                found = 1;
                break;
            }
        }

        if (!found) {
            printf("Error: string not found in results\n");
            printf("String: %s\n", strings[i]);
            exit(1);
        }
    }

    /* free resources */
    ringbuffer_destroy(ringbuffer_context);
    free(rbuf);
    free(ringbuffer_context);

    for (int i = 0; i < NUMBER_OF_STRINGS; i++) {
        free(strings[i]);
        free(result_strings[i]);
    }

    printf("Test passed!\n");

    return 0;
}
