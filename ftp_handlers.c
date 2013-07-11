/* User includes */
#include "ftp_handlers.h"
#include "ftp_messages.h"
#include "error.h"

/* Standard includes */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

/* Networking includes */

#include <sys/socket.h>
#include <netinet/in.h>

static char *current_user = NULL;
struct sockaddr_in data_sock
    = {.sin_family = AF_INET,
    .sin_addr = {.s_addr = INADDR_ANY}};
int data_sockfd;

char *read_args(int sockfd)
{
    uint8_t done = 0;
    uint32_t buffsize = 256;
    char *args_buffer = calloc(sizeof(char), buffsize);
    uint32_t index = 0;

    while(1)
    {
        uint8_t cr = 0;

        while(index < buffsize - 1)
        {
            int8_t bytes_read = recv(sockfd, &args_buffer[index], 1, 0);

            if(bytes_read < 1)
                error("ERROR reading from socket");
            else if(cr && args_buffer[index] == '\n')
            {
                args_buffer[index - 1] = 0;
                break;
            }
            else if(args_buffer[index] == '\r')
            {
                cr = 1;
                continue;
            }

            cr = 0;

            ++index;
        }

        if(index == buffsize)
        {
            buffsize *= 2;
            free(args_buffer);
            char *tmp_args_buffer = calloc(sizeof(char), buffsize);
            memcpy(tmp_args_buffer, args_buffer, buffsize / 2);
            args_buffer = tmp_args_buffer;
            continue;
        }

        break;
    }

    return args_buffer;
}

void size_handler(int sockfd)
{

}

void user_handler(int sockfd)
{
    char uname_buffer[256];
    bzero(uname_buffer, 256);
    uint8_t done = 0;
    int16_t index = -1;
    uint8_t cr = 0;

    while(++index < 255)
    {
        int8_t bytes_read = recv(sockfd, &uname_buffer[index], 1, 0);

        if(bytes_read < 1)
            error("ERROR reading from socket");
        else if(uname_buffer[index] == ' ')
            index = -1;
        else if(cr && uname_buffer[index] == '\n')
        {
            uname_buffer[index - 1] = 0;
            break;
        }
        else if(uname_buffer[index] == '\r')
        {
            cr = 1;
            continue;
        }

        cr = 0;
    }

    if(!strcmp(uname_buffer, "anonymous") || uname_buffer[0] == 0)
    {
        current_user = "anonymous";
        write(sockfd, MSG_LOGIN_SUCCESS, sizeof(MSG_LOGIN_SUCCESS));
    }
}

void port_handler(int);
void pasv_handler(int sockfd)
{
    char ret_message[sizeof(MSG_PASSIVE_SUCCESS) + 29];

    data_sock.sin_port = 0;

    if (bind(data_sockfd, (struct sockaddr *)&data_sock, sizeof(data_sock)) < 0) 
        error("ERROR on PASV binding");

    snprintf(ret_message, sizeof(MSG_PASSIVE_SUCCESS) + 29, MSG_PASSIVE_SUCCESS " (%d,%d,%d,%d,%d,%d)\r\n", 192, 168, 1, 1, 255 & (data_sock.sin_port >> 8), 255 & data_sock.sin_port);

    printf("%s\n", ret_message);

    write(sockfd, ret_message, strlen(ret_message));
}

void empty_handler(int sockfd)
{
    write(sockfd, MSG_UNSUPPORTED, strlen(MSG_UNSUPPORTED));
}