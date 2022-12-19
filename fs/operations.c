#include "operations.h"
#include "config.h"
#include "state.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "betterassert.h"

static pthread_mutex_t open_lock;

tfs_params tfs_default_params() {
    tfs_params params = {
        .max_inode_count = 64,
        .max_block_count = 1024,
        .max_open_files_count = 16,
        .block_size = 1024,
    };
    return params;
}

int tfs_init(tfs_params const *params_ptr) {
    tfs_params params;
    if (params_ptr != NULL) {
        params = *params_ptr;
    } else {
        params = tfs_default_params();
    }

    if (state_init(params) != 0) {
        return -1;
    }

    pthread_mutex_init(&open_lock, NULL);

    // create root inode
    int root = inode_create(T_DIRECTORY);
    if (root != ROOT_DIR_INUM) {
        return -1;
    }

    return 0;
}

int tfs_destroy() {
    if (state_destroy() != 0) {
        return -1;
    }

    pthread_mutex_destroy(&open_lock);
    return 0;
}

static bool valid_pathname(char const *name) {
    return name != NULL && strlen(name) > 1 && name[0] == '/';
}

/**
 * Looks for a file.
 *
 * Note: as a simplification, only a plain directory space (root directory only)
 * is supported.
 *
 * Input:
 *   - name: absolute path name
 *   - root_inode: the root directory inode
 * Returns the inumber of the file, -1 if unsuccessful.
 */
static int tfs_lookup(char const *name, inode_t const *root_inode) {
    // TODO: assert that root_inode is the root directory
    if (!valid_pathname(name)) {
        return -1;
    }

    // skip the initial '/' character
    name++;

    return find_in_dir(root_inode, name);
}

int tfs_open(char const *name, tfs_file_mode_t mode) {
    // Checks if the path name is valid
    if (!valid_pathname(name)) {
        return -1;
    }

    inode_t *root_dir_inode = inode_get(ROOT_DIR_INUM);
    ALWAYS_ASSERT(root_dir_inode != NULL,
                  "tfs_open: root dir inode must exist");
    pthread_mutex_lock(&open_lock);
    int inum = tfs_lookup(name, root_dir_inode);
    size_t offset;

    if (inum >= 0) {
        // The file already exists
        pthread_mutex_unlock(&open_lock);
        inode_t *inode = inode_get(inum);
        ALWAYS_ASSERT(inode != NULL,
                      "tfs_open: directory files must have an inode");

        // Handle opening of soft links
        if (inode->i_node_type == T_SYMLINK) {
            if (inode->i_size > 0) {
                void* block = data_block_get(inode->i_data_block);
                return tfs_open((char *)block, mode);
            }
            else {
                return -1; // empty link
            }
        }
        // Truncate (if requested)
        if (mode & TFS_O_TRUNC) {
            pthread_rwlock_wrlock(&inode->i_lock);
            if (inode->i_size > 0) {
                data_block_free(inode->i_data_block);
                inode->i_size = 0;
            }
            pthread_rwlock_unlock(&inode->i_lock);
        }
        // Determine initial offset
        if (mode & TFS_O_APPEND) {
            pthread_rwlock_rdlock(&inode->i_lock);
            offset = inode->i_size;
            pthread_rwlock_unlock(&inode->i_lock);
        } else {
            offset = 0;
        }
    } else if (mode & TFS_O_CREAT) {
        // The file does not exist; the mode specified that it should be created
        // Create inode
        inum = inode_create(T_FILE);
        if (inum == -1) {
            pthread_mutex_unlock(&open_lock);
            return -1; // no space in inode table
        }

        // Add entry in the root directory
        if (add_dir_entry(root_dir_inode, name + 1, inum) == -1) {
            inode_delete(inum);
            pthread_mutex_unlock(&open_lock);
            return -1; // no space in directory
        }

        pthread_mutex_unlock(&open_lock);
        offset = 0;
    } else {
        pthread_mutex_unlock(&open_lock);
        return -1;
    }

    // Finally, add entry to the open file table and return the corresponding
    // handle
    return add_to_open_file_table(inum, offset);

    // Note: for simplification, if file was created with TFS_O_CREAT and there
    // is an error adding an entry to the open file table, the file is not
    // opened but it remains created
}

int tfs_sym_link(char const *target, char const *link_name) {
    inode_t *root_dir_inode = inode_get(ROOT_DIR_INUM);
    ALWAYS_ASSERT(root_dir_inode != NULL,
                  "tfs_open: root dir inode must exist");

    // checks if target file exists
    if (tfs_lookup(target, root_dir_inode) == -1) {
        return -1;
    }
    
    // checks if a file with the same name already exists
    if (tfs_lookup(link_name, root_dir_inode) != -1) {
        return -1;
    }

    // Create symlink inode
    int inum = inode_create(T_SYMLINK);
    if (inum == -1) {
        return -1; // no space in inode table
    }

    // Add entry in the root directory
    if (add_dir_entry(root_dir_inode, link_name+1, inum) == -1) {
        inode_delete(inum);
        return -1; // no space in directory
    }

    // Writing the target file's path in the link file's data block
    inode_t *inode = inode_get(inum);
    pthread_rwlock_wrlock(&inode->i_lock);
    if (inode->i_size == 0) {
            // If empty file, allocate new block
            int bnum = data_block_alloc();
            if (bnum == -1) {
                return -1; // no space
            }

            inode->i_data_block = bnum;
        }
    void *block = data_block_get(inode->i_data_block);
    memcpy(block, target, strlen(target)+1);
    inode->i_size += strlen(target)+1; // FIXME: maybe not strlen but not sure

    pthread_rwlock_unlock(&inode->i_lock);
    return 0;
}

