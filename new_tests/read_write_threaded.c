#include "fs/operations.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_THREAD_AMOUNT 50

char *file_path = "/f1";
char file_contents[] = "this is a sentence";

void assert_contents_ok(char const *target) {
    int f = tfs_open(target, 0);
    assert(f != -1);

    uint8_t buffer[sizeof(file_contents)];
    assert(tfs_read(f, buffer, sizeof(buffer)) == sizeof(buffer));
    assert(memcmp(buffer, file_contents, sizeof(buffer)) == 0);

    assert(tfs_close(f) != -1);
}

void *read_contents(void *target) {
    int f = *((int *)target);

    char buffer[sizeof(file_contents)];
	ssize_t r = tfs_read(f, buffer, sizeof(buffer));
	assert(r == 0 || r == sizeof(buffer));

	return target;
}

void *write_contents(void *target) {
    int f = *((int *)target);

    assert(tfs_write(f, file_contents, sizeof(file_contents)) == sizeof(file_contents));
    assert_contents_ok(file_path);

	return NULL;
}

int main() {

    pthread_t reader_tid[MAX_THREAD_AMOUNT];
    pthread_t writer_tid[MAX_THREAD_AMOUNT];

    assert(tfs_init(NULL) != -1);

    int f = tfs_open(file_path, TFS_O_CREAT);
    assert(f != -1);

    for(int i = 0; i < MAX_THREAD_AMOUNT; i++) {
        if(pthread_create(&writer_tid[i], NULL, write_contents, (void *)&f) != 0) {
            fprintf(stderr, "failed to create write thread: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        if(pthread_create(&reader_tid[i], NULL, read_contents, (void *)&f) != 0) {
            fprintf(stderr, "failed to create read thread: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    for(int i = 0; i < MAX_THREAD_AMOUNT; i++) {
        pthread_join(writer_tid[i], NULL);
        pthread_join(reader_tid[i], NULL);
    }

    assert(tfs_close(f) != -1);

	assert(tfs_destroy() != -1);

	printf("Successful test.\n");

    return 0;
}