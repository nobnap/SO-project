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

#define THREADS 10

char const file_path[] = "/f";
char const file_contents[] = "this is a sentence";

void assert_contents_ok(char const *path, char const *file) {
    int f = tfs_open(path, 0);
    assert(f != -1);

    char buffer[sizeof(file)];
    assert(tfs_read(f, buffer, sizeof(buffer)) == sizeof(buffer));
    assert(memcmp(buffer, file, sizeof(buffer)) == 0);

    assert(tfs_close(f) != -1);
}

void *alloc_inode(void *num) {
	int n = *((int *)num);

	char path[sizeof(file_path)+1];
	memset(path, 0, sizeof(path));
	strcpy(path, file_path);
	char content[sizeof(file_contents)+1];
	memset(content, 0, sizeof(content));
	strcpy(content, file_contents);

	sprintf(path+2, "%i", n);
	sprintf(content+18, "%i", n);

	int f = tfs_open(path, TFS_O_CREAT);
    assert(f != -1);

    assert(tfs_write(f, content, sizeof(content)) == sizeof(content));

    assert(tfs_close(f) != -1);

	assert_contents_ok(path, content);

	return NULL;
}

int main() {
	pthread_t tid[THREADS];
	int nums[THREADS];

	assert(tfs_init(NULL) != -1);

	for (int i=0; i < THREADS; i++) {
		nums[i] = i;
		if (pthread_create(&tid[i], NULL, alloc_inode, (void *)&nums[i]) != 0) {
       		fprintf(stderr, "failed to create thread: %s\n", strerror(errno));
       		exit(EXIT_FAILURE);
    	}
	}

	for (int i=0; i < THREADS; i++) {
		pthread_join(tid[i], NULL);
	}

	assert(tfs_destroy() != -1);

	printf("Successful test.\n");
	
	return 0;
}