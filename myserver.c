

#include "unp.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>


#define ARRAY_SIZE_MAX 1000
#define DELIM "|"

int fsize(FILE*);
int parse_client_filename_header(char *, char *);
int make_server_file_size_header(int, int, char *);
int parse_client_chunk_header(char *, int *, int *, char *);
int make_server_chunk_header(char *, int, int);

int main(int argc, char **argv)
{
	struct  sockaddr_in servaddr, pcliaddr;
	int     listenfd, file_size, i, chunk_number, chunk_size, offset;
	char header_from_client[ARRAY_SIZE_MAX], filename[100], server_header[ARRAY_SIZE_MAX], client_number[100], *msg_type,
		chunk_header[ARRAY_SIZE_MAX];
	socklen_t len;
	FILE *fp;
	bool fin;
	
	   if(argc != 2)
   		err_quit("usage: myserver <port number>\n");

	while(1) {
		//initialize socket
		listenfd = socket(AF_INET,SOCK_DGRAM,0);
		bzero(&servaddr, sizeof(servaddr));
		servaddr.sin_family = AF_INET;
		servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
		servaddr.sin_port = htons(atoi(argv[1]));
	 
		//bind socket
		Bind(listenfd, (SA *) &servaddr, sizeof(servaddr));
	
		bzero(&header_from_client, sizeof(header_from_client));
	
		len = sizeof(pcliaddr);
		recvfrom(listenfd, header_from_client, ARRAY_SIZE_MAX, 0, (struct sockaddr *)&pcliaddr, &len);
		sprintf(client_number, "%d", pcliaddr.sin_addr.s_addr);
	
		msg_type = (char *) malloc(100);
		strncpy(msg_type, header_from_client, 4);
		if(strcmp(msg_type, "FILE") == 0) {
			parse_client_filename_header(header_from_client, filename);
			fp = fopen(filename,"r");
			if(fp == NULL) {
				make_server_file_error_header(filename, 1, server_header, "file not found");
			} else {
				file_size = fsize(fp);
				fclose(fp);
			
				//create header with size of file
				make_server_file_size_header(file_size, 0, server_header);
			}
		
			//send size of file to client
			sendto(listenfd, server_header, ARRAY_SIZE_MAX, 0, (struct sockaddr *) &pcliaddr, len);
		}
		
		if(strcmp(msg_type, "CHUN") == 0) {
			parse_client_chunk_header(header_from_client, &chunk_number, &chunk_size, &filename[0]);

			//read file chunk
			char * data = (char*)malloc(chunk_size);
			i = 0;
			fp = fopen(filename,"r");
			if(fp == NULL) {
				make_server_chunk_error_header(chunk_header, 1, "bad file name");
				sendto(listenfd, chunk_header, ARRAY_SIZE_MAX, 0, (struct sockaddr *) &pcliaddr, len);
			} else {
				offset = chunk_number * chunk_size;
				fseek(fp, offset, SEEK_SET);
				fin = false;
				for(i = 0; i < chunk_size; i++) {
			
					data[i] = fgetc(fp);
					if( feof(fp) ) {
						fin = true;
						break ;
					}
				}
				data[i] = 0;
				fclose(fp);
	
				//Send file chunk to client
				
				make_server_chunk_header(chunk_header, 0, chunk_number);
				sendto(listenfd, chunk_header, ARRAY_SIZE_MAX, 0, (struct sockaddr *) &pcliaddr, len);
				sendto(listenfd, data, strlen(data), 0, (struct sockaddr *) &pcliaddr, len);
				free(data);
			}
		}
		Close(listenfd);
	}
	free(msg_type);
}

int parse_client_filename_header(char * client_header, char * filename) {

	//store name of file
	char junk[ARRAY_SIZE_MAX];
	strcpy(junk, strtok(client_header, DELIM));
	strcpy(filename, strtok(NULL, DELIM));
}

int make_server_file_error_header(char * file_name, int return_code, char *server_header, char * error_msg) {

	//fill header with type, return code and error message
	char temp[100];
	strcpy(server_header,"SIZE");
	strcat(server_header, DELIM);
	sprintf(temp, "%d", return_code);
	strcat(server_header, temp);
	strcat(server_header, DELIM);
	strcat(server_header, error_msg);
}

int fsize(FILE *fp){
    int p = ftell(fp);
    fseek(fp, 0L, SEEK_END);
    int s = ftell(fp);
    fseek(fp, p,SEEK_SET);
    return s;
}

int make_server_file_size_header(int file_size, int return_code, char *server_header) {

	//fill header with type, return code and size data
	char temp[100];
	strcpy(server_header,"SIZE");
	strcat(server_header, DELIM);
	sprintf(temp, "%d", return_code);
	strcat(server_header, temp);
	strcat(server_header, DELIM);
	sprintf(temp, "%d", file_size);
	strcat(server_header, temp);
}

int parse_client_chunk_header(char * header, int * chunk_number, int * chunk_size, char * file_name) {

	//extract chunk number, size and name of file from client's header
	char junk[ARRAY_SIZE_MAX];
	strcpy(junk, strtok(header, DELIM));
	strcpy(file_name, strtok(NULL, DELIM));
	strcpy(junk, strtok(NULL, DELIM));
	*chunk_number = atoi(junk);
	strcpy(junk, strtok(NULL, DELIM));
	strcpy(junk, strtok(NULL, DELIM));
	*chunk_size = atoi(junk);
}

int make_server_chunk_error_header(char *header, int return_code, char *error_msg) {

	//make header containing type, return code and error message
	char temp[100];
	strcpy(header,"CHUNKDATA");
	strcat(header, DELIM);
	sprintf(temp, "%d", return_code);
	strcat(header, temp);
	strcat(header, DELIM);
	strcat(header, error_msg);
}

int make_server_chunk_header(char *header, int return_code, int place) {

	//make header with chunk data
	char temp[100];
	strcpy(header,"CHUNKDATA");
	strcat(header, DELIM);
	sprintf(temp, "%d", return_code);
	strcat(header, temp);
	strcat(header, DELIM);
	sprintf(temp, "%d", place);
	strcat(header, temp);
	strcat(header, DELIM);
}

	