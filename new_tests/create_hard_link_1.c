#include "fs/operations.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

uint8_t const file_contents[] = "test.";
char const target[] = "/f1";
char const hard_link[] = "/l1";

void assert_contents_ok(char const *path) {
    int f = tfs_open(path, 0);
    assert(f != -1);

    uint8_t buffer[sizeof(file_contents)];
    assert(tfs_read(f, buffer, sizeof(buffer)) == sizeof(buffer));
    assert(memcmp(buffer, file_contents, sizeof(buffer)) == 0);

    assert(tfs_close(f) != -1);
}

void write_contents(char const *path) {
    int f = tfs_open(path, 0);
    assert(f != -1);

    assert(tfs_write(f, file_contents, sizeof(file_contents)) ==
           sizeof(file_contents));

    assert(tfs_close(f) != -1);
}

int main() {
	tfs_params params = tfs_default_params();
	params.max_inode_count = 2;
	params.max_block_count = 2;
    assert(tfs_init(&params) != -1);

    // Create new file and write to it
    {
        int f = tfs_open(target, TFS_O_CREAT);
        assert(f != -1);
		write_contents(target);
        assert(tfs_close(f) != -1);
    }

	// Guarantee that creating a hard link does not create an inode
    assert(tfs_link(target, hard_link) != -1);

    assert_contents_ok(target);
	assert_contents_ok(hard_link);

    assert(tfs_destroy() != -1);

    printf("Successful test.\n");

    return 0;
}
