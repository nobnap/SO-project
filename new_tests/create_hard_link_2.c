#include "fs/operations.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

char const target_path[] = "/f1";
char const link_path[] = "/l1";

void assert_empty_file(char const *path) {
    int f = tfs_open(path, 0);
    assert(f != -1);

    uint8_t buffer[20];
    assert(tfs_read(f, buffer, sizeof(buffer)) == 0);

    assert(tfs_close(f) != -1);
}

int main() {
    assert(tfs_init(NULL) != -1);

    // Open file
    int f = tfs_open(target_path, TFS_O_CREAT);
    assert(f != -1);
    assert_empty_file(target_path); // sanity check

	// attempt to unlink open file
    assert(tfs_unlink(target_path) == -1);

    assert(tfs_link(target_path, link_path) != -1);

    // now it should be possible to delete, since there is another way to
    // access the file (link_path)
    assert(tfs_unlink(target_path) != -1);

    // close the file
	assert(tfs_close(f) != -1);

    // now it should be possible to delete it permanently
    assert(tfs_unlink(link_path) != -1);

    assert(tfs_destroy() != -1);

    printf("Successful test.\n");

    return 0;
}
