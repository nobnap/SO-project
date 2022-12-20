#include "fs/operations.h"
#include "fs/state.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_THREAD_AMOUNT 10

char *file_path = "/f1";
char *link_path = "/soft_link";

void *create_symlink(void *target) {
    //int inum = *((int *)target);
	(void) target;

    int f = tfs_sym_link(file_path, link_path);
    assert(f == -1);

    return NULL;
}

int main() {
	pthread_t tid[MAX_THREAD_AMOUNT];

	assert(tfs_init(NULL) != -1);

    int f = tfs_open(file_path, TFS_O_CREAT);
    assert(f != -1);

	int s = tfs_sym_link(file_path, link_path);
	assert(s == 0);

	for (int i=0; i < MAX_THREAD_AMOUNT; i++) {
		if (pthread_create(&tid[i], NULL, create_symlink, (void *)&f) != 0) {
        	fprintf(stderr, "failed to create symlink thread: %s\n", strerror(errno));
        	exit(EXIT_FAILURE);
    	}
	}

	for (int i=0; i < MAX_THREAD_AMOUNT; i++) {
		pthread_join(tid[i], NULL);
	}

	size_t len = sizeof(file_path) + 1;
	char buffer[len];
	memset(buffer, 0, len);
	inode_t *inode = inode_get(2);
	void *sym_block = data_block_get(inode->i_data_block);
	memcpy(buffer, sym_block, len);
	assert(!strcmp(buffer,file_path));

    assert(tfs_close(f) != -1);

	assert(tfs_destroy() != -1);

	printf("Successful test.\n");

	return 0;
}