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


#define BUFSIZE 1024
#define FILENAMESIZE 100


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
            exit(-1);
    }

    //buffer is null terminated
    buf[BUFSIZE-1] = '\0';

    //printf("'%s'\n",buf);
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
    //printf("'%s'\n", file_path);

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

int 
main(int argc, char ** argv)
{
    int server_port = -1;
    int ret         =  0;
    int sock        = -1;
    int client_sock = -1;
    struct sockaddr_in saddr;

    /* parse command line args */
    if (argc != 2) {
        fprintf(stderr, "usage: http_server1 port\n");
        exit(-1);
    }

    server_port = atoi(argv[1]);

    if (server_port < 1500) {
        fprintf(stderr, "INVALID PORT NUMBER: %d; can't be < 1500\n", server_port);
        exit(-1);
    }

    /* initialize and make socket */
    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) <0) {
        //AF_INET is address family specifying IPv4 Internet protocol
        //SOCK_STREAM sockets have different types. This specifies communication semantics
            //this one in particular provides sequenced, reliable, two-way based byte streams --so TCP
        //IPPROTO_TCP inside include netinet, in.h 
            //sockets are open to access IP protocl directly. this one specifies using TCP. most of this information
            //comes from man7
        perror("tcp_server: socket error");
        exit(-1);
    }
    /* set server address */
    memset(&saddr, 0, sizeof(saddr)); //sets variable to zero
    saddr.sin_family = AF_INET; //means ipv4
    saddr.sin_addr.s_addr = INADDR_ANY; //ip address 
    saddr.sin_port = htons(server_port);  //port number
    // these are the 5-tuple

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

    //wait for connection
    while ((client_sock = accept(sock, NULL, NULL)) >=0) {
        handle_connection(client_sock); //handle connection
    }

    close(sock);
    return ret;
}
