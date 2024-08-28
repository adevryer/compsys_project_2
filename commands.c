#include "commands.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <getopt.h>
#include <strings.h>
#include <string.h>
#include <netdb.h>
#include <ctype.h>

int tag = 48;

int imap_login(int connfd, char* username, char* password) {
    int s;
    
    char cmd[BUFSIZE];
    snprintf(cmd, BUFSIZE, "%c LOGIN %s %s\r\n", tag, username, password);
    
    s = write(connfd, cmd, strlen(cmd));
    if (s==-1) {
        fprintf(stderr, "Failed to write to socket stream\n");
        return CONNECTION_ISSUE;
    }
    
    char buffer[BUFSIZE+1];
    s = read(connfd, buffer, BUFSIZE);
    if (s < 0) {
        fprintf(stderr, "Failed to read after login\n");
        return CONNECTION_ISSUE;
    }
    buffer[s] = '\0';
    if (confirm_tag_OK(buffer)) {
        tag++;
        return SUCCESS;
    }
    tag++;
    return GEN_FAILURE;
}

int select_folder(int connfd, char* folder) {
    // default folder is INBOX        
    if (folder == NULL) folder = "INBOX";

    int s;

    char cmd[BUFSIZE];
    snprintf(cmd, BUFSIZE, "%c SELECT %s\r\n", tag, folder);

    s = write(connfd, cmd, strlen(cmd));
    if (s==-1) {fprintf(stderr, "Failed to write to socket stream\n");}

    char buffer[BUFSIZE+1];
    s = read(connfd, buffer, BUFSIZE);
    if (s < 0) {
        fprintf(stderr, "Failed to read after selecting folder\n");
        return CONNECTION_ISSUE;
    }
    buffer[s] = '\0';
    if (confirm_tag_OK(buffer)) {
        tag++;
        char* ptr = stristr(buffer, "EXISTS");
        int num_msgs = read_list_num(ptr);
        return num_msgs;
    }
    tag++;
    return GEN_FAILURE;
}

int send_fetch_body_cmd(int connfd, int msg_num) {
    int latest_msg = FALSE, s;
    if (msg_num == -1) latest_msg = TRUE; 

    char cmd[BUFSIZE];
    if (!latest_msg) snprintf(cmd, BUFSIZE, "%c FETCH %d BODY.PEEK[]\r\n", tag, msg_num);
    else snprintf(cmd, BUFSIZE, "%c FETCH * BODY.PEEK[]\r\n", tag);
    
    s = write(connfd, cmd, strlen(cmd));
    if (s==-1) {
        fprintf(stderr, "Failed to write to socket stream\n");
        return CONNECTION_ISSUE;
    }
    return SUCCESS;
}

int read_message_size(char* buffer, int* index, int s) {
    int i = *index;
    int size = 0;
    if (check_tag_BAD_NO(buffer)) return MSG_NOT_FOUND; // MORE SPECFIC FAILURE?
          
    if (buffer[0] == '*') { // successful command
        for (; i<s; i++) {
            if (buffer[i] == '}') { // storing the size of the message
                int j=i-1;
                int magitude = 1;
                while (buffer[j] != '{') { // for each digit, times it by the current decimal place then move up one
                    size += (buffer[j]-'0') * magitude;
                    magitude *= 10; 
                    j--;
                }
                fprintf(stderr, "Message has %d bytes\n", size);
                *index = 3 + i; // right bracket + carriage return + line feed
                return size;
            }
        }
    }
    return GEN_FAILURE;
}

int fetch(int connfd, int msg_num, char** rspn) {
    int s;
    s = send_fetch_body_cmd(connfd, msg_num);
    if (s == CONNECTION_ISSUE) return CONNECTION_ISSUE;

    int rspnlen = LARGE_BUF;
    char* response = malloc(LARGE_BUF*sizeof(char)); // maybe change size to read bytes
    assert(response);
    int rspn_bytes = 0;
    int nbytes_read = 0;
    char buffer[LARGE_BUF+1];
    int finished = 0;
    while (finished != 1 && (s=read(connfd, buffer, LARGE_BUF)) > 0) {
        buffer[s] = '\0'; // safety reasons
        int i=0;
        if (nbytes_read == 0) { // first read
            rspn_bytes = read_message_size(buffer, &i, s);
            if (rspn_bytes == MSG_NOT_FOUND) {
                free(response);
                return MSG_NOT_FOUND;
            }
            else if (rspn_bytes == GEN_FAILURE) {
                free(response);
                return GEN_FAILURE;
            }
        }
        for (; i<s; i++) {
            //fprintf(stderr, "%c", buffer[i]);
            response[nbytes_read] = buffer[i];
            nbytes_read += 1;
            if (nbytes_read >= rspn_bytes) {
                response[nbytes_read] = '\0';
                finished = 1;
                break;
            }
            if (nbytes_read >= rspnlen) {
                rspnlen *= GROWTHFCT;
                response = (char*) realloc(response, rspnlen*sizeof(char));
                assert(response);
            }
        }
        fprintf(stderr, "Read all bytes from this buffer, nbr=%d, rspb=%d rspl=%d\n", nbytes_read, rspn_bytes, rspnlen);
    }

    if (s==-1) {
        fprintf(stderr, "Failed to fully read response from fetch\n");
        return CONNECTION_ISSUE;
    }
    tag++;
    *rspn = response; 
    return SUCCESS;
}

int mime(int connfd, int msg_num) {
    char* response;
    int s=fetch(connfd, msg_num, &response);
    if (s < 0 ) return s;

    char* mime_v = stristr(response, "MIME-Version: 1.0\r\nContent-Type: multipart/alternative;");
    if (mime_v == NULL) {
        fprintf(stderr, "Not MIME\n");
        return IMAP_ERROR;
    }
    int i = (mime_v - response) + 56; // 56 is length of above match
    while (response[i] != '=') i++; // find start of boundary
    char* boundary = NULL;
    if (response[++i] != '"') boundary = get_unquoted_boundary(response, i);
    else boundary = get_quoted_boundary(response, ++i);

    fprintf(stderr, "boundary=%s\n", boundary);
    i += strlen(boundary);
    
    char* offset;
    while ((offset=stristr(response+i, boundary)) != NULL) {
        i += (offset - (response+i)) + strlen(boundary);
        //d_print_line(response+i);
        char* content_type = stristr(response+i, "Content-Type: ");
        char* content_encode = stristr(response+i, "Content-Transfer-Encoding: ");
        if (content_type == NULL) {
            if (content_encode == NULL) fprintf(stderr, "Couldn't find encode data, assuming 7bit\n");
            if (content_type == NULL) fprintf(stderr, "Couldn't find type data\n");
            return GEN_FAILURE;
        }
        

        int c_type = check_content_type(content_type+14);
        int c_encoding;
        if (content_encode != NULL) {
            c_encoding = check_content_transfer_encoding(content_encode+27); // adding length of literals
        } else c_encoding = 1; // assuming 7bit
        
        if (c_type && c_encoding) { 
            char* ending = stristr(response+i, boundary) - 4; // \r,\n and -- 
            int end = (ending - response);

            while(response[i-3] != '\r' || response[i-2] != '\n' || response[i-1] != '\r' || response[i] != '\n') i++;
            i++;

            for (int j=i; j<end; j++) {
                printf("%c", response[j]);
            }
            tag++;
            free(response);
            free(boundary);
            return SUCCESS;
        }
        fprintf(stderr, "\n");
    }

    tag++;
    free(boundary);
    free(response);
    return GEN_FAILURE;
}

void d_print_line(char* str) {
    int i=0;
    while (str[i] != '\0' && (str[i] != '\r' && str[i+1] != '\n')) {
        fprintf(stderr, "%c", str[i]);
        i++;
    }
    fprintf(stderr, "\n");
}

int check_content_type(char* field) {
    if (strncasecmp(field, "text/plain", 10) != 0) {
        fprintf(stderr, "Failing text/plain match\n");
        return FALSE;
    }
    int i=10; // start after first match
    while (field[i] != '=') {
        i++; // find '=' that is after charset
        if (i> 100) return GEN_FAILURE; // shouldnt happen
    }
    if (strncasecmp(field+(++i), "UTF-8", 5) != 0) {
        fprintf(stderr, "Failing UTF-8 match\n");
        return FALSE;
    }
    return TRUE;
}

int check_content_transfer_encoding(char* field) {
    if (strncasecmp(field, "quoted-printable", 16) == 0) {
        return TRUE;
    }
    if (strncasecmp(field, "7bit", 4) == 0) {
        return TRUE;
    }
    if (strncasecmp(field, "8bit", 4) == 0) {
        return TRUE;
    }
    fprintf(stderr, "Failing encoding match\n");
    return FALSE;
}

char* get_quoted_boundary(char* response, int start_idx) {
    int i = 0;
    int bound_len = 10;
    char* boundary = (char*) malloc(sizeof(char)*bound_len);
    assert(boundary);
    while (response[start_idx + i] != '"') { // go until the end of the quoted boundary
        boundary[i] = response[start_idx + i];
        i++;
        if (i >= bound_len) {
            bound_len *= GROWTHFCT;
            boundary = (char*) realloc(boundary, sizeof(char)*bound_len);
            assert(boundary);
        }
    }
    boundary[i] = '\0';
    return boundary;
}

char* get_unquoted_boundary(char* response, int start_idx) {
    int i = 0;
    int bound_len = 10;
    char* boundary = (char*) malloc(sizeof(char)*bound_len);
    assert(boundary);
    while (!isspace(response[start_idx + i])) { // go until we find a space character
        boundary[i] = response[start_idx + i];
        i++;
        if (i >= bound_len) {
            bound_len *= GROWTHFCT;
            boundary = (char*) realloc(boundary, sizeof(char)*bound_len);
            assert(boundary);
        }
    }
    boundary[i] = '\0';
    return boundary;
}

int parse(int connfd, int msg_num) {
    char *str;
    str = get_header_field(connfd, msg_num, "From");
    if (str == NULL) {
        return GEN_FAILURE;
    }
    printf("From: %s\n", str);
    free(str);
    
    str = get_header_field(connfd, msg_num, "To");
    if (str == NULL) {
        printf("To:\n");
    } else {
        printf("To: %s\n", str);
    }
    free(str);
    
    str = get_header_field(connfd, msg_num, "Date");
    if (str == NULL) {
        return GEN_FAILURE;
    }
    printf("Date: %s\n", str);
    free(str);

    int needfree = TRUE;
    str = get_header_field(connfd, msg_num, "Subject");
    if (str == NULL) {
        str = "<No subject>";
        needfree = FALSE;
    }
    printf("Subject: %s\n", str);
    if (needfree) free(str);
    
    return SUCCESS;
}

int list(int connfd, int num_msgs) {
    char* str;
    for (int i=1; i<num_msgs+1; i++) {
        int needfree = TRUE;
        str = get_header_field(connfd, i, "Subject");
        if (str == NULL) {
            str = "<No subject>";
            needfree = FALSE;
        }
        printf("%d: %s\n", i, str);
        if (needfree) free(str);
    }

    return SUCCESS;
}

char* get_header_field(int connfd, int msg_num, char* field) {
    int s;
    char cmd[BUFSIZE];
    if (msg_num != -1) snprintf(cmd, BUFSIZE, "%c FETCH %d BODY.PEEK[HEADER.FIELDS (%s)]\r\n", tag, msg_num, field);
    else snprintf(cmd, BUFSIZE, "%c FETCH * BODY.PEEK[HEADER.FIELDS (%s)]\r\n", tag, field);
    
    char field_matching[BUFSIZE];
    snprintf(field_matching, BUFSIZE, "%s:", field);

    s = write(connfd, cmd, strlen(cmd));
    if (s==-1) fprintf(stderr, "Failed to write to socket stream\n");

    char buffer[BUFSIZE+1];
    char header_field[BUFSIZE];
    int j = 0;
    buffer[BUFSIZE] = '\0';
    int finished = 0;
    while (finished != 1 && (s=read(connfd, buffer, BUFSIZE)) > 0) {
        buffer[s] = '\0';
        int i=3;
        char* header_ptr = stristr(buffer, field_matching);
        if (header_ptr == NULL) {
            return NULL;
        }
        i += strlen(field_matching);
        while (i < BUFSIZE) { // get offset from header_ptr to ensure?
            if (header_ptr[i-3] == '\r' && header_ptr[i-2] == '\n'&& header_ptr[i-1] == '\r' && header_ptr[i] == '\n') { // double carriage return
                finished = 1;
                break;
            }
            if (header_ptr[i-3] == '\r' && header_ptr[i-2] == '\n'&& header_ptr[i-1] != '\r') {
                i += 2;
            }
            header_field[j++] = header_ptr[i-3];
            i++;
        }
        header_field[j] = '\0';
    }
    tag++;
    return strdup(strip_whitespace(header_field));
}

char* strip_whitespace(char* str) {
    while (isspace(*str)) str++; // move string forward til non-space chaacter
        
    int i = strlen(str) - 1;
    while (isspace(str[i])) i--; // move back from end til non-space character

    str[i+1] = '\0';
    return str;
}

int read_list_num(char* ptr) {
    int list_num = 0;
    while (!isspace(ptr[0])) {
        ptr--;
    }
    ptr--;
    int magitude = 1;
    while (!isspace(ptr[0])) {
        list_num += (ptr[0]-'0') * magitude;
        magitude *= 10; 
        ptr--;
    }

    return list_num;
}

int check_tag_BAD_NO(char* msg) {
    char badstr[6];
    snprintf(badstr, 6, "%c BAD", tag);
    char* s1 = strstr(msg, badstr);

    char nostr[5];
    snprintf(nostr, 5, "%c NO", tag);
    char* s2 = strstr(msg, nostr);

    return (s1 != NULL) || (s2 != NULL);

}

int confirm_tag_OK(char* msg) {
    char okstr[5];
    snprintf(okstr, 5, "%c OK", tag);
    return strstr(msg, okstr) != NULL;
}

int confirm_response(char* msg) {
    int s;
    for (int i=1; msg[i] != '\0'; i++) {
        if (msg[i-1] == 'O' && msg[i] == 'K') {
            s = TRUE;
            break;
        }
        s = FALSE;
    }
    if (s == TRUE) return SUCCESS;
    return GEN_FAILURE;
}

int execute_command(int connfd, char* command, int msg_num, int num_msgs) {
    int s;
    if (strncmp(command, "retrieve", 9) == 0) {
       char* response = NULL;
       s = fetch(connfd, msg_num, &response);
       if (response != NULL) {
            fprintf(stdout, "%s", response);
            free(response);
       }
    }
    if (strncmp(command, "parse", 5) == 0) {
       s = parse(connfd, msg_num);
    }
    if (strncmp(command, "mime", 4) == 0) {
       s = mime(connfd, msg_num);
    }
    if (strncmp(command, "list", 4) == 0) {
       s = list(connfd, num_msgs);
    }
    return s;
}

// case insenstive version of strstr, by chux from
// https://stackoverflow.com/questions/27303062/strstr-function-like-that-ignores-upper-or-lower-case
char* stristr(const char* haystack, const char* needle) {
  do {
    const char* h = haystack;
    const char* n = needle;
    while (tolower((unsigned char) *h) == tolower((unsigned char ) *n) && *n) {
      h++;
      n++;
    }
    if (*n == 0) {
      return (char *) haystack;
    }
  } while (*haystack++);
  return 0;
}