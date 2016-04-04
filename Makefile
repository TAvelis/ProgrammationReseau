all: sws_q2 sws_q3 sws_q4 sws_q5 sws_q6 sws_q7
sws_q2: sws_q2.o utility.o inet_socket.o
	gcc sws_q2.o utility.o inet_socket.o -o sws_q2
sws_q2.o: sws_q2.c utility.h inet_socket.h 
	gcc -c sws_q2.c -std=c99 -pedantic -Wall
sws_q3: sws_q3.o utility.o inet_socket.o
	gcc sws_q3.o utility.o inet_socket.o -o sws_q3
sws_q3.o: sws_q3.c utility.h inet_socket.h
	gcc -c sws_q3.c -std=c99 -pedantic -Wall
sws_q4: sws_q4.o utility.o inet_socket.o
	gcc sws_q4.o utility.o inet_socket.o -o sws_q4
sws_q4.o: sws_q4.c utility.h inet_socket.h
	gcc -c sws_q4.c -std=c99 -pedantic -Wall
sws_q5: sws_q5.o utility.o inet_socket.o
	gcc sws_q5.o utility.o inet_socket.o -o sws_q5
sws_q5.o: sws_q5.c utility.h inet_socket.h
	gcc -c sws_q5.c -std=c99 -pedantic -Wall
sws_q6: sws_q6.o utility.o inet_socket.o
	gcc sws_q6.o utility.o inet_socket.o -o sws_q6 -D_POSIX_C_SOURCE=200112L
sws_q6.o: sws_q6.c utility.h inet_socket.h
	gcc -c sws_q6.c -std=c99 -pedantic -Wall
sws_q7: sws_q7.o utility.o inet_socket.o
	gcc sws_q7.o utility.o inet_socket.o -o sws_q7 -D_POSIX_C_SOURCE=200112L
sws_q7.o: sws_q7.c utility.h inet_socket.h
	gcc -c sws_q7.c -std=c99 -pedantic -Wall
utility.o: utility.c utility.h
	gcc -c utility.c -std=c99 -pedantic -Wall
inet_socket.o: inet_socket.c inet_socket.h
	gcc -c inet_socket.c -std=c99 -pedantic -Wall
clean: 
	rm -f sws_q2 sws_q3 sws_q4 sws_q5 sws_q6 sws_q7 *.o
