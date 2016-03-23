// COMPILE WITH:
// gcc ./sws_q2.c -o sws_q2 utility.o inet_socket.o -std=c99

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/timerfd.h>
#include <errno.h>

#include "utility.h"
#include "inet_socket.h"

#define BUFFER_SIZE 1000

struct timespec {
   time_t tv_sec;                /* Secondes */
   long   tv_nsec;               /* Nanosecondes */
};

struct itimerspec {
   struct timespec it_interval;  /* Intervalle pour les
                                    minuteries périodiques */
   struct timespec it_value;     /* Expiration initiale */
};


int check_timeout(fd timer_fd){
	uint64_t exp;
	s = read(fd, &exp, sizeof(uint64_t));
}

int main() {
	struct itimerspec timeout_value;
    int num_read;
    int buf_size;
	int listen_fd = inetListen("8080", 5, NULL);
	if (listen_fd == -1)
		errExit("inetListen");

	//timeout value
	timeout_value.it_interval.tv_sec = 0;
	timeout_value.it_interval.tv_nsec = 0;
	timeout_value.it_value.tv_sec = 10;
	timeout_value.it_value.tv_nsec = 0;
	
	// Turn on non-blocking mode on stdin
    int flags = fcntl(STDIN_FILENO, F_GETFL);
    if (fcntl(STDIN_FILENO, F_SETFL, flags|O_NONBLOCK) == -1)
        errExit("stdin non block");
    
	// Turn on non-blocking mode on server FD (listen_fd)
	flags = fcntl(listen_fd, F_GETFL);
	if(fcntl(listen_fd, F_SETFL, flags|O_NONBLOCK) == -1)
		errExit("listen fd non block");
	
	char buffer[BUFFER_SIZE];
	
	for (;;) {
		char x = 0;
		if ((read(STDIN_FILENO, &x, 1) == 1) && (x == 'q'))
			break;
		
		int client_fd = accept(listen_fd, NULL, NULL);
		if (client_fd == -1 && errno != EWOULDBLOCK)
			errExit("accept");
		
		num_read = read(client_fd, &buffer, buf_size);
		
		// There is a new connection
		if (client_fd != -1) {
			printf("Accepting a new connection...\n");
			flags = fcntl(client_fd, F_GETFL);
			if (fcntl(client_fd, F_SETFL, flags|O_NONBLOCK) == -1) {
				close(client_fd);
			} else {
				printf("New client connected!\n");
				int timer_fd = timerfd_create(CLOCK_REALTIME, 0);
				if (timerfd_settime(timer_fd, , &timeout_value, NULL) ==-1)
					errExit("timerfd_settime");
				if (num_read == 0) {
				    printf("nothing read from client");
			    } else {
		            printf("%d char read from client\n%s", num_read, buffer);
	            }
			}
		}
	}
	
	close(listen_fd);
	return 1;
}
