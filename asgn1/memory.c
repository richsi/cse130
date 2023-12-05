#include "functions.h"

int main() {

    //char userCommand[5];
    //char fileLocation[PATH_MAX + 4];
    char textInput[PATH_MAX];
    int readBytes;
    int textLength = 0;
    int newline = 1;

    while (
        (readBytes = read(STDIN_FILENO, textInput + textLength, 1)) > 0) { //READING THE CMD & FILE
        textLength += readBytes;
        if (textInput[textLength - 1] == '\n') {
            newline = 0;
            break;
        }
    }

    if (newline == 1)
        terminateProgram();

    char *userInput = strtok(textInput, "\n"); //first line

    char *userCommand = strtok(userInput, " ");
    char *fileLocation = strtok(NULL, " \n");
    char *nullTok = strtok(NULL, "\n");

    if (!cmdIsGet(userCommand) && !cmdIsSet(userCommand))
        terminateProgram();
    //validateUserInput(userCommand, fileLocation);

    if (nullTok != NULL)
        terminateProgram();

    checkExtraGetInput(userCommand, fileLocation);

    int textFileDescriptor = openFile(userCommand, fileLocation);
    executeCommand(userCommand, textFileDescriptor);
    //close file in execute command
    return 0;
}

// FAILED:
