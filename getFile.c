//Method from Lab1
int readline(int fd, char *buf, int nbytes) {
    int numread = 0;
    int returnval;
    while (numread < nbytes - 1) {
        returnval = read(fd, buf + numread, 1);
        /* If the read() syscall was interrupted by a signal */
        if ((returnval == -1) && (errno == EINTR))
            continue;
        /* A value of returnval equal to 0 indicates end of file */
        if ( (returnval == 0) && (numread == 0) )
            return 0;
        if (returnval == 0)
            break;
        if (returnval == -1)
            return -1;
        numread++;
        if (buf[numread-1] == '\n') {
            /* make this buffer a null-terminated string */
            buf[numread] = '\0';
            return numread;
        }
    }
    /* set errno to "invalid argument" */
    errno = EINVAL;
    return -1;
}

int getFile(const char *filePath){
	int file_fd = open(filePath, O_RDONLY);
	return file_fd
}

void writeFile(int writeTo_fd, const char *filePath){
	
	myFile_fd = getFile(filePath);

	int BUFSIZE = 100;
        char buf[BUFSIZE];
        int numread = 0;

       	while((numread = readline(STDIN_FILENO, buf, BUFSIZE)) > 0){
		       write(writeTo_fd, buf, numread);
       	}
}
