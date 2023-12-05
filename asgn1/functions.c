#include "functions.h"

//HELPER FUNCTIONS

//USER INPUT STUFF

bool cmdIsGet(char *userCommand) {
    if (strcmp(userCommand, "get") != 0)
        return false;
    return true;
}

bool cmdIsSet(char *userCommand) {
    if (strcmp(userCommand, "set") != 0)
        return false;
    return true;
}

bool isNotValidCommand(char *userCommand) {
    if (!cmdIsGet(userCommand) && !cmdIsSet(userCommand))
        return true;
    return false;
}

bool sValidPathAndFileName(char *fileLocation) {
    char *fileCopy = strdup(fileLocation);
    // char* txt = strstr(fileCopy, ".txt");
    strcat(fileCopy, "/..");

    if (access(fileCopy, F_OK))
        return true;
    return false;
}

bool invalidDir(char *fileLocation) {
    struct stat stats;
    if (stat(fileLocation, &stats) == 0 && S_ISDIR(stats.st_mode))
        return false;
    return true;
}

bool invalidFileLocation(char *userCommand, char *fileLocation) {
    struct stat stats;
    stat(fileLocation, &stats);

    //edge case
    if (cmdIsSet(userCommand) && (fileLocation == NULL))
        return true;

    // if ((cmdIsGet(userCommand) && S_ISREG(stats.st_mode)) ||
    //     (cmdIsSet(userCommand) && sValidPathAndFileName(fileLocation))) return false;
    if (S_ISREG(stats.st_mode))
        return false;

    return true;
}

void validateUserInput(char *userCommand, char *fileLocation) {
    if (isNotValidCommand(userCommand) || invalidFileLocation(userCommand, fileLocation)) {

        fprintf(stderr, "Invalid Command\n");
        exit(1);
    }
}

void executeCommand(char *userCommand, int textFileDescriptor) {
    if (textFileDescriptor == -1)
        terminateProgram();

    if (cmdIsGet(userCommand)) {
        printFileContents(textFileDescriptor);
    } else if (cmdIsSet(userCommand)) {
        writeToFile(textFileDescriptor);
    }

    closeFile(textFileDescriptor);
    exit(0);
}

//FILE MANIPULATION
int openFile(char *userCommand, char *fileLocation) {
    int fd = 0;

    if (cmdIsGet(userCommand)) {
        fd = open(fileLocation, O_RDONLY, 0666);
        validateFileDescriptor(fd);
        // if (fd == -1){
        //     fprintf(stderr, "Operation Failed\n");
        //     exit(1);
        // }
    } else if (cmdIsSet(userCommand)) {
        //fd = creat(fileLocation, S_IRWXO);
        fd = open(fileLocation, O_WRONLY | O_TRUNC | O_CREAT, 0666);
        validateFileDescriptor(fd);
    }
    return fd;
}

void closeFile(int fileDescriptor) {
    int fd = close(fileDescriptor);
    if (fd != 0) {
        exit(1);
    }
}

void printFileContents(int fileDescriptor) {
    int writtenBytes, readBytes;
    char buf[PATH_MAX];

    while ((readBytes = read(fileDescriptor, buf, PATH_MAX)) > 0) {
        writtenBytes = 0;
        if (readBytes < 1)
            break;
        while (writtenBytes < readBytes) {
            writtenBytes += write(STDOUT_FILENO, buf + writtenBytes, readBytes - writtenBytes);
        }
    }
}

void writeToFile(int fileDescriptor) {
    int writtenBytes, readBytes;
    char buf[PATH_MAX + 4];

    while ((readBytes = read(STDIN_FILENO, buf, PATH_MAX)) > 0) {
        writtenBytes = 0;
        if (readBytes < 1)
            break;
        while (writtenBytes < readBytes) {
            writtenBytes += write(fileDescriptor, buf + writtenBytes, readBytes - writtenBytes);
        }
    }

    printf("OK\n");
}

void validateFileDescriptor(int fileDescriptor) {
    if (fileDescriptor == -1) {
        terminateProgram();
    }
}

void terminateProgram() {
    fprintf(stderr, "Invalid Command\n");
    exit(1);
}

void checkExtraGetInput(char *userCommand, char *fileLocation) {
    int readBytes;
    char buf[PATH_MAX + 4];

    if (cmdIsGet(userCommand)) {
        validateUserInput(userCommand, fileLocation);
        while ((readBytes = read(STDIN_FILENO, buf, PATH_MAX)) > 0) {
            if (readBytes > 0) {
                terminateProgram();
            }
        }
    }
}
