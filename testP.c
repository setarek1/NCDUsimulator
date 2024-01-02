#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>

#define MAX_FILES 1000
#define MAX_FILE_PATH_LENGTH 256

//struct to hold file information
struct FileInfo {
    int numFiles;
    char fileTypes[MAX_FILES][MAX_FILE_PATH_LENGTH];
    char largestFile[MAX_FILE_PATH_LENGTH];
    char smallestFile[MAX_FILE_PATH_LENGTH];
    long largestFileSize;
    long smallestFileSize;
    long finalSize;
};

//SHM: shared memory key
#define SHM_KEY 1234

//shared memory segment
int shmid;
struct FileInfo *sharedData;

//mutex lock
pthread_mutex_t lock;

//semaphore
sem_t semaphore;

//functions:
void *traverseDirectoryWithProcesses(void *path); //for main directory
void *traverseDirectoryWithThreads(void *path); 

int main(int argc, char *argv[]) {
    // if (argc != 2) {
    //     printf("Usage: %s <directory_path>\n", argv[0]);
    //     exit(EXIT_FAILURE);
    // }
    //printf("Enter Directory:")
    char *mainDirectory = "/Users/mac/Desktop/Codes/c"; //argv[1];
    printf("Main process started for folder: %s\n", mainDirectory);

    //initialize mutex
    pthread_mutex_init(&lock, NULL);

    //shared memory segment creation
    shmid = shmget(SHM_KEY, sizeof(struct FileInfo), IPC_CREAT | 0666);

    // if (shmid == -1) {
    //     perror("shmget failed");
    //     exit(EXIT_FAILURE);
    // }

    //attach shared memory
    sharedData = (struct FileInfo *)shmat(shmid, NULL, 0);

    // if (sharedData == (struct FileInfo *)(-1)) {
    //     perror("shmat failed");
    //     exit(EXIT_FAILURE);
    // }

    //initialize shared data
    sharedData->numFiles = 0;
    sharedData->largestFileSize = 0;
    sharedData->smallestFileSize = __INT_MAX__;

    //directory traversal with processes
    traverseDirectoryWithProcesses((void *)mainDirectory);

    //print results
    printf("Total number of files: %d\n", sharedData->numFiles);
    printf("File types:\n");
    for (int i = 0; i < sharedData->numFiles; ++i) {
        printf("%s\n", sharedData->fileTypes[i]);
    }
    printf("Address of the largest file: %s, Size: %ld\n", sharedData->largestFile, sharedData->largestFileSize);
    printf("Address of the smallest file: %s, Size: %ld\n", sharedData->smallestFile, sharedData->smallestFileSize);
    printf("Final size of the main folder: %ld bytes\n", sharedData->finalSize);

    //detach and remove shared memory
    shmdt(sharedData);
    shmctl(shmid, IPC_RMID, NULL);

    //destroy mutex
    pthread_mutex_destroy(&lock);

    return 0;
}

void *traverseDirectoryWithProcesses(void *path) { 
    sem_init(&semaphore, 0, 1); //initialie semaphore

    char *currentPath = (char *)path;

    DIR *dir;
    struct dirent *entry;

    if (!(dir = opendir(currentPath))) {
        perror("opendir error");
        pthread_exit(NULL);
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG){
    
            //fnmber of files++
            sharedData->numFiles++;

            //file type
            char fileType[20];
            strcpy(fileType, entry->d_name);
            strcpy(sharedData->fileTypes[sharedData->numFiles - 1], entry->d_name);

            //file size
            char filePath[256];
            snprintf(filePath, sizeof(filePath), "%s/%s", currentPath, entry->d_name);
            FILE *file = fopen(filePath, "r");
            fseek(file, 0L, SEEK_END);
            long fileSize = ftell(file);
            fseek(file, 0L, SEEK_SET);
            fclose(file);

            if (fileSize > sharedData->largestFileSize) {
                sharedData->largestFileSize = fileSize;
                strcpy(sharedData->largestFile, filePath);
            }

            if (fileSize < sharedData->smallestFileSize) {
                sharedData->smallestFileSize = fileSize;
                strcpy(sharedData->smallestFile, filePath);
            }

        printf("File(in the main process): %s, with size: %ld\n", filePath, fileSize);

        }
        if (entry->d_type == DT_DIR) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            char subFolderPath[256];
            snprintf(subFolderPath, sizeof(subFolderPath), "%s/%s", currentPath, entry->d_name);

            pid_t pid = fork();
            if (pid < 0) {
                perror("fork error");
                closedir(dir);
                //pthread_exit(NULL);
            } else if (pid == 0) { //child process
                printf("Child process created for folder: %s with pid:%ld\n", subFolderPath, getpid());
                traverseDirectoryWithThreads((void *)subFolderPath); // Second traversal with threads
                closedir(dir);
                //pthread_exit(NULL);
            } else { //parent process
                wait(NULL); //wait for child process to finish before continuing
            }
        }
    }

    closedir(dir);
    //exitProcess(NULL);
}

void *traverseDirectoryWithThreads(void *path) {
    sem_wait(&semaphore); //semaphore
    char *currentPath = (char *)path;

    DIR *dir;
    struct dirent *entry;

    if (!(dir = opendir(currentPath))) {
        perror("opendir error");
        pthread_exit(NULL);
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            pthread_mutex_lock(&lock); //lock critical section

            //number of files ++
            sharedData->numFiles++;

            //file type
            char fileType[20];
            strcpy(fileType, entry->d_name);
            strcpy(sharedData->fileTypes[sharedData->numFiles - 1], entry->d_name);

            //file size
            char filePath[256];
            snprintf(filePath, sizeof(filePath), "%s/%s", currentPath, entry->d_name);
            FILE *file = fopen(filePath, "r");
            fseek(file, 0L, SEEK_END);
            long fileSize = ftell(file);
            fseek(file, 0L, SEEK_SET);
            fclose(file);

            if (fileSize > sharedData->largestFileSize) {
                sharedData->largestFileSize = fileSize;
                strcpy(sharedData->largestFile, filePath);
            }

            if (fileSize < sharedData->smallestFileSize) {
                sharedData->smallestFileSize = fileSize;
                strcpy(sharedData->smallestFile, filePath);
            }

            sharedData->finalSize += fileSize;

            pthread_mutex_unlock(&lock); //unlock critical section
            printf("File: %s, with size: %ld\n", filePath, fileSize);

            
        } else if (entry->d_type == DT_DIR) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            char subFolderPath[256];
            snprintf(subFolderPath, sizeof(subFolderPath), "%s/%s", currentPath, entry->d_name);

            pthread_t thread;
            printf("New thread created for folder: %s with id:%ld\n", subFolderPath, pthread_self());
            pthread_create(&thread, NULL, traverseDirectoryWithThreads, (void *)subFolderPath);
            pthread_join(thread, NULL);
        }
    }
    sem_post(&semaphore); //semaphore

    closedir(dir);
    pthread_exit(NULL);

}
