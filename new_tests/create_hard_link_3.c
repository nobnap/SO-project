#include "../fs/operations.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main() {
    const char *file_path = "/f1";
    const char *link_path = "/l1";

    tfs_params params = tfs_default_params();
    params.max_inode_count = 3;
    params.max_block_count = 3;
    assert(tfs_init(&params) != -1);

    int fd = tfs_open(file_path, TFS_O_CREAT);
    assert(fd != -1);
    assert(tfs_close(fd) != -1);

    // Remove original file
    assert(tfs_unlink(file_path) != -1);

    // Tries to create hard-link to nonexistent file
    assert(tfs_link(link_path, file_path) == -1);

    printf("Successful test.\n");
}