int tfs_link(char const *target, char const *link_name) {
    inode_t *root_dir_inode = inode_get(ROOT_DIR_INUM);
    ALWAYS_ASSERT(root_dir_inode != NULL,
                  "tfs_link: root dir inode must exist");

    int inum = tfs_lookup(target, root_dir_inode);
    if (inum == -1) {
        return -1;
    }

    // checks if a file with the same name already exists
    if (tfs_lookup(link_name, root_dir_inode) != -1) {
        return -1;
    }

    inode_t *inode = inode_get(inum);
    if (inode->i_node_type == T_SYMLINK) {
        return -1;
    }

    if (add_dir_entry(root_dir_inode, link_name + 1, inum) == -1) {
        return -1; // no space in directory
    }

    pthread_rwlock_wrlock(&inode->i_lock);
    inode->i_links++;
    pthread_rwlock_unlock(&inode->i_lock);
    return 0;
}

int tfs_close(int fhandle) {
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        return -1; // invalid fd
    }

    remove_from_open_file_table(fhandle);

    return 0;
}

ssize_t tfs_write(int fhandle, void const *buffer, size_t to_write) {
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        return -1;
    }

    //  From the open file table entry, we get the inode
    inode_t *inode = inode_get(file->of_inumber);
    ALWAYS_ASSERT(inode != NULL, "tfs_write: inode of open file deleted");

    // Determine how many bytes to write
    size_t block_size = state_block_size();
    pthread_rwlock_rdlock(&file->of_lock);
    if (to_write + file->of_offset > block_size) {
        to_write = block_size - file->of_offset;
    }
    pthread_rwlock_unlock(&file->of_lock);

    if (to_write > 0) {
        pthread_rwlock_wrlock(&inode->i_lock);
        if (inode->i_size == 0) {
            // If empty file, allocate new block
            int bnum = data_block_alloc();
            if (bnum == -1) {
                pthread_rwlock_unlock(&inode->i_lock);
                return -1; // no space
            }

            inode->i_data_block = bnum;
        }
        pthread_rwlock_unlock(&inode->i_lock);

        void *block = data_block_get(inode->i_data_block);
        ALWAYS_ASSERT(block != NULL, "tfs_write: data block deleted mid-write");

        // Perform the actual write
        memcpy(block + file->of_offset, buffer, to_write);

        // The offset associated with the file handle is incremented accordingly
        pthread_rwlock_wrlock(&inode->i_lock);
        pthread_rwlock_wrlock(&file->of_lock);
        file->of_offset += to_write;
        if (file->of_offset > inode->i_size) {
            inode->i_size = file->of_offset;
        }
        pthread_rwlock_unlock(&file->of_lock);
        pthread_rwlock_unlock(&inode->i_lock);
    }

    return (ssize_t)to_write;
}

ssize_t tfs_read(int fhandle, void *buffer, size_t len) {
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        return -1;
    }

    // From the open file table entry, we get the inode
    pthread_rwlock_rdlock(&file->of_lock);
    inode_t *inode = inode_get(file->of_inumber);
    ALWAYS_ASSERT(inode != NULL, "tfs_read: inode of open file deleted");

    // Determine how many bytes to read
    size_t to_read = inode->i_size - file->of_offset;
    // TODO: if something fails, check this
    if (to_read > len) {
        to_read = len;
    }
    pthread_rwlock_unlock(&file->of_lock);

    if (to_read > 0) {
        pthread_rwlock_rdlock(&inode->i_lock);
        void *block = data_block_get(inode->i_data_block);
        ALWAYS_ASSERT(block != NULL, "tfs_read: data block deleted mid-read");

        // Perform the actual read
        memcpy(buffer, block + file->of_offset, to_read);
        // The offset associated with the file handle is incremented accordingly
        pthread_rwlock_wrlock(&file->of_lock);
        file->of_offset += to_read;
        pthread_rwlock_unlock(&file->of_lock);
        pthread_rwlock_unlock(&inode->i_lock);
    }

    return (ssize_t)to_read;
}

int tfs_unlink(char const *target) {
    inode_t *root_dir_inode = inode_get(ROOT_DIR_INUM);
    ALWAYS_ASSERT(root_dir_inode != NULL,
                  "tfs_unlink: root dir inode must exist");

    int inum = tfs_lookup(target, root_dir_inode);
    if (inum == -1) {
        return -1;
    }

    inode_t *inode = inode_get(inum);
    if(inode->i_links == 1 && count_open_file_entries_associated_with_inum(inum) == 1)
        return -1; // cannot proceed if the last link is still open


    if (clear_dir_entry(root_dir_inode, target + 1) == -1)
        return -1;
    
    pthread_rwlock_wrlock(&inode->i_lock);
    inode->i_links--;
    pthread_rwlock_unlock(&inode->i_lock);
    if (inode->i_links == 0) {
        inode_delete(inum);
    }
    return 0;
}

int tfs_copy_from_external_fs(char const *source_path, char const *dest_path) {
    char buffer[128];
    memset(buffer, 0, sizeof(buffer));

    FILE *file = fopen(source_path, "r");
    if (file == NULL)
        return -1;

    int fhandle = tfs_open(dest_path, TFS_O_TRUNC | TFS_O_CREAT);
    if (fhandle == -1)
        return -1;

    size_t bytes_read = 0;
    do {
        bytes_read = fread(buffer, sizeof(char), sizeof(buffer), file);
        if (ferror(file)) {
            return -1;
        }

        buffer[bytes_read] = '\0';

        ssize_t bytes_written = tfs_write(fhandle, buffer, bytes_read);
        if (bytes_written < 0) {
            return -1;
        }

    } while (bytes_read > 0);

    fclose(file);
    tfs_close(fhandle);

    return 0;
}
