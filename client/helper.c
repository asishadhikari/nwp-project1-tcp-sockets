#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "helper.h"




void error(const char* msg){
	perror(msg);
	exit(EXIT_FAILURE);
}

void flush_buffer(char *buffer){
	for (int i = 0; i < BUFFER_SIZE; i++) buffer[i] = '\0';
}


void capString(int soc, char *buf){
	char *cmd = "CAP\n", nl = '\n', c;
	Writeline(soc, cmd, 4);
	Writeline(soc, buf, strlen(buf));
	Writeline(soc, &nl, sizeof(nl));
	flush_buffer(buf);
	int rc = Readline(soc, buf, BUFFER_SIZE);
	flush_buffer(buf);
	Readline(soc, buf, BUFFER_SIZE);
	printf("\tThe capitalised string is %s\n\n",buf);
	flush_buffer(buf);
}


void getFile(int soc, char *buffer){
	char *pad = "FILE\n", *temp_buf,  nl = '\n', *endptr;
	int n; 
	Writeline(soc, pad, 5);
	Writeline(soc, buffer, strlen(buffer));
	Writeline(soc, &nl, 1);

	//only first part of message held ####\n
	if ( (temp_buf = calloc(35, 1)) == NULL ){
		errno = ENOMEM;
		error("Unable to allocate memory");
	}
	//get the number of bytes to read in first message from server
	Readline(soc, temp_buf, MAX_FILE_NAME_LEN);
	char *num_bytes_c = calloc(4,1);
	for (int i = 0; i < 4 ; i++){
		num_bytes_c[3-i] = temp_buf[i];
	}

	uint32_t num_bytes;
	for ( int i = 0 ; i < 4 ; i++ ) {
 	 num_bytes = (num_bytes << 8) | num_bytes_c[i];
	}
	
	char *file_name = calloc(MAX_FILE_NAME_LEN,1);
	strcpy(file_name, buffer);	
	file_name[(strlen(file_name)) - 1 ] = '\0';
	flush_buffer(buffer);
	//read in data to buffer
	Readfile(soc, buffer, num_bytes);
	//file not found in server
	if (strcmp(buffer,"NOT FOUND") == 0){
		printf("\tFile not found in server directory!! \n\n");
		flush_buffer(buffer);
	}else{
		FILE *fp;
		if ( (fp = fopen(file_name, "wb")) == NULL ) 
			error("Unable to open file for writing\n");	
		free(file_name);
		fwrite(buffer,1, num_bytes,fp);
		
		//attempt to make sure all missing bytes are read
/*		flush_buffer(temp_buf);
		flush_buffer(buffer);
		while ( (Readline(soc, temp_buf, BUFFER_SIZE)) > 0){
			num_bytes = 0;
			for (int i = 0; i < 4 ; i++){
				num_bytes_c[3-i] = temp_buf[i];
			}
			for ( int i = 0 ; i < 4 ; i++ ) {
 	 			num_bytes = (num_bytes << 8) | num_bytes_c[i];
			}
			printf(" attempting to read %d bytes\n", num_bytes);
			Readfile(soc, buffer, num_bytes);
			fwrite(buffer,1,num_bytes,fp);
			flush_buffer(temp_buf);
			flush_buffer(buffer);

		}
*/		free(num_bytes_c);
		fclose(fp);


	}
	flush_buffer(buffer);
	free(temp_buf);

}





ssize_t Readfile(int sockd, void *vptr, size_t maxlen) {
    ssize_t n, rc;
    char    c, *buffer;

    buffer = vptr;

    //attempt to read maxlen num bytes
    for ( n = 1; n <= maxlen; n++ ) {
	
	//if successful read of one character
	if ( (rc = read(sockd, &c, 1)) == 1 ) {
	    //increment buffer pointer and store the read byte
	    *buffer++ = c;
	    //loop if newline character encountered
	    if ( c == EOF)
		break;
	}
	
	//if no character was read
	else if ( rc == 0 ) {
	    //if  in first iteration of reading 
	    if ( n == 1 )
		return 0;  //no data ever available in the socket
	    else
		break; //finished reading all data in socket
	}

	else {
	    if ( errno == EINTR ) //non fatal error 
			continue;
    	return -1; //all other errors 
		}
    }

    *buffer = 0; //load terminating null byte 
    return n;
}



ssize_t Readline(int sockd, void *vptr, size_t maxlen) {
    ssize_t n, rc;
    char    c, *buffer;

    buffer = vptr;

    //attempt to read maxlen num bytes
    for ( n = 1; n < maxlen; n++ ) {
	
	//if successful read of one character
	if ( (rc = read(sockd, &c, 1)) == 1 ) {
	    //increment buffer pointer and store the read byte
	    *buffer++ = c;
	    //loop if newline character encountered
	    if ( c == '\n' )
		break;
	}
	
	//if no character was read
	else if ( rc == 0 ) {
	    //if  in first iteration of reading 
	    if ( n == 1 )
		return 0;  //no data ever available in the socket
	    else
		break; //finished reading all data in socket
	}

	else {
	    if ( errno == EINTR ) //non fatal error 
			continue;
    	return -1; //all other errors 
		}
    }

    *buffer = 0; //load terminating null byte 
    return n;
}

ssize_t Writeline(int fd, void *vptr, size_t n) {
    size_t      nleft;
    ssize_t     nwritten;
    char *buffer;
    buffer = vptr;
    nleft  = n;
    while ( nleft > 0 ) {
	if ( (nwritten = write(fd, buffer, nleft)) <= 0 ) {
	    if ( errno == EINTR )
		nwritten = 0;
	    else
		return -1;
	}
	nleft  -= nwritten;
	buffer += nwritten;
    }

    return n;
}


