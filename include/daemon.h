#ifndef DAEMON_H
#define DAEMON_H

typedef struct {
    int from;
    int to;
    char* filename;
} connection_t;

#define MESSAGE_SIZE 128    
#define MINIMUM_PORT 0          /* this will always be 0 */
#define MAXIMUM_PORT 128
#define NUMBER_OF_PROCESSING_THREADS 4

/**
 * @brief simpledaemon
 * 
 * @param connections 
 * @param number_of_connections
 * @return int
 */
int simpledaemon(connection_t *connections, int number_of_connections);

#endif