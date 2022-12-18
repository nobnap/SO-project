#include "fs/operations.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

char *file_contents = "this is a string.";
char *file_path = "/f1";
char *link_path = "/l1";
char *path_src = "new_tests/small_file.txt";

void assert_contents_ok(char const *path) {
    int f = tfs_open(path, TFS_O_CREAT);
    assert(f != -1);

    char buffer[40];
	ssize_t w = tfs_read(f, buffer, sizeof(buffer));
	assert(w == strlen(file_contents));
    assert(memcmp(buffer, file_contents, strlen(file_contents)) == 0);

    assert(tfs_close(f) != -1);
}

int main() {
	assert(tfs_init(NULL) != -1);

	// Creates file and sym link to it
	int f = tfs_open(file_path, TFS_O_CREAT);
	assert(f != -1);
	assert(tfs_sym_link(file_path, link_path) != -1);

	tfs_close(f);

	// Attempts to copy file to sym link
	assert(tfs_copy_from_external_fs(path_src, link_path) != -1);

	// Checks if content was properly copied
    assert_contents_ok(file_path);
	assert_contents_ok(link_path);

    printf("Successful test.\n");

    return 0;
}
