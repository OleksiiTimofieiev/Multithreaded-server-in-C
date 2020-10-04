#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <limits.h>

#define SERVERPORT          8989
#define BUFSIZE             4096
#define SOCKET_ERROR        (-1)
#define SERVER_BACKLOG      1

typedef struct sockaddr_in  SA_IN;
typedef struct sockaddr     SA;

void                        handle_connection(int client_socket);
int                         check(int exp, const char *msg);


int main(int argc, char **argv)
{
    int server_socket, client_socket, addr_size;
    SA_IN server_addr, client_addr;

    check((server_socket = socket(AF_INET, SOCK_STREAM, 0)), "Failed to create socket");

    // initialize the address struct
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVERPORT);

    check(bind(server_socket, (SA *)&server_addr, sizeof(server_addr)), "Bind Failed!");
    check(listen(server_socket, SERVER_BACKLOG), "Listen Failed!");

    while (true)
    {
        printf("Waiting for the new connections ... \n");

        // wait and accept new connection eventually;
        addr_size = sizeof(SA_IN);

        check(client_socket = accept(server_socket, (SA *)&client_addr, (socklen_t *) &addr_size), "Accept failed !");

        printf("Connected !\n");

        handle_connection(client_socket);

        return 0;
    }
}

void    handle_connection(int client_socket)
{
    char    buffer[BUFSIZE];
    size_t  bytes_read;
    int     msg_size = 0;
    char    actual_path[PATH_MAX + 1];

    // read the msg
    while ((bytes_read = read(client_socket, buffer + msg_size, sizeof(buffer) - msg_size - 1)) > 0)
    {
        msg_size += bytes_read;
        if (msg_size > BUFSIZE - 1 || buffer[msg_size - 1] == '\n')
            break;
    }

    check(bytes_read, "recv error");
    buffer[msg_size - 1] = 0;

    printf("REQUEST: %s\n", buffer);
    fflush(stdout);

    // validity check
    if (realpath(buffer, actual_path) == NULL)
    {
        printf("ERROR(bad path): %s\n", buffer);
        close(client_socket);
        return ;
    }

    FILE *fp = fopen(actual_path, "r");
    if (fp == NULL) {
        printf("ERROR(open): %s\n", buffer);
        close(client_socket);
        return ;
    }

    while ((bytes_read = fread(buffer, 1, BUFSIZE, fp)) > 0) 
    {
        printf("sending %zu bytes\n", bytes_read);
        write(client_socket, buffer, bytes_read);
    }

    close(client_socket);
    fclose(fp);
    printf("Closing connection\n");
    
    
}

int     check(int exp, const char *msg)
{
    if (exp == SOCKET_ERROR) {
        perror(msg);
        exit(1);
    }
    return exp;
}