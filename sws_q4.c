// COMPILE WITH:
// gcc ./sws_q3.c -o sws_q3 utility.o inet_socket.o -std=c99

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

#include "utility.h"
#include "inet_socket.h"

/* storage buffer size for HTTP resquest header
 * its huge size is chosen on purpose
 * to be able to store any possible header
 */ 
#define BUFFER_SIZE 2000

char* writeFile(const char *filePath){
	int file_fd = open(filePath, O_RDONLY);
	if(file_fd == -1){
		return "404";
	}
	struct stat fileStats;
	stat(filePath, &fileStats);
	char* buf = malloc(fileStats.st_size + 1);
       	read(file_fd, buf, fileStats.st_size);
        close(file_fd);

       	buf[fileStats.st_size] = 0;
	return buf;
}

int main() {
    int num_read;
    
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
			        // print the request header until reaching the ending signature
		            printf("%.*s\n\n", ending_index, buffer);
		            
		            
			    char* fileToGet = "one/idex.html";
			    char* newBuf = writeFile(fileToGet);
			    if(newBuf == "404"){
		                sprintf(buffer,"HTTP/1.1 404 Not Found\r\nDate: Mon, 27 Jul 2009 12:28:53 GMT\r\nServer: Apache/2.2.14 (Win32)\r\nLast-Modified: Wed, 22 Jul 2009 19:15:56 GMT\r\nContent-Length: 58\r\nContent-Type: text/html\r\nConnection: keep-alive\r\n\r\n");
		            	printf("%s\n", buffer);
		            	write(client_fd, buffer, strlen(buffer));
				char* superNewBuf = writeFile("404");
		            	printf("%s\n", superNewBuf);
				write(client_fd, superNewBuf, strlen(superNewBuf));
			    }else{
		                sprintf(buffer,"HTTP/1.1 200 OK\r\nDate: Mon, 27 Jul 2009 12:28:53 GMT\r\nServer: Apache/2.2.14 (Win32)\r\nLast-Modified: Wed, 22 Jul 2009 19:15:56 GMT\r\nContent-Length: 58\r\nContent-Type: text/html; charset=UTF-8\r\nConnection: keep-alive\r\n\r\n");
		            	printf("%s\n", buffer);
		            	write(client_fd, buffer, strlen(buffer));
			    	printf("%s\n", newBuf);
			    	write(client_fd, newBuf, strlen(newBuf));
	            }
	            }
			}
		}
	}
	
    // close file descriptors and 
	close(listen_fd);
	
	return 1;
}
