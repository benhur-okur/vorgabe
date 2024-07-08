#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>

#include "../include/daemon.h"
#include "../include/ringbuf.h"

/* IN THE FOLLOWING IS THE CODE PROVIDED FOR YOU
 * changing the code will result in points deduction */

/********************************************************************
* NETWORK TRAFFIC SIMULATION:
* This section simulates incoming messages from various ports using
* files. Think of these input files as data sent by clients over the
* network to our computer. The data isn't transmitted in a single
* large file but arrives in multiple small packets. This concept
* is discussed in more detail in the advanced module:
* Rechnernetze und Verteilte Systeme
*
* To simulate this parallel packet-based data transmission, we use multiple
* threads. Each thread reads small segments of the files and writes these
* smaller packets into the ring buffer. Between each packet, the
* thread sleeps for a random time between 1 and 100 us. This sleep
* simulates that data packets take varying amounts of time to arrive.
*********************************************************************/
typedef struct {
    rbctx_t* ctx;
    connection_t* connection;
} w_thread_args_t;

void* write_packets(void* arg) {
    /* extract arguments */
    rbctx_t* ctx = ((w_thread_args_t*) arg)->ctx;
    size_t from = (size_t) ((w_thread_args_t*) arg)->connection->from;
    size_t to = (size_t) ((w_thread_args_t*) arg)->connection->to;
    char* filename = ((w_thread_args_t*) arg)->connection->filename;

    /* open file */
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "Cannot open file with name %s\n", filename);
        exit(1);
    }

    /* read file in chunks and write to ringbuffer with random delay */
    unsigned char buf[MESSAGE_SIZE];
    size_t packet_id = 0;
    size_t read = 1;
    while (read > 0) {
        size_t msg_size = MESSAGE_SIZE - 3 * sizeof(size_t);
        read = fread(buf + 3 * sizeof(size_t), 1, msg_size, fp);
        if (read > 0) {
            memcpy(buf, &from, sizeof(size_t));
            memcpy(buf + sizeof(size_t), &to, sizeof(size_t));
            memcpy(buf + 2 * sizeof(size_t), &packet_id, sizeof(size_t));
            while(ringbuffer_write(ctx, buf, read + 3 * sizeof(size_t)) != SUCCESS){
                usleep(((rand() % 50) + 25)); // sleep for a random time between 25 and 75 us
            }
        }
        packet_id++;
        usleep(((rand() % (100 -1)) + 1)); // sleep for a random time between 1 and 100 us
    }
    fclose(fp);
    return NULL;
}

/* END OF PROVIDED CODE */


/********************************************************************/

/* YOUR CODE STARTS HERE */

// 1. read functionality
// 2. filtering functionality
// 3. (thread-safe) write to file functionality

// Reader thread arguments struct
typedef struct {
    rbctx_t* ctx;
    connection_t* connections;
    FILE** file_handlers;
    int nr_of_connections;
    pthread_mutex_t* file_mutexes;
    volatile bool* running;
} r_thread_args_t;

// Mesaj filtreleme fonksiyonu
bool validate(size_t from, size_t to, unsigned char* msg, size_t msg_len) {
    if (from == to || from == 42 || to == 42 || (from + to) == 42) {
        return false;
    }

    const char* malicious = "malicious";
    size_t malicious_len = strlen(malicious);
    size_t msg_index = 0, mal_index = 0;

    while (msg_index < msg_len && mal_index < malicious_len) {
        if (msg[msg_index] == malicious[mal_index]) {
            mal_index++;
            if (mal_index == malicious_len) {
                return false;
            }
        }
        msg_index++;
    }

    return true;
}

// Reader thread fonksiyonu
void* read_packets(void* arg) {
    r_thread_args_t* args = (r_thread_args_t*) arg;
    rbctx_t* ctx = args->ctx;
    connection_t* connections = args->connections;
    int nr_of_connections = args->nr_of_connections;
    volatile bool* running = args->running;
    size_t buffer_len;
    buffer_len = MESSAGE_SIZE;
    unsigned char buf[MESSAGE_SIZE];

    while (*running) {
        while (ringbuffer_read(ctx, buf, &buffer_len) == SUCCESS) {
            size_t from, to, packet_id;
            memcpy(&from, buf, sizeof(size_t));
            memcpy(&to, buf + sizeof(size_t), sizeof(size_t));
            memcpy(&packet_id, buf + 2 * sizeof(size_t), sizeof(size_t));
            bool is_message_valid = validate(from, to, buf + 3 * sizeof(size_t), buffer_len - 3 * sizeof(size_t));

            if(!is_message_valid) {
                break;
            }

            for (int i = 0; i < nr_of_connections; i++) {
                if ((size_t) connections[i].to == to) {
                    // pthread_mutex_lock(&args->file_mutexes[i]);
                    fwrite(buf + 3 * sizeof(size_t), 1, buffer_len - 3 * sizeof(size_t), args->file_handlers[i]);
                    // pthread_mutex_unlock(&args->file_mutexes[i]);
                    usleep(((rand() % 50) + 25)); // sleep for a random time between 25 and 75 us
                }
            }
        }
        usleep(((rand() % 99) + 1)); // sleep for a random time between 25 and 75 us
    }
    return NULL;
}

/* YOUR CODE ENDS HERE */

/********************************************************************/

int simpledaemon(connection_t* connections, int nr_of_connections) {
    /* initialize ringbuffer */
    rbctx_t rb_ctx;
    size_t rbuf_size = 1024;
    void *rbuf = malloc(rbuf_size);
    if (rbuf == NULL) {
        fprintf(stderr, "Error allocation ringbuffer\n");
    }

    ringbuffer_init(&rb_ctx, rbuf, rbuf_size);

    /****************************************************************
    * WRITER THREADS
    * ***************************************************************/

    /* prepare writer thread arguments */
    w_thread_args_t w_thread_args[nr_of_connections];
    for (int i = 0; i < nr_of_connections; i++) {
        w_thread_args[i].ctx = &rb_ctx;
        w_thread_args[i].connection = &connections[i];
        /* guarantee that port numbers range from MINIMUM_PORT (0) - MAXIMUMPORT */
        if (connections[i].from > MAXIMUM_PORT || connections[i].to > MAXIMUM_PORT ||
            connections[i].from < MINIMUM_PORT || connections[i].to < MINIMUM_PORT) {
            fprintf(stderr, "Port numbers %d and/or %d are too large\n", connections[i].from, connections[i].to);
            exit(1);
        }
    }

    /* start writer threads */
    pthread_t w_threads[nr_of_connections];
    for (int i = 0; i < nr_of_connections; i++) {
        pthread_create(&w_threads[i], NULL, write_packets, &w_thread_args[i]);
    }

    /****************************************************************
    * READER THREADS
    * ***************************************************************/

    pthread_t r_threads[NUMBER_OF_PROCESSING_THREADS];

    /* END OF PROVIDED CODE */

    /********************************************************************/

    /* YOUR CODE STARTS HERE */

    // 1. think about what arguments you need to pass to the processing threads
    // 2. start the processing threads

    // Prepare arguments for reader threads
    pthread_mutex_t file_mutexes[nr_of_connections];
    FILE* file_handlers[nr_of_connections];
    for (int i = 0; i < nr_of_connections; i++) {
        char output_filename[20];
        int to = connections[i].to;
        sprintf(output_filename, "%d.txt", to);
        file_handlers[i] = fopen(output_filename, "a");
        pthread_mutex_init(&file_mutexes[i], NULL);
    }

    volatile bool running = true;

    r_thread_args_t r_thread_args[NUMBER_OF_PROCESSING_THREADS];
    for (int i = 0; i < NUMBER_OF_PROCESSING_THREADS; i++) {
        r_thread_args[i].ctx = &rb_ctx;
        r_thread_args[i].connections = connections;
        r_thread_args[i].file_handlers = file_handlers;
        r_thread_args[i].nr_of_connections = nr_of_connections;
        r_thread_args[i].file_mutexes = file_mutexes;
        r_thread_args[i].running = &running;
        pthread_create(&r_threads[i], NULL, read_packets, &r_thread_args[i]);
    }

    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

    /* YOUR CODE ENDS HERE */

    /********************************************************************/



    /* IN THE FOLLOWING IS THE CODE PROVIDED FOR YOU
     * changing the code will result in points deduction */

    /****************************************************************
     * CLEANUP
     * ***************************************************************/

    /* after 5 seconds JOIN all threads (we should definitely have received all messages by then) */
    printf("daemon: waiting for 5 seconds before canceling reading threads\n");
    sleep(5);

    printf("Cancelling read threads \n");
    for (int i = 0; i < NUMBER_OF_PROCESSING_THREADS; i++) {
        pthread_cancel(r_threads[i]);
    }
    printf("Cancelled read threads \n");

    printf("Joining write threads \n");
    /* wait for all threads to finish */
    for (int i = 0; i < nr_of_connections; i++) {
        pthread_join(w_threads[i], NULL);
    }
    printf("Joined write threads \n");

    printf("Joining read threads \n");
    /* join all threads */
    for (int i = 0; i < NUMBER_OF_PROCESSING_THREADS; i++) {
        pthread_join(r_threads[i], NULL);
    }
    printf("Joined read threads \n");

    /* END OF PROVIDED CODE */



    /********************************************************************/

    /* YOUR CODE STARTS HERE */

    for (int i = 0; i < nr_of_connections; i++) {
        pthread_mutex_destroy(&file_mutexes[i]);
        fclose(file_handlers[i]);
    }
    /* YOUR CODE ENDS HERE */

    /********************************************************************/



    /* IN THE FOLLOWING IS THE CODE PROVIDED FOR YOU
    * changing the code will result in points deduction */

    free(rbuf);
    ringbuffer_destroy(&rb_ctx);

    return 0;

    /* END OF PROVIDED CODE */
}