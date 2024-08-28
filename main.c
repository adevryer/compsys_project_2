#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <getopt.h>
#include <strings.h>
#include <string.h>
#include <netdb.h>
#include "commands.h"

//#define _POSIX_C_SOURCE 200112L not needed?

/*
fetchmail
-u <username> -p <password> [-f <folder>] [-n <messageNum>] [-t]
<command> <server_name>
Where <command> may be one of: retrieve, parse, mime, or list.

For the purpose of assessment, use the following predefined status codes:
1. Command-line parsing error, argument validation failure (if implemented)
2. Connection initiation errors
3. Unexpected IMAP or server responses (e.g. unexpected disconnect, random characters)
4. Parse failure (e.g. Expected header is missing (optional), text/plain part not found)
5. Other errors
*/

int connect_to_IMAP(char* server);
int confirm_connection(int connfd);
void escape_unsafe_characters(char* input, char* output);
void clean_exit();
int validate_message_num(char* str);
void validate_and_escape_string(char* src, char** dst);

// global variables for the sake of easy free-ing
int connfd;
char* username=NULL, *pass=NULL, *folder=NULL, *command=NULL, *server_name=NULL; 


int main(int argc, char* argv[]) {
    if (argc < 6) {
        printf("Invaild commandline entry, aborting\n");
        exit(COMMAND_ERROR);
    }    
    int c, msg_num=-1;
    //int TLS=FALSE;

    // getopt() uses global vars optarg and optind      
    while ((c = getopt(argc, argv, "u:p:f:n:t")) != -1) {
        switch (c) {
            case 'u':
                validate_and_escape_string(optarg, &username);
                break;
            case 'p':
                validate_and_escape_string(optarg, &pass);
                break;
            case 'f':
                // read from INBOX if not specified, and surround with "" for whitespace protection
                validate_and_escape_string(optarg, &folder);
                break;
            case 'n':
                // read the most recent message if not specified
                if (!validate_message_num(optarg)) {
                    printf("Invaild commandline entry, aborting\n");
                    exit(COMMAND_ERROR);
                }
                msg_num = atoi(optarg);
                if (msg_num <= 0) {
                    printf("Invaild commandline entry, aborting\n");
                    exit(COMMAND_ERROR);
                }
                break;
            case 't':
                fprintf(stderr, "TSL not currently supported, using regular type connection\n");
                break;
        }
    }
    int i=optind;
    if (i >= argc) {
        printf("Invaild commandline entry, aborting\n");
        exit(COMMAND_ERROR);
    }
    command = strdup(argv[i++]);
    if (i >= argc) {
        printf("Invaild commandline entry, aborting\n");
        exit(COMMAND_ERROR);
    }
    server_name = strdup(argv[i]);

    //printf("u:%s p:%s f:%s n:%d c:%s s:%s\n", username, pass, folder, msg_num, command, server_name);

    int s;
    int connfd = connect_to_IMAP(server_name);
    s = confirm_connection(connfd);
    if (s != 0) {
        fprintf(stderr, "Failed to connect\n");
        clean_exit();
        exit(CONNECTION_ERROR);
    }

    s = imap_login(connfd, username, pass);
    if (s != 0) {
        fprintf(stdout, "Login failure\n");
        clean_exit();
        exit(IMAP_ERROR);
    }

    s = select_folder(connfd, folder);
    int num_msgs = s;
    if (s < 0) {
        fprintf(stdout, "Folder not found\n");
        clean_exit();
        exit(IMAP_ERROR);
    }
    
    s = execute_command(connfd, command, msg_num, num_msgs);
    if (s == MSG_NOT_FOUND) {
        fprintf(stdout, "Message not found\n");
        clean_exit();
        exit(IMAP_ERROR);    
    } 

    clean_exit();
    return 0;
}

// Escape unsafe characters and surround string with "" for safety
void validate_and_escape_string(char* src, char** dst) {
    *dst = malloc(sizeof(char)*(2*strlen(src)));
    escape_unsafe_characters(src, *dst);
    
    int len = strlen(*dst) + 3; // 2 additional characters ("") plus null byte
    char* buffer = malloc(sizeof(char)*len);
    snprintf(buffer, len, "\"%s\"", *dst);
    
    free(*dst);
    *dst = buffer;
}

// Free all commandline strings and close connection
void clean_exit() {
    close(connfd);
    free(command);
    free(server_name);
    free(folder);
    free(username);
    free(pass);
}

/* Escapes \r and \n so commands cannot be injected, 
   output needs to be twice in size of input for worst case */
void escape_unsafe_characters(char* input, char* output) {
    int i=0;
    int j=i;
    while (input[i] != '\0') {
        if (input[i] == '\n') {
            output[j++] = '\\';
            output[j++] = 'n';
        } else if (input[i] == '\r') {
            output[j++] = '\\';
            output[j++] = 'r';
        } else {
            output[j++] = input[i];
        }
        i++;
    }
    output[j] = '\0';
}

// Ensure no non-digit characters in the message_num string
int validate_message_num(char* str) {
    int i=0;
    while (str[i] != '\0') {
        if (!isdigit(str[i++])) {
            return FALSE;
        }
    }
    return TRUE;
}

// Confirm our connection to the server by reading response
int confirm_connection(int connfd) {
    int s;
    char buffer[1024];
    s = read(connfd, buffer, 1024);
    if (s < 0) {
        fprintf(stderr, "Failed reading into buffer\n");
    }
    return confirm_response(buffer);
}

// Find connection details for given hostname and connect and return the file pointer
int connect_to_IMAP(char* server) {
    int connfd = 0, s;
    struct addrinfo hints, *results, *rp;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC; // IPV4 or IPV6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0; 

    s = getaddrinfo(server, "143", &hints, &results); // returns 0 on success
    if (s != 0) {
        fprintf(stderr, "Failed getaddrinfo = %s\n", gai_strerror(s));
        //free(results);
        clean_exit();
        exit(EXIT_FAILURE);
    }

    // cycle through all results to find first valid connect
    for (rp = results; rp!= NULL; rp=rp->ai_next) {
        connfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (connfd == -1) continue;
        if (connect(connfd, rp->ai_addr, rp->ai_addrlen) != -1) break;
        close(connfd);
    }

    freeaddrinfo(results);

    if (rp == NULL) {
        fprintf(stderr, "Failed to connect\n");
        clean_exit();
        exit(EXIT_FAILURE);
    }
    return connfd;
}