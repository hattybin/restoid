#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>

#define MAX_PATH 1024
#define MAX_SNAPSHOTS 100

void trim(char *str) {
    char *end;
    while(isspace((unsigned char)*str)) str++;
    if(*str == 0) return;
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
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

int list_snapshots(const char *file_path, char snapshots[MAX_SNAPSHOTS][MAX_PATH]) {
    char command[MAX_PATH * 2];
    snprintf(command, sizeof(command), "findoid \"%s\" 2>/dev/null", file_path);
    FILE *fp = popen(command, "r");

    if (fp == NULL) {
        perror("Failed to run findoid");
        exit(1);
    }

    int count = 0;
    printf("Snapshots found:\n");
    while (fgets(snapshots[count], MAX_PATH, fp) != NULL && count < MAX_SNAPSHOTS) {
        trim(snapshots[count]);
        printf("[%d] %s\n", count, snapshots[count]);  // Add newline here
        count++;
    }
    pclose(fp);

    if (count == 0) {
        printf("No snapshots found for the given path.\n");
        exit(1);
    }

    return count;
}

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

void restore_file(const char *selected_snapshot, const char *restore_path, const char *file_path) {
    char snapshot_path[MAX_PATH];
    extract_snapshot_path(selected_snapshot, snapshot_path, sizeof(snapshot_path));

    char full_snapshot_path[MAX_PATH * 2];
    snprintf(full_snapshot_path, sizeof(full_snapshot_path), "%s/%s", snapshot_path, strrchr(file_path, '/') + 1);

    char command[MAX_PATH * 3];
    printf("Restoring from snapshot: %s\n", full_snapshot_path);
    printf("Restoring to: %s\n", restore_path);

    snprintf(command, sizeof(command), "cp -R \"%s\" \"%s\"", full_snapshot_path, restore_path);
    printf("Executing command: %s\n", command);
    
    int result = system(command);
    if (result == 0) {
        printf("Restore complete.\n");
    } else {
        printf("Error occurred during restore. Please check permissions and paths.\n");
    }
}

int main(int argc, char *argv[]) {
    char file_path[MAX_PATH] = "";
    char restore_path[MAX_PATH] = "";
    char snapshots[MAX_SNAPSHOTS][MAX_PATH];

    if (argc > 1) {
        strncpy(file_path, argv[1], sizeof(file_path) - 1);
        file_path[sizeof(file_path) - 1] = '\0';
    }
    get_file_path(file_path, sizeof(file_path));

    int total_snapshots = list_snapshots(file_path, snapshots);
    int snapshot_id = get_snapshot_id(total_snapshots);
    confirm_restore(restore_path, file_path, sizeof(restore_path));
    restore_file(snapshots[snapshot_id], restore_path, file_path);

    return 0;
}
