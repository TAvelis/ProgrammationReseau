// COMPILE WITH:
// gcc ./sws_q5.c -o sws_q5 utility.o inet_socket.o -std=c99

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <dirent.h>

#include "utility.h"
#include "inet_socket.h"

/* storage buffer size for HTTP resquest header
 * its huge size is chosen on purpose
 * to be able to store any possible header
 */ 
#define BUFFER_SIZE 2000

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
	
	// storage buffer for HTTP request header
	char buffer[BUFFER_SIZE];
	
	// server running until shutdown
	for (;;) {
	
	    // shutdown the server when administrator enters 'q' in the console
		char x = 0;
		if ((read(STDIN_FILENO, &x, 1) == 1) && (x == 'q')) {
			break;
		}
		
		// accept client connection on socket listen_fd
		int client_fd = accept(listen_fd, NULL, NULL);
		if (client_fd == -1 && errno != EWOULDBLOCK) {
			errExit("accept");
		}
		
		/* read the full request from the client if listen_fd != -1
		 * num_read: number of char read
		 * buffer: request's content storage
		 */
	    int num_read;
		if (listen_fd != -1) {
		    num_read = read(client_fd, &buffer, BUFFER_SIZE);
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
				printf("New client connected!\n");
				// try to print the request header in the console
				if (num_read == 0) {
				    printf("ERROR: no request read from the client");
			    } else {
			        // seek the request ending's signature in the data received
			        char* request_ending = "\r\n\r\n";
			        char* ending_ptr = strstr(buffer, request_ending);
			        // get the index of the ending
			        int ending_index = ending_ptr - buffer;
			        // Server log : HTTP request Header
			        sprintf(buffer, "%.*s\n", ending_index, buffer);
		            printf("%s\n", buffer);
                    
                    // Extract name of requested file
                    char* server_path = argv[1];
                    char requested_file[BUFFER_SIZE];
                    char file_path[BUFFER_SIZE];
                    
                    ending_index = strstr(buffer, " HTTP/1.1") - buffer - 5;
                    sprintf(requested_file, "%.*s", ending_index, buffer + 5);
                    printf("requested file: %s\n", requested_file);
                    strcpy(file_path, server_path);
                    //TODO check that server_path ends with a "/"
                    strcat(file_path, requested_file);
                    printf("file path: %s\n", file_path);
                    
                    /* if the file exists
                     *   then take its content
                     *   else take index.html
                     */
                    char* fileToSend = "";
                    DIR* requested_dir = opendir(file_path);
                    // if no file requested
                    if (strlen(file_path) == strlen(server_path)) {
                        fileToSend = writeFile("index.html");
                    } else if (requested_dir) {
                        //this is a directory, send its index.html
                        closedir(requested_dir);
                        //TODO check that file_path ends with a "/"
                        strcat(file_path, "index.html");
                        printf("send directory: %s", file_path);
                        fileToSend = writeFile(file_path);
                    } else {
                        if( access( file_path, F_OK ) != -1 ){
                            fileToSend = writeFile(file_path);
                        } else {
                            // not a directory, send Error 404
                            fileToSend = writeFile("404.html");
                            // Prepare HTTP response
                            sprintf(buffer, "HTTP/1.1 404 Bad Request\r\nContent-Length: %d\r\nContent-Type: text/html; charset=UTF-8\r\nConnection: Closed\r\n\r\n%s", (int)strlen(fileToSend), fileToSend);
		                    // Server log : HTTP response
		                    printf("%s\n", buffer);
		                    // Send HTTP response
		                    write(client_fd, buffer, strlen(buffer));
		                    continue;
                        }
                    }
                    
                    // Prepare HTTP response
                    sprintf(buffer, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html; charset=UTF-8\r\nConnection: Closed\r\n\r\n%s", (int)strlen(fileToSend), fileToSend);
		            // Server log : HTTP response
		            printf("%s\n", buffer);
		            // Send HTTP response
		            write(client_fd, buffer, strlen(buffer));
	            }
			}
		}
	}
	
    // close file descriptors and 
	close(listen_fd);
	
	return 1;
}
