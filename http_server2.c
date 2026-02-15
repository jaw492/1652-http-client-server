/*
 * CS 1652 Project 1 
 * (c) Jack Lange, 2020
 * (c) Amy Babay, 2022
 * (c) <Student names here>
 * 
 * Computer Science Department
 * University of Pittsburgh
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#define BUFSIZE 1024
#define FILENAMESIZE 100

static const struct timeval Timeout = {10, 0};

static int 
handle_connection(int client_sock) 
{   
    //initialize buffer
    char buf[BUFSIZE] = {0};
    int len, res;

    //response to send when file exists and can be sent
    char * ok_response_f  = "HTTP/1.0 200 OK\r\n"        \
        					"Content-type: text/plain\r\n"                  \
        					"Content-length: %d \r\n\r\n";
 
    //response to send when file does not exist or cannot be sent
    char * notok_response = "HTTP/1.0 404 FILE NOT FOUND\r\n"   \
        					"Content-type: text/html\r\n\r\n"                       \
        					"<html><body bgColor=black text=white>\n"               \
        					"<h2>404 FILE NOT FOUND</h2>\n"
        					"</body></html>\n";

    //receive from client socket and write it to buffer
    if ((len = recv(client_sock, buf, sizeof(buf) - 1, 0)) <= 0)  {
            perror("tcp_server: recv error");
            return(-1);
    }

    //buffer is null terminated
    buf[BUFSIZE] = '\0';

    printf("'%s'\n",buf);
    //parse file path from request
    char file_path[FILENAMESIZE] = {0};
    //find first space, start reading file path. find second space, stop reading file path.
    int spaces = 0, x = 0;
    for(int i = 0; spaces < 2 && i < BUFSIZE; i++){
        if(buf[i] == ' '){
            spaces++;
            continue;
        }
        if(spaces == 1)
            file_path[x++]=buf[i];
    }
    file_path[x] = '\0';
    printf("'%s'\n", file_path);

    //open file
	FILE *file = fopen(file_path, "r");
    size_t size = BUFSIZE-strlen(ok_response_f);
    char file_buf[size];
    
    if (file){
        //read file contents into file_buf
        fread(file_buf, size, 1, file);
        fclose(file);

        //craft response
        int content_length = strlen(file_buf);
        sprintf(buf, ok_response_f, content_length);
        strcat(buf, file_buf);

        //send response
        len = strlen(buf);
        if ((res = send(client_sock, buf, len, 0)) <= 0) {
            perror("tcp_server: send error");
            exit(-1);
        }
    } else{
        //send error response
        len = strlen(notok_response);
        if ((res = send(client_sock, notok_response, len, 0)) <= 0) {
            perror("tcp_server: send error");
            exit(-1);
        }
    }
    close(client_sock);
	return 0;
}

int main(int argc, char ** argv)
{
    int server_port = -1;
    int ret         =  0;
    int sock        = -1;
    int client_sock = -1;
    struct sockaddr_in saddr;
    fd_set read_mask, write_mask, error_mask;
    fd_set tmp_rmask;
    int num;
    struct timeval tmp_timeout;

    //get command arguments
    if (argc != 2) {
        fprintf(stderr, "usage: http_server2 port\n");
        exit(-1);
    }

    server_port = atoi(argv[1]);

    if (server_port < 1500) {
        fprintf(stderr, "Requested port(%d) must be above 1500\n", server_port);
        exit(-1);
    }
    
    /* initialize and make socket */
    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("tcp_server: socket error");
        exit(-1);
    }
    /* set server address */
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = INADDR_ANY;
    saddr.sin_port = htons(server_port);

    /* bind listening socket */
    if (bind(sock, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
        perror("tcp_server: bind error");
        exit(-1);
    }
    /* start listening */
    if (listen(sock, 32) < 0) {
        perror("tcp_server: listen error");
        exit(-1);
    }

    //initalize masks
    FD_ZERO(&read_mask);
    FD_ZERO(&write_mask);
    FD_ZERO(&error_mask);
    //set up mask for file descriptors to read from
    FD_SET(sock, &read_mask);

    while (1) {
        //reset masks
        tmp_rmask = read_mask;
        tmp_timeout = Timeout;

        //wait for file descriptors
        num = select(FD_SETSIZE, &tmp_rmask, &write_mask, &error_mask, &tmp_timeout);

        //error
        if (num < 0) break;
        //nothings happened
        if (num == 0) continue;

        //handle each file descriptor
        for (int i = 0; i <= FD_SETSIZE && num > 0; i++){
            if (FD_ISSET(i, &tmp_rmask)){
                num--;

                if(i == sock){
                    client_sock = accept(sock, NULL, NULL); // accept connection
                    if(client_sock < 0) continue;
                    FD_SET(client_sock, &read_mask); // set up mask with new connection
                } else{
                    handle_connection(i);
                    FD_CLR(i, &read_mask); // remove socket after it's been handled
                }
            }
        }
    }
    close(sock);
    return ret;
}
