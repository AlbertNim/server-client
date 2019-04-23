/******************************************************************************
 * Author: Albert Nim
 * Date: 03/07/18
 * Assignment: a server written in C that allows for the connection to a client and the transfer of files
 *****************************************************************************/
/******************************************************************************
* Resources
* ********* https://beej.us/guide/bgnet/output/html/multipage/index.html
* ********* http://www.linuxhowtos.org/C_C++/socket.htm
* ********* http://www.linuxhowtos.org/data/6/server.c
* ********* https://forgetcode.com/C/1201-File-Transfer-Using-TCP
* ********* http://stackoverflow.com/questions/4204666/how-to-list-files-in-a-directory-in-a-c-program
******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>

// function that starts up the server and listens for connections
// utilizes much from https://beej.us/guide/bgnet/output/html/multipage/index.html
int startUp(int portno) {
    int sockfd;
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        return -1;
    }
// from beej's guide and previous project 
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(portno);
    server.sin_addr.s_addr = INADDR_ANY;
//sets up the sockets using values
    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
//if unable to bind of listen, then 
    if(bind(sockfd, (struct sockaddr *) &server, sizeof(server)) < 0){
        fprintf(stderr, "Unable to bind!\n");
        exit(1);
    }
    if(listen(sockfd, 10) < 0){
        fprintf(stderr, "Unable to listen!\n");
        exit(1);
    }

    return sockfd;
}


//sends a message to the client that has connected
void sendMessage(int sock, char* buffer) {
//sets up the buffer and the message sizes 
    ssize_t n;
    size_t size = strlen(buffer) + 1;
    size_t total = 0;
//loops while the message needs to be sent
    while (total < size) {
        n = write(sock, buffer, size - total);

        total += n;
//exits out the program in case we reach negative (sock, buffer, size - total)
        if (n < 0) {
        fprintf(stderr, "Error sending message!\n");
        exit(1);
        }
//otherwise, keep on going down message
        else if (n == 0) {
            total = size - total;
        }
    }
}

// Recieves a message from the client
void receiveMessage(int sock, char* buffer, size_t size) {
//sets up variables that store the string and count the size 
    char tempBuffer[size + 1];
    ssize_t n;
    size_t total = 0;
//continously grabs the char from the client
    while (total < size) {
        n = read(sock, tempBuffer + total, size - total);
        total += n;
// error validation 
        if (n < 0){
            fprintf(stderr, "Error receiving message!\n");
            exit(1);
        }
    }
    strncpy(buffer, tempBuffer, size);
}

//gets the contents of the directory and the number of files/size
int getDirectory(char* path[]) {
//creates pointer and variables for number of files within 
    DIR *d;
    struct dirent *dir;
    int totalSize = 0;
    int totalFiles = 0;
//opens the directory for 
    d = opendir(".");
    if (d) {
        int i = 0;
        while ((dir = readdir(d)) != NULL) {
//adds the chars to the pointer in a loop
            if (dir->d_type == DT_REG) {
                path[i] = dir->d_name;
                totalSize += strlen(path[i]);
                i++;
            }
        }
        totalFiles = i - 1;
    }
    closedir(d);
    return totalSize + totalFiles;
}

// function that reads the contents of a file into a buffer.
char* readFile(char* fileName) {
    char *source = NULL;
    FILE* fp = fopen(fileName, "r");
//null pointer means that the file was not present
    if (fp == NULL) {
        fprintf(stderr, "Unable to open file!\n");
        exit(1);
    }
//file pointer was found
    if (fp != NULL) {
        if (fseek(fp, 0L, SEEK_END) == 0) {
            long bufSize = ftell(fp);
//file size too large
            if (bufSize == -1) {
                fprintf(stderr, "File not valid!\n");
                exit(1);
            }
// Allocate our buffer to that size. 
            source = malloc(sizeof(char) * (bufSize + 1));

// Go back to the start of the file. 
            if (fseek(fp, 0L, SEEK_SET) != 0) {
                fprintf(stderr, "Unable to read file!\n");
                exit(1);
            }

// stores the file into memory 
            size_t newLen = fread(source, sizeof(char), bufSize, fp);
            if ( ferror( fp ) != 0 ) {
                fputs("Error reading file", stderr);
            } else {
                source[newLen++] = '\0';
            }
        }
    }
    fclose(fp);
    return source;
}

// receives a number from the client
int receiveNumber(int sock) {
    int num;
    ssize_t n = 0;
    n = read(sock, &num, sizeof(int));
// error detection
    if (n < 0) {
            fprintf(stderr, "Socket not receivable!\n");
            exit(1);
    }
    return (num);
}

//sends a number to the client
void sendNumber(int sock, int num) {
    ssize_t n = 0;

    n = write(sock, &num, sizeof(int));
// error detection 
    if (n < 0) {
            fprintf(stderr, "Unable to send through socket!\n");
            exit(1);
    }
}

//handles the initial request from the client
int handleRequest(int sock, int* dataPort) {
    char command[3] = "\0";

//utilizes created functions to get request values
    receiveMessage(sock, command, 3);
    *dataPort = receiveNumber(sock);
//returns number based on the command
    if (strcmp(command, "-l") == 0) {
        return 1;
    }

    if (strcmp(command, "-g") == 0) {
        return 2;
    }
//otherwise return false
    return 0;
}

 //function that is used to send our files to client
 // takes in a socket and pointer to file
void sendFile(int sock, char* fileName) {
    char* toSend;
    toSend = readFile(fileName);

    sendNumber(sock, strlen(toSend));
    sendMessage(sock, toSend);
}


// main file for the server
int main(int argc, char *argv[]) {

//sets up the arguments 
    int sockfd, newsockfd, datasockfd, portno, pid;

    if(argc != 2){
        fprintf(stderr, "Input only the port number.\n");
        exit(1);
    }
//checks if the ports are able to be used
    portno = atoi(argv[1]);
    if (portno < 1024 || portno > 65535) {
        fprintf(stderr, "Invalid port number\n");
        exit(1);
    }

//Set up initial socket
    sockfd = startUp(portno);
    printf("Server open on %d\n", portno);
//loops while receiving the inputs
    while(1) {
        newsockfd = accept(sockfd, NULL, NULL);
        if(newsockfd < 0) {
            fprintf(stderr, "error on accept\n");
            exit(1);
        }
        pid = fork();
        if (pid < 0) {
            fprintf(stderr, "error on accept\n");
            exit(1);
        }
        if (pid == 0) {
            close(sockfd);
            int command = 0;
            int dataPort;
            int newsock;
//starts the connection
            printf("Control connection started on port %d.\n", portno);
            command = handleRequest(newsockfd, &dataPort);

// sends the contents of the directory to the client
            if (command == 1) {
                char* path[100];
                int i = 0;
                int length = 0;
                printf("List directory requested on port %d.\n", dataPort);
                length = getDirectory(path);
// creates the socket if possible 
                newsock = startUp(dataPort);
                datasockfd = accept(newsock, NULL, NULL);
                if (datasockfd < 0) {
                    fprintf(stderr, "Unable to open data socket\n");
                    exit(1);
                }
// sends the names of the contents while there are still files
                sendNumber(datasockfd, length);
                while (path[i] != NULL) {
                    sendMessage(datasockfd, path[i]);
                    i++;
                }
                close(newsock);
                close(datasockfd);
                exit(0);
            }

// sends the file to the client if returned value is right
            if (command == 2) {
                int i = receiveNumber(newsockfd);
                char fileName[255] = "\0";
                receiveMessage(newsockfd, fileName, i);
                printf("File \"%s\" requested on port %d\n", fileName, dataPort);
// checks if the file is within the directory
                if (access(fileName, F_OK) == -1) {
                    printf("File not found. Sending error message on port %d\n", portno);
                    char errorMessage[] = "FILE NOT FOUND!";
// sends the message and number 
                    sendNumber(newsockfd, strlen(errorMessage));
                    sendMessage(newsockfd, errorMessage);
// closes the socket
                    close(newsock);
                    close(datasockfd);
                    exit(1);
                }
                else {
                    char message[] = "FOUND!";
                    sendNumber(newsockfd, strlen(message));
                    sendMessage(newsockfd, message);
                }
                printf("Sending \"%s\" on port %d\n", fileName, dataPort);
// sets up a new socket
                newsock = startUp(dataPort);
                datasockfd = accept(newsock, NULL, NULL);
                if (datasockfd < 0) {
                    fprintf(stderr, "Unable to open data socket\n");
                    exit(1);
                }
// sends the file to the client and closes the socket afterwards
                sendFile(datasockfd, fileName);
                close(newsock);
                close(datasockfd);
                exit(0);
            }
            exit(0);
        }

    }
}