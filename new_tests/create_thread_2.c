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

typedef struct info{
	char const *file;
	char const *content;
}* args;

char const path1[] = "/f1";
char const path2[] = "/f2";
char const path3[] = "/f3";
char const file_contents1[] = "this is a sentence1";
char const file_contents2[] = "this is a sentence2";
char const file_contents3[] = "this is a sentence3";

struct info t1 = {path1, file_contents1};
struct info t2 = {path2, file_contents2};
struct info t3 = {path3, file_contents3};
args test1 = &t1;
args test2 = &t2;
args test3 = &t3;


void assert_contents_ok(char const *path, char const *file) {
    int f = tfs_open(path, 0);
    assert(f != -1);

    char buffer[sizeof(file)];
    assert(tfs_read(f, buffer, sizeof(buffer)) == sizeof(buffer));
    assert(memcmp(buffer, file, sizeof(buffer)) == 0);

    assert(tfs_close(f) != -1);
}

void *alloc_inode(void *info) {
	args thing = (args)info;

	char const *path = thing->file;
	size_t len = strlen(thing->content)+1;
	char file_contents[len];
	strcpy(file_contents, thing->content);

	assert(!strcmp(path, thing->file));
	assert(!strcmp(file_contents, thing->content));

	int f = tfs_open(path, TFS_O_CREAT);
    assert(f != -1);

    assert(tfs_write(f, file_contents, sizeof(file_contents)) == sizeof(file_contents));

    assert(tfs_close(f) != -1);
	return NULL;
}

int main() {
	int num = 3;
	pthread_t tid[num];

	assert(tfs_init(NULL) != -1);

	if (pthread_create(&tid[0], NULL, alloc_inode, (void *)test1) != 0) {
       	fprintf(stderr, "failed to create thread: %s\n", strerror(errno));
       	exit(EXIT_FAILURE);
    }
	if (pthread_create(&tid[1], NULL, alloc_inode, (void *)test2) != 0) {
       	fprintf(stderr, "failed to create thread: %s\n", strerror(errno));
       	exit(EXIT_FAILURE);
    }
	if (pthread_create(&tid[2], NULL, alloc_inode, (void *)test3) != 0) {
       	fprintf(stderr, "failed to create thread: %s\n", strerror(errno));
       	exit(EXIT_FAILURE);
    }

	pthread_join(tid[0], NULL);
	pthread_join(tid[1], NULL);
	pthread_join(tid[2], NULL);

	assert_contents_ok(path1, file_contents1);
	assert_contents_ok(path2, file_contents2);
	assert_contents_ok(path3, file_contents3);

	assert(tfs_destroy() != -1);

	printf("Successful test.\n");
	
	return 0;
}