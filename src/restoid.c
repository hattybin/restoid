#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h> // Ensure this header is included
#include <fcntl.h>
#include <ctype.h>
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <errno.h>

#define MAX_SNAPSHOTS 10
#define MAX_PATH 256
#ifndef DT_DIR
#define DT_DIR 4
#endif


// Mock functions
DIR *mock_opendir(const char *name) {
    if (strcmp(name, "valid_path") == 0) {
        return (DIR *)1; // Mock valid directory pointer
    }
    errno = ENOENT;
    return NULL;
}

struct dirent *mock_readdir(DIR *dirp) {
    static int count = 0;
    static struct dirent entry;

    if (count == 0) {
        entry.d_type = DT_DIR;
        strcpy(entry.d_name, "snapshot1");
        count++;
        return &entry;
    } else if (count == 1) {
        entry.d_type = DT_DIR;
        strcpy(entry.d_name, "snapshot2");
        count++;
        return &entry;
    }
    return NULL;
}

int mock_closedir(DIR *dirp) {
    return 0;
}

// Function to be tested
int list_snapshots(const char *file_path, char snapshots[MAX_SNAPSHOTS][MAX_PATH]);

// Test cases
void test_list_snapshots_directory_open_failure(void) {
    char snapshots[MAX_SNAPSHOTS][MAX_PATH];
    CU_ASSERT_EQUAL(list_snapshots("invalid_path", snapshots), 0);
}

void test_list_snapshots_no_snapshots_found(void) {
    char snapshots[MAX_SNAPSHOTS][MAX_PATH];
    CU_ASSERT_EQUAL(list_snapshots("empty_path", snapshots), 0);
}

void test_list_snapshots_snapshots_found(void) {
    char snapshots[MAX_SNAPSHOTS][MAX_PATH];
    int count = list_snapshots("valid_path", snapshots);
    CU_ASSERT_EQUAL(count, 2);
    CU_ASSERT_STRING_EQUAL(snapshots[0], "valid_path/snapshot1");
    CU_ASSERT_STRING_EQUAL(snapshots[1], "valid_path/snapshot2");
}

int main() {
    CU_initialize_registry();
    CU_pSuite suite = CU_add_suite("list_snapshots_test_suite", 0, 0);

    CU_add_test(suite, "test_list_snapshots_directory_open_failure", test_list_snapshots_directory_open_failure);
    CU_add_test(suite, "test_list_snapshots_no_snapshots_found", test_list_snapshots_no_snapshots_found);
    CU_add_test(suite, "test_list_snapshots_snapshots_found", test_list_snapshots_snapshots_found);

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();

    return 0;
}

void trim(char *str) {
    char *end;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
}

void get_file_path(char *file_path, size_t size) {
    if (strlen(file_path) == 0) {
        printf("Enter the file or folder path: ");
        if (fgets(file_path, size, stdin) != NULL) {
            trim(file_path);
        }
    }
}

// Function to list snapshots from a specified directory
int list_snapshots(const char *file_path, char snapshots[MAX_SNAPSHOTS][MAX_PATH]) {
    DIR *dir;
    struct dirent *entry;
    int count = 0;

    dir = opendir(file_path);
    if (dir == NULL) {
        perror("Failed to open directory");
        return 0;
    }

    printf("Snapshots found:\n");
    while ((entry = readdir(dir)) != NULL && count < MAX_SNAPSHOTS) {
        if (entry->d_type == DT_DIR) { // Check if it is a directory
            if (strlen(file_path) + strlen(entry->d_name) + 1 < MAX_PATH) {
                strncpy(snapshots[count], file_path, MAX_PATH - 1);
                strncat(snapshots[count], "/", MAX_PATH - strlen(snapshots[count]) - 1);
                strncat(snapshots[count], entry->d_name, MAX_PATH - strlen(snapshots[count]) - 1);
            } else {
                fprintf(stderr, "Path length exceeds maximum allowed length.\n");
                closedir(dir);
                return count;
            }
            trim(snapshots[count]);
            printf("[%d] %s\n", count, snapshots[count]);
            count++;
        }
    }
    closedir(dir);

    if (count == 0) {
        printf("No snapshots found for the given path.\n");
    }

    return count;
}

// Function to get snapshot ID
int get_snapshot_id(int total_snapshots) {
    int snapshot_id;
    char input[20];
    do {
        printf("Enter the unique ID of the snapshot to restore from: ");
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("Error reading input.\n");
            exit(1);
        }
        snapshot_id = atoi(input);
    } while (snapshot_id < 0 || snapshot_id >= total_snapshots);
    return snapshot_id;
}

// Function to confirm restore
int is_affirmative(const char *response) {
    return (strcasecmp(response, "yes") == 0 ||
            strcasecmp(response, "y") == 0 ||
            strcasecmp(response, "1") == 0);
}

void confirm_restore(char *restore_path, const char *file_path, size_t size) {
    char confirm[10];
    printf("Do you want to restore to the original location and overwrite? (yes/no): ");
    if (fgets(confirm, sizeof(confirm), stdin) != NULL) {
        trim(confirm);
        if (is_affirmative(confirm)) {
            strncpy(restore_path, file_path, size);
            restore_path[size - 1] = '\0';
        } else {
            printf("Enter the path where you want to restore the file: ");
            if (fgets(restore_path, size, stdin) != NULL) {
                trim(restore_path);
            }
        }
    }
}

void extract_snapshot_path(const char *snapshot_info, char *snapshot_path, size_t size) {
    const char *path_start = strrchr(snapshot_info, '/');
    if (path_start != NULL) {
        const char *snapshot_start = strstr(snapshot_info, "/home");
        if (snapshot_start != NULL) {
            size_t path_length = path_start - snapshot_start;
            strncpy(snapshot_path, snapshot_start, path_length < size ? path_length : size - 1);
            snapshot_path[path_length < size ? path_length : size - 1] = '\0';
        } else {
            fprintf(stderr, "Error: Unable to find snapshot path.\n");
            exit(1);
        }
    } else {
        fprintf(stderr, "Error: Invalid snapshot information format.\n");
        exit(1);
    }
}

// Function to copy files
void copy_file(const char *src, const char *dest) {
    int src_fd, dest_fd;
    char buffer[1024];
    ssize_t bytes_read;

    src_fd = open(src, O_RDONLY);
    if (src_fd < 0) {
        perror("Failed to open source file");
        exit(1);
    }

    dest_fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (dest_fd < 0) {
        perror("Failed to open destination file");
        close(src_fd);
        exit(1);
    }

    while ((bytes_read = read(src_fd, buffer, sizeof(buffer))) > 0) {
        write(dest_fd, buffer, bytes_read);
    }

    close(src_fd);
    close(dest_fd);
}

// Function to restore the file from the selected snapshot
void restore_file(const char *selected_snapshot, const char *restore_path, const char *file_path) {
    char snapshot_path[MAX_PATH];
    snprintf(snapshot_path, sizeof(snapshot_path), "%s/%s", selected_snapshot, strrchr(file_path, '/') + 1);

    char full_restore_path[MAX_PATH];
    snprintf(full_restore_path, sizeof(full_restore_path), "%s/%s", restore_path, strrchr(file_path, '/') + 1);

    printf("Restoring from snapshot: %s\n", snapshot_path);
    printf("Restoring to: %s\n", full_restore_path);

    // Copy the file
    copy_file(snapshot_path, full_restore_path);
    printf("Restore complete.\n");
}