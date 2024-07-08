#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include "../include/daemon.h"

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


int main() {
    connection_t connection[3] = {
        {.from = 1, .to = 11, .filename = "test/test_daemon/rndtxt1.txt"},
        {.from = 2, .to = 12, .filename = "test/test_daemon/rndtxt2.txt"},
        {.from = 3, .to = 13, .filename = "test/test_daemon/rndtxt3.txt"}
    };

    /* delete file 11.txt, 12.txt, 13.txt if they exist (from previous runs) */
    remove("11.txt");
    remove("12.txt");
    remove("13.txt");

    

    /* execute daemon */
    printf("Executing daemon! If it does not terminate (in 10s), it is likely stuck\n");
    printf("You may need to kill it manually (CTRL+C) and compare the files with 'diff'\n");
    printf("If the files are the same, you are nearly done, and have to worry only about pthread_cancel logic!\n\n");
    simpledaemon((connection_t *)connection, 3);

    /* check if correct files were created */
    printf("Checking results\n");
    FILE *fp1 = fopen("11.txt", "r");
    if (fp1 == NULL) {
        fprintf(stderr, "Error: There should be a file with name 11.txt\n");
        return 1;
    }
    fclose(fp1);

    FILE *fp2 = fopen("12.txt", "r");
    if (fp2 == NULL) {
        fprintf(stderr, "Error: There should be a file with name 12.txt\n");
        return 1;
    }
    fclose(fp2);

    FILE *fp3 = fopen("13.txt", "r");
    if (fp3 == NULL) {
        fprintf(stderr, "Error: There should be a file with name 13.txt\n");
        return 1;
    }
    fclose(fp3);

    /* check if the files have the correct content */
    if (check_files("11.txt", "test/test_daemon/rndtxt1_lsg.txt") != 0) {
        fprintf(stderr, "Error: files 11.txt and test/test_daemon/rndtxt1_lsg.txt are not the same\n");
        return 1;
    }

    if (check_files("12.txt", "test/test_daemon/rndtxt2_lsg.txt") != 0) {
        fprintf(stderr, "Error: files 12.txt and test/test_daemon/rndtxt2_lsg.txt are not the same\n");
        return 1;
    }

    if (check_files("13.txt", "test/test_daemon/rndtxt3_lsg.txt") != 0) {
        fprintf(stderr, "Error: files 13.txt and test/test_daemon/rndtxt3_lsg.txt are not the same\n");
        return 1;
    }

    printf("Test passed!\n");

    return 0;
}
