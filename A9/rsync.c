#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <utime.h>
#include <errno.h>
#include <fcntl.h>

// function to recursively remove a (non-empty) directory  
void remove_directory(char* path) {
    DIR *dir;
    struct dirent *entry;
    struct stat dir_stat;
    char fullpath[1024];

    dir = opendir(path);

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        sprintf(fullpath, "%s/%s", path, entry->d_name);
        lstat(fullpath, &dir_stat);

        if (S_ISDIR(dir_stat.st_mode)) { remove_directory(fullpath); }
        else { remove(fullpath); }
    }

    closedir(dir);
    remove(path);
}

// to recursively sync two directories
void sync_directories(char* src, char* dst){
    DIR* src_dir = opendir(src);
    DIR* dst_dir = opendir(dst);

    // check if either of them are invalid 
    if (src_dir == NULL || dst_dir == NULL) {
        if (src_dir == NULL) {
            perror("Error opening source directory");
        } else {
            perror("Error opening destination directory");
        }
        exit(1);
    }

    struct dirent *src_item, *dst_item;
    struct stat src_stat, dst_stat;
    char src_path[1024], dst_path[1024];

    // iterate through src directory
    while ((src_item = readdir(src_dir)) != NULL) {

        // ignore . and .. directories
        if (strcmp(src_item->d_name, ".") == 0 || strcmp(src_item->d_name, "..") == 0)
            continue;

        sprintf(src_path, "%s/%s", src, src_item->d_name);
        sprintf(dst_path, "%s/%s", dst, src_item->d_name);

        lstat(src_path, &src_stat);

        // setting to 0 incase dst/path/item does not exist
        if (lstat(dst_path, &dst_stat) != 0) {
            dst_stat.st_mode = 0;
        }

        // case 1: item is a regular file
        if (S_ISREG(src_stat.st_mode)) {

            // both are regular files
            if (S_ISREG(dst_stat.st_mode)) {

                // both are identical, do nothing
                if (src_stat.st_size == dst_stat.st_size && src_stat.st_mtime == dst_stat.st_mtime) {} 

                // else, copy source file to destination
                else {
                    printf("[o] %s\n", dst_path);

                    int src_fd = open(src_path, O_RDONLY);
                    int dst_fd = open(dst_path, O_WRONLY | O_CREAT | O_TRUNC, src_stat.st_mode);

                    char buf[1024];
                    ssize_t bytes_read, bytes_written;
                    while ((bytes_read = read(src_fd, buf, 1024)) > 0) {
                        bytes_written = write(dst_fd, buf, bytes_read);
                    }

                    close(src_fd);
                    close(dst_fd);

                    utime(dst_path, &(struct utimbuf){src_stat.st_atime, src_stat.st_mtime});
                    chmod(dst_path, src_stat.st_mode);
                }
            } 
            
            // create dst/path/item if it does not exist
            else if (dst_stat.st_mode == 0) {
                printf("[+] %s\n", dst_path);

                int src_fd = open(src_path, O_RDONLY);
                int dst_fd = open(dst_path, O_WRONLY | O_CREAT | O_TRUNC, src_stat.st_mode);
 
                char buf[1024];
                ssize_t bytes_read;
                while ((bytes_read = read(src_fd, buf, 1024)) > 0) {
                    write(dst_fd, buf, bytes_read);
                }

                close(src_fd);
                close(dst_fd);
            } 
            
            // dst/path/item is not a regular file, delete it and create a regular file
            else {
                printf("[-] %s\n", dst_path);
                remove(dst_path);

                // create dst/path/item
                printf("[+] %s\n", dst_path);

                int src_fd = open(src_path, O_RDONLY);
                int dst_fd = open(dst_path, O_WRONLY | O_CREAT | O_TRUNC, src_stat.st_mode);

                char buf[1024];
                ssize_t bytes_read;
                while ((bytes_read = read(src_fd, buf, 1024)) > 0) {
                    write(dst_fd, buf, bytes_read);
                }

                close(src_fd);
                close(dst_fd);
            }
        }

        // case 2: item is a directory
        else if (S_ISDIR(src_stat.st_mode)) {

            // both are directories, so recurse
            if (S_ISDIR(dst_stat.st_mode)) 
                sync_directories(src_path, dst_path);

            // create dst/path/item if it does not exist, then recurse
            else if (dst_stat.st_mode == 0) {
                printf("[+] %s\n", dst_path);
                mkdir(dst_path, src_stat.st_mode);
                sync_directories(src_path, dst_path);
            } 

            // dst/path/item is not a directory, delete it and create a directory, then recurse
            else {
                printf("[-] %s\n", dst_path);
                remove(dst_path);
                printf("[+] %s\n", dst_path);
                mkdir(dst_path, src_stat.st_mode);
                sync_directories(src_path, dst_path);
            }
        }
    }

    // now iterate through dst to see if there are any items that are not in src
    while ((dst_item = readdir(dst_dir)) != NULL) {
        if (strcmp(dst_item->d_name, ".") == 0 || strcmp(dst_item->d_name, "..") == 0)
            continue;

        sprintf(dst_path, "%s/%s", dst, dst_item->d_name);
        sprintf(src_path, "%s/%s", src, dst_item->d_name);

        lstat(dst_path, &dst_stat);

        // src/path/item does not exist, delete it
        if (lstat(src_path, &src_stat) != 0) {
            printf("[-] %s\n", dst_path);
            if (S_ISDIR(dst_stat.st_mode))
                remove_directory(dst_path);
            else
                remove(dst_path);
        }
    }

    closedir(src_dir);
    closedir(dst_dir);
}

// to recursively change timestamps and permissions if they do not match 
void change_timestamps_and_perms(char* src, char* dst){
    DIR* src_dir = opendir(src);
    DIR* dst_dir = opendir(dst);

    struct dirent *src_item, *dst_item;
    struct stat src_stat, dst_stat;
    char src_path[1024], dst_path[1024];

    while ((src_item = readdir(src_dir)) != NULL) {

        // ignore . and .. directories
        if (strcmp(src_item->d_name, ".") == 0 || strcmp(src_item->d_name, "..") == 0)
            continue;

        sprintf(src_path, "%s/%s", src, src_item->d_name);
        sprintf(dst_path, "%s/%s", dst, src_item->d_name);

        lstat(src_path, &src_stat);
        lstat(dst_path, &dst_stat);

        // case 1: item is a regular files
        if (S_ISREG(src_stat.st_mode)) {
            // both source and destination are regular files
            if (S_ISREG(dst_stat.st_mode)) {
                // change the timestamp if different
                if (src_stat.st_mtime != dst_stat.st_mtime) {
                    printf("[t] %s\n", dst_path);
                    utime(dst_path, &(struct utimbuf){src_stat.st_atime, src_stat.st_mtime});
                }

                // change the permissions if different
                if (src_stat.st_mode != dst_stat.st_mode) {
                    printf("[p] %s\n", dst_path);
                    chmod(dst_path, src_stat.st_mode);
                }
            }
        }

        // case 2: item is a directory - recurse
        else if (S_ISDIR(src_stat.st_mode)) {
            if (S_ISDIR(dst_stat.st_mode)) {
                change_timestamps_and_perms(src_path, dst_path);
            }
        }
    }
}

int main(int argc, char *argv[]) {

    if (argc != 3) {
        printf("Usage: %s <source_directory> <destination_directory>\n", argv[0]);
        exit(1);
    }

    char* src = argv[1];
    char* dst = argv[2];

    sync_directories(src, dst);
    change_timestamps_and_perms(src, dst);

    return 0;
}