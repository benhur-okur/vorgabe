#include "../include/ringbuf.h"
#include <stdio.h>
#include <unistd.h>

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define BUF_SIZE 30

int write_and_check(rbctx_t *ringbuffer_context, char *msg, size_t msg_len) {
    if (ringbuffer_write(ringbuffer_context, msg, msg_len) != SUCCESS) {
        printf("Error: write failed\n");
        return 1;
    }

    return 0;
}

void read_available(rbctx_t *ringbuffer_context, FILE *fp_dst, size_t nr_of_msgs_to_read) {
    unsigned char buf[BUF_SIZE];
    size_t buf_len = BUF_SIZE;
    for (size_t i = 0; i < nr_of_msgs_to_read; i++) {
        buf_len = BUF_SIZE;
        assert(ringbuffer_read(ringbuffer_context, buf, &buf_len) == SUCCESS);
        fwrite(buf, sizeof(*buf), buf_len, fp_dst);
    }
}

int check_files(const char *file1, const char *file2) {
    FILE *fp1 = fopen(file1, "r");
    if (fp1 == NULL) {
        fprintf(stderr, "Cannot open file with name %s\n", file1);
        return 1;
    }
    FILE *fp2 = fopen(file2, "r");
    if (fp2 == NULL) {
        fprintf(stderr, "Cannot open file with name %s\n", file2);
        return 1;
    }

    int c1, c2;
    while ((c1 = fgetc(fp1)) != EOF) {
        c2 = fgetc(fp2);
        if (c1 != c2) {
            fclose(fp1);
            fclose(fp2);
            return 1;
        }
    }

    fclose(fp1);
    fclose(fp2);
    return 0;
}

int main( int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "Too few arguments. Usage %s src_file dst_file\n", argv[0]);
        exit(1);
    }

    /* open source and destination files */
    FILE *fp_src = fopen(argv[1], "r");
    if (fp_src == NULL) {
        fprintf(stderr, "Cannot open file with name %s\n", argv[1]);
    }
    FILE *fp_dst = fopen(argv[2], "w");
    if (fp_dst == NULL) {
        fprintf(stderr, "Cannot open file with name %s\n", argv[1]);
    }

    /* get source file size */
    fseek(fp_src, 0, SEEK_END);
    long file_src_size = ftell(fp_src);
    if (file_src_size < 5 * BUF_SIZE) {
        printf("Error: file size is too small - it should be at least 5 times BUF_SIZE=%d characters long\n", BUF_SIZE);
        exit(1);
    }
    fseek(fp_src, 0, SEEK_SET);

    /* initialize ringbuffer */
    size_t rbuf_size = MAX((size_t)(BUF_SIZE + 2 * sizeof(size_t)), (size_t) (file_src_size / 4));  // buffer is atleast maximum message length (BUF_LENGTH) + 2 * length of header long, for longer files it is 1/4 of the file size
                                                                                                    // this should trigger plenty of wrap arounds
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

    /* write to ringbuffer in random sized chunks */
    unsigned char buf[BUF_SIZE];
    size_t nr_of_msgs_written = 0;
    size_t nr_of_bytes_written = 0;
    size_t read = 1, buf_len;
    while (read > 0) {
        buf_len = (rand() % BUF_SIZE) + 1; // random size between 1 and BUF_SIZE
        read = fread(buf, sizeof(*buf), buf_len, fp_src);
        
        /* check if write would fail, if so read available messages */
        if (nr_of_bytes_written + (read + sizeof(size_t)) >= rbuf_size) {
            read_available(ringbuffer_context, fp_dst, nr_of_msgs_written);
            nr_of_msgs_written = 0;
            nr_of_bytes_written = 0;
        }

        /* write to buffer */
        if (read > 0) {
            if (ringbuffer_write(ringbuffer_context, (char*) buf, read) != SUCCESS) {
                printf("Error: write failed\n");
                exit(1);
            }
            nr_of_msgs_written++;
            nr_of_bytes_written += read + sizeof(size_t);
        } 
    }

    /* read last part of the buffer */
    read_available(ringbuffer_context, fp_dst, nr_of_msgs_written);

    fclose(fp_src);
    fclose(fp_dst);

    /* check if the files are the same */
    if (check_files(argv[1], argv[2]) != 0) {
        printf("Error: files are not the same\n");
        exit(1);
    } 

    ringbuffer_destroy(ringbuffer_context);
    free(rbuf);
    free(ringbuffer_context);

    printf("Test passed! Files are the same\n");

    return 0;
}
