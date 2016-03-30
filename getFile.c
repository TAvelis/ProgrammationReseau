char* writeFile(const char *filePath){
	
	FILE *myFile = fopen(filePath, "r");
	fseek(myFile, 0L, SEEK_END);
	long file_size = ftell(myFile);
	fseek(myfile, 0L, SEEK_SET);

	char *buf = malloc(file_size + 1);
	fread(buf, fsize, 1, myFile);
	fclose(myFile);

	buf[file_size] = 0;

	return buf;
}
