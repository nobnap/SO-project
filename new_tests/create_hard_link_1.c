#include "fs/operations.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

char const target[] = "/f1";
char const repeated_name[] = "/f2";

int main() {
    assert(tfs_init(NULL) != -1);

    // Create new file named 'f1'
    {
        int f = tfs_open(target, TFS_O_CREAT);
        assert(f != -1);
        assert(tfs_close(f) != -1);
    }

    // Create new file named 'f2'
    {
        int f = tfs_open(repeated_name, TFS_O_CREAT);
        assert(f != -1);
        assert(tfs_close(f) != -1);
    }

    // Attempts to create hard link named 'f2'
    assert(tfs_link(target, repeated_name) == -1);

    assert(tfs_destroy() != -1);

    printf("Successful test.\n");

    return 0;
}
