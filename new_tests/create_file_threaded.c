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

#define MAX_THREAD_AMOUNT 16

char *file_path = "/f1";

void *create_file(void *target) {
    (void) target;

    int f = tfs_open(file_path, TFS_O_CREAT);
    assert(f != -1);

    return NULL;
}

int main() {
	pthread_t tid[MAX_THREAD_AMOUNT];

	assert(tfs_init(NULL) != -1);

	for (int i=0; i < MAX_THREAD_AMOUNT; i++) {
		if (pthread_create(&tid[i], NULL, create_file, NULL) != 0) {
        	fprintf(stderr, "failed to create file thread: %s\n", strerror(errno));
        	exit(EXIT_FAILURE);
    	}
	}

	for (int i=0; i < MAX_THREAD_AMOUNT; i++) {
		pthread_join(tid[i], NULL);
	}

    for (int i=0; i < MAX_THREAD_AMOUNT; i++) {
		assert(tfs_close(i) != -1);
	}

	assert(tfs_destroy() != -1);

	printf("Successful test.\n");

	return 0;
}