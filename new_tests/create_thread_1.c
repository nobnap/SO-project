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

void write_contents(char const *path) {
	int f = tfs_open(path, TFS_O_CREAT);
    assert(f != -1);

	assert(tfs_write(f, file_contents, sizeof(file_contents)) == sizeof(file_contents));
    assert_contents_ok(path);

	assert(tfs_close(f) != -1);
}

void *write_thread(void *target){
	int f = *((int *)target);

	assert(tfs_write(f, file_contents, sizeof(file_contents)) == sizeof(file_contents));
    assert_contents_ok(file_path);

	return NULL;
}

void *read_thread(void *target) {
	int f = *((int *)target);

	char buffer[sizeof(file_contents)];
	ssize_t r = tfs_read(f, buffer, sizeof(buffer));
	assert(r == 0 || r == sizeof(buffer));

	return target;
}

int main() {
	pthread_t tid[2];

	assert(tfs_init(NULL) != -1);

	write_contents(file_path);

	int f = tfs_open(file_path, TFS_O_CREAT);
    assert(f != -1);

	if (pthread_create(&tid[0], NULL, write_thread, (void *)&f) != 0) {
        fprintf(stderr, "failed to create read thread: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

	if (pthread_create(&tid[1], NULL, read_thread, (void *)&f) != 0) {
        fprintf(stderr, "failed to create read thread: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    } 

	pthread_join(tid[0], NULL);
	pthread_join(tid[1], NULL);

	assert(tfs_close(f) != -1);

	assert(tfs_destroy() != -1);

	printf("Successful test.\n");

	return 0;
}