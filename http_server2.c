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
handle_connection(int sock) 
{   
    char buf[BUFSIZE]; //wrote this here because if I wrote in main compiler couldn't see it here
    int len, res;
    char * ok_response_f  = "HTTP/1.0 200 OK\r\n"        \
        					"Content-type: text/plain\r\n"                  \
        					"Content-length: %d \r\n\r\n";
 
    char * notok_response = "HTTP/1.0 404 FILE NOT FOUND\r\n"   \
        					"Content-type: text/html\r\n\r\n"                       \
        					"<html><body bgColor=black text=white>\n"               \
        					"<h2>404 FILE NOT FOUND</h2>\n"
        					"</body></html>\n";
    int status = 1;
    /* first read loop -- get request and headers*/
    //what does this if statement do? mean?
    /*it checks if the response is at least a response. it places the response in buffer buf. The
      the caller specifies size of buf in by buf...how does that work. Why is size sizeof(buf) - 1)?*/
    if ((len = recv(sock, buf, sizeof(buf) - 1, 0)) <= 0)  {
            perror("tcp_server: bind error");
            exit(-1);
    }
    /* parse request to get file name */
    int spaces = 0;
    int reading_toggle = -1;
    int iterator = 0;
    int iterator2 = 0;
    char file_path[BUFSIZE] = {0};
    while(spaces < 2 && iterator < BUFSIZE){
        if(buf[iterator]==' '){
            reading_toggle*=-1;
            spaces++;
        }
        if(reading_toggle==1 && buf[iterator]!=' '){
            file_path[iterator2++]=buf[iterator];
        }
        iterator++;
    }
    //printf("'%s'\n",file_path);
    /* Assumption: this is a GET request and filename contains no spaces*/
    /* open and read the file */
	FILE *file = fopen(file_path, "r");
    size_t size = BUFSIZE-strlen(ok_response_f);
    char file_buf[size];
    
    if (file){
        //read the file and save somewhere so i can craft response
        fread(file_buf, size, 1, file);
        fclose(file);
    } else{
        status = 0;
    };
	/* send response */
    //printf("STATUS:%d\n",status);
    if(status){
        sprintf(buf, "%s%s", ok_response_f,file_buf);
        len = strlen(buf);
        if ((res = send(sock, buf, len, 0)) <= 0) {
            perror("tcp_server: send error");
            exit(-1);
        }
    }
    else{
        len = strlen(notok_response);
        if ((res = send(sock, notok_response, len, 0)) <= 0) {
            perror("tcp_server: send error");
            exit(-1);
        }
    }
    /* close socket and free pointers */
    close(sock);
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

    /* parse command line args */
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

    /* Zero out all masks to initialize */
    FD_ZERO(&read_mask);
    FD_ZERO(&write_mask);
    FD_ZERO(&error_mask);
    /* Set up mask for file descriptors we want to read from */
    FD_SET(sock, &read_mask);

    /* connection handling loop: wait to accept connection */

    while ((client_sock = accept(sock, NULL, NULL)) >= 0) {
    
        /* create read list */
        
        /* do a select() */
        tmp_rmask = read_mask;
        tmp_timeout = Timeout;

        num = select(FD_SETSIZE, &tmp_rmask, &write_mask, &error_mask, &tmp_timeout);
        if (num > 0) {
            if (FD_ISSET(sock, &tmp_rmask)) {
                ret = handle_connection(sock);
            }
        }
    }
    return ret;
}
