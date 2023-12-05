#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <linux/limits.h>

bool isNotValidCommand(char *userCommand);
bool invalidFileLocation(char *userCommand, char *fileLocation);
bool sValidPathandFileName(char *fileLocation);
bool invalidDir(char *fileLocation);

int openFile(char *userCommand, char *fileLocation);
void closeFile(int fileDescriptor);

void printFileContents(int fileDescriptor);
void validateFileDescriptor(int fileDescriptor);

bool cmdIsGet(char *userCommand);
bool cmdIsSet(char *userCommand);
void validateUserInput(char *userCommand, char *fileLocation);

void executeCommand(char *userCommand, int textFileDescriptor);
void writeToFile(int fileDescriptor);
void terminateProgram();
void checkExtraGetInput(char *userCommand, char *fileLocation);
