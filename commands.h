#ifndef COMMANDS_H
#define COMMANDS_H

#define SUCCESS 0
#define GEN_FAILURE -1
#define MSG_NOT_FOUND -2
#define CONNECTION_ISSUE -3
#define HEADER_NOT_FOUND -4

#define COMMAND_ERROR 1
#define CONNECTION_ERROR 2
#define IMAP_ERROR 3
#define PARSE_ERROR 4
#define OTHER_ERROR 5

#define BUFSIZE 512
#define LARGE_BUF 10000

#define TRUE 1
#define FALSE 0

#define GROWTHFCT 2

// Sends login command with given user and pass, and returns success or failure
int imap_login(int, char*, char*);
// not really used
int confirm_response(char* msg);
// not really used
int read_into_buffer(int connfd, char* buffer);
// Sends select command for given folder name, and returns success or failure
int select_folder(int connfd, char* folder);
// Runs the appoporite command given the commandline entry
int execute_command(int connfd, char* command, int msg_num, int num_msgs);
// Sends fetch command and stores response (on success) into a string (response)
int fetch(int connfd, int msg_num, char** response);
// Checks for a BAD or NO response from the server
int check_tag_BAD_NO(char* msg);
// Checks for a OK response from the server
int confirm_tag_OK(char* msg);
// Sends command for given header field and returns the response as a string (on success)
char* get_header_field(int connfd, int msg_num, char* field);
// Lists the subject for all emails in the folder
int list(int connfd, int num_msgs);
// Reads the amount of emails in a folder from successful folder selection response
int read_list_num(char* ptr);
// Read the MIME boundary into a string given it's surrounded by ""
char* get_quoted_boundary(char* response, int start_idx);
// Read the MIME boundary into a string given it's not surrounded by ""
char* get_unquoted_boundary(char* response, int start_idx);
// Verify the content type of the current body to see if its viable to display
int check_content_type(char* field);
// Verify the content encoding of the current body to see if its viable to display
int check_content_transfer_encoding(char* field);
// DEBUG FUNCTION, print until carraige return
void d_print_line(char* str);

char* stristr(const char* haystack, const char* needle);
char* strip_whitespace(char* str);


#endif