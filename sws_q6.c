// COMPILE WITH:
// gcc ./sws_q6.c -o sws_q6 utility.o inet_socket.o -std=c99 -D_POSIX_C_SOURCE=200112L

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/epoll.h>
#include <string.h>
#include <assert.h>
#include <dirent.h>
#include <netdb.h>

#include "utility.h"
#include "inet_socket.h"

#define BUFFER_SIZE 2000
#define EPOLL_SIZE 2000
#define MAX_EVENTS 2000

char* writeFile(const char *filePath){
    printf("received: %s\n", filePath);
    FILE* myFile = fopen(filePath, "r");
    fseek(myFile, 0L, SEEK_END);
    long file_size = ftell(myFile);
    fseek(myFile, 0L, SEEK_SET);

    char* buf = malloc(file_size + 1);
    fread(buf, file_size, 1, myFile);
    fclose(myFile);

    buf[file_size] = 0;

    return buf;
}

int main(int argc, char *argv[]) {
    int num_read = 0;
    struct epoll_event event;
    struct epoll_event *events;
    
    // check input parameters
    if (argc != 2) {
        printf("Usage: sws_q5 [server_directory]\n");
        return -1;
    } else {
        DIR* server_dir = opendir(argv[1]);
        if (server_dir)
        {
            printf("Server directory found!\n");
            closedir(server_dir);
        } else if (ENOENT == errno) {
            errExit("server directory not found");
        } else {
            errExit("opendir");
        }
    }
    
    // listen to the client on port 8080
	int listen_fd = inetListen("8080", 5, NULL);
	if (listen_fd == -1) {
		errExit("inetListen");
	}
	
	// Turn on non-blocking mode on stdin
    int flags = fcntl(STDIN_FILENO, F_GETFL);
    if (fcntl(STDIN_FILENO, F_SETFL, flags|O_NONBLOCK) == -1) {
        errExit("stdin non block");
    }
    
	// Turn on non-blocking mode on server FD (listen_fd)
	flags = fcntl(listen_fd, F_GETFL);
	if(fcntl(listen_fd, F_SETFL, flags|O_NONBLOCK) == -1) {
		errExit("listen_fd non block");
	}
	
	// Set max connections allowed
	if (listen(listen_fd, EPOLL_SIZE) == -1) {
	    errExit("listen_fd max connections");
	}
	
	// Initialize the epoll instance 
    int epfd = epoll_create1(0);
    if (epfd == -1) {
        errExit("epoll_create");
    }
    
    // add listen_fd to the interest list
    event.data.fd = listen_fd;
    event.events = EPOLLIN | EPOLLET; // event : data can be read
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &event) == -1) {
        errExit("listen_fd epoll_ctl");
    }
    
    // add STDIN to the interest list
    event.data.fd = STDIN_FILENO;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, STDIN_FILENO, &event) == -1) {
        errExit("stdin epoll_ctl");
    }
    
	// storage buffer for events
	events = calloc(MAX_EVENTS, sizeof(event));
	
	// server running until shutdown
	for (;;) {
		int nb_fd_ready = epoll_wait(epfd, events, MAX_EVENTS, -1);

        if (nb_fd_ready == -1) {
           	if (errno == EINTR) { // restart if interrupted by signal
               	continue;
           	} else {
               	errExit("epoll");
           	}
        }
	   // shutdown the server when administrator enters 'q' in the console
		char x = 0;
		if ((read(STDIN_FILENO, &x, 1) == 1) && (x == 'q')) {
			break;
		}
		int i = 0;
		int client_fd;
	    for (i = 0; i < nb_fd_ready; i++) {
	        int fd = events[i].data.fd;
		    if ((events[i].events & EPOLLERR) ||
              (events[i].events & EPOLLHUP) ||
              (!(events[i].events & EPOLLIN))) {
              // An error has occured or socket not ready
	            fprintf(stderr, "epoll error\n");
	            close (events[i].data.fd);
	            continue;
	        } else if (listen_fd == events[i].data.fd) {
	            while (1) {
	                struct sockaddr in_addr;
                    socklen_t in_len;
                    char hbuf[10], sbuf[10];
                    in_len = sizeof in_addr;
                    
		            // accept client connection on socket listen_fd
		            client_fd = accept(listen_fd, &in_addr, &in_len);
		            if (client_fd == -1) {
			            if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                            break;
                        } else {
                            perror ("accept");
                            break;
                        }
		            }
		            
                    if (getnameinfo(&in_addr, in_len, hbuf, sizeof hbuf, sbuf, sizeof sbuf, NI_NUMERICHOST | NI_NUMERICSERV) == 0) {
                        printf("Accepted connection on descriptor %d (host=%s, port=%s)\n", client_fd, hbuf, sbuf);
                    }
		            
		            // New connection detected
		            if (client_fd != -1) {
			            printf("Accepting a new connection...\n");
			            /* turn on non-blocking mode on client FD (client_fd)
			            * fail: close connection and exit
			            * success: confirm connection
			            */
			            flags = fcntl(client_fd, F_GETFL);
			            if (fcntl(client_fd, F_SETFL, flags|O_NONBLOCK) == -1) {
				            close(client_fd);
				            errExit("client_fd non block");
			            } else {
			                event.data.fd = client_fd;
			                event.events = EPOLLIN; // | EPOLLET;
			                if (epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &event) == -1) {
			                    close(client_fd);
			                    errExit("epoll_ctl");
		                    }
	                    }
                    }
		        }
		        continue;
	        } else {
	            int done = 0;
	            while (1) {
	                // storage buffer for HTTP request header
	                char buffer[BUFFER_SIZE];
	                /* read the full request from the client if listen_fd != -1
		            * num_read: number of char read
		            * buffer: request's content storage
		            */
		            if (listen_fd != -1) {
		               num_read = read(events[i].data.fd, buffer, BUFFER_SIZE);
		            }
		            if (num_read == -1) {
		                if (errno != EAGAIN) {
                          perror("read");
                          done = 1;
                        }
		            } else if (num_read == 0) {
		                done = 1;
		                break;
		            }
		            // seek the request ending's signature in the data received
			        char* request_ending = "\r\n\r\n";
			        char* ending_ptr = strstr(buffer, request_ending);
			        // get the index of the ending
			        int ending_index = ending_ptr - buffer;
			        // Server log : HTTP request Header
			        char request_header[BUFFER_SIZE];
			        sprintf(request_header, "%.*s\n", ending_index, buffer);
		            printf("%s\n", request_header);
		            
		            // Extract name of requested file
                    char* server_path = argv[1];
                    char requested_file[BUFFER_SIZE];
                    char file_path[BUFFER_SIZE];
                    
                    ending_index = strstr(request_header, " HTTP/1.1") - request_header - 5;
                    sprintf(requested_file, "%.*s", ending_index, request_header + 5);
                    printf("requested file: %s\n", requested_file);
                    strcpy(file_path, server_path);
                    //TODO check that server_path ends with a "/"
                    strcat(file_path, requested_file);
                    printf("file path: %s\n", file_path);
                    
                    char response[BUFFER_SIZE];
                    char* fileToSend = "";
                    DIR* requested_dir = opendir(file_path);
                    // if no file requested
                    if (strlen(file_path) == strlen(server_path)) {
                        fileToSend = writeFile("index.html");
                    } else if (requested_dir) {
                        //this is a directory, send its index.html
                        //TODO check that file_path ends with a "/"
                        strcat(file_path, "index.html");
                        printf("send directory: %s", file_path);
                        fileToSend = writeFile(file_path);
                    } else if( access( file_path, F_OK ) != -1 ){
                        // this is a file, send it back to the client 
                        fileToSend = writeFile(file_path);
                    } else {
                        // not a directory nor a file, send Error 404
                        fileToSend = writeFile("404.html");
                        // Prepare HTTP response
                        sprintf(response, "HTTP/1.1 404 Bad Request\r\nContent-Length: %d\r\nContent-Type: text/html; charset=UTF-8\r\nConnection: Closed\r\n\r\n%s", (int)strlen(fileToSend), fileToSend);
	                    // Server log : HTTP response
	                    printf("%s\n", response);
	                    // Send HTTP response
	                    write(fd, response, strlen(response));
	                    continue;
                    }
                    
                    closedir(requested_dir);
                    
                    // Prepare HTTP response
                    sprintf(response, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html; charset=UTF-8\r\nConnection: Closed\r\n\r\n%s", (int)strlen(fileToSend), fileToSend);
		            // Server log : HTTP response
		            printf("reponse:\n%s\n", response);
		            // Send HTTP response
		            write(fd, response, strlen(response));
		            break;
		        }
		        
		        //TODO : persistent connections, so no close, have to check if it works
		        if (done) {
		            printf("Closed connection on descriptor %d\n", events[i].data.fd);
		            close(events[i].data.fd);
		        }
	        }
	    }
	}
	
	free(events);
    // close file descriptors and 
	close(listen_fd);
	
	return 1;
}
