#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <resolv.h>
#include <arpa/inet.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "packet_headers.h"

static volatile int signal_flag = 1;
 int mode = 1;
int sockfd;
char hostname[50], NUID[20], flag[50], file_name[30];
void intHandler(int dummy) {
    signal_flag = 0;
    close(sockfd);
    exit(1);
}



char* build_file_go_back_n(int clientfd, char* file){
}
char* build_file_stop_and_wait(int clientfd, char* file){
	char buffer[MAX_BUFFER];
	int seqno = 0;
	while(recv(clientfd, buffer, MAX_BUFFER, 0) > 0){
		printf("before\n");
		print_message(buffer, sizeof(file_packet_t));
		printf("after\n");
		file_packet_t f;
		// int magic_number = ntohl(s->magic_number);
		// int file_len = ntohl(s->file_len);
		memcpy(&f, buffer, sizeof(file_packet_t)); // "Deserialize"		
		int chksum = f.chksum;
		if(seqno == f.seqno){
			strncat (file, f.data, f.len);			
			seqno++;
			int file_chunk_acknowledgement = send_acknowledgement(clientfd, seqno);
		}
		else{
			printf("Droppping packet got seq no %d before %s \n", seqno,f.data);	
		}
		
		uint8_t calculated = chksum8(f.data, f.len);
		printf("chksums calculated at receiver %d, sender: %d \n", calculated, chksum);
		if(calculated != chksum)
		{
			// printf("chksums calculated at receiver %d, sender: %d ", calculated, chksum);
			printf("chksums dont match  \n");
			return NULL;
		}	
	}
	return file;

    
}
// validates the first message received from client to see if it is a hwllo message
int validate_hello_message(char buffer[]){
	// initial_ack_packet_t *s = (initial_ack_packet_t *)buffer;
	initial_ack_packet_t s;
	// int magic_number = ntohl(s->magic_number);
	// int file_len = ntohl(s->file_len);
	
	memcpy(&s, buffer, sizeof(initial_ack_packet_t)); // "Deserialize"	
	int magic_number = s.magic_number;
	int file_len = s.file_len;
	memcpy(file_name, s.file_name, strlen(s.file_name));
	if(mode !=  s.mode){
		printf("mode of sender doesnt match receiver");
		return -1;
	}
	printf("magic number : %d, len: %d file_name: %sfileName", magic_number, file_len, file_name);
	return file_len;
}

//sends acknowledgement asking for sequence number specified
int send_acknowledgement(int clientfd, int seqno) {
	char ackData[8];
	file_ack_packet_t file_ack;
	file_ack.seqno = seqno;
	;
	if (send(clientfd, &file_ack, sizeof(file_ack_packet_t), 0)== -1){
		return 0;
	}
	return 1;
}

int start_listening(int port) {
	struct sockaddr_in self;
	// char buffer[MAX_BUFFER];
	srand(time(NULL));  
    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("Socket");
		exit(errno);
	}

	bzero(&self, sizeof(self));
	self.sin_family = AF_INET;
	self.sin_port = htons(port);
	self.sin_addr.s_addr = INADDR_ANY;

    if ( bind(sockfd, (struct sockaddr*)&self, sizeof(self)) != 0 )
	{
		perror("socket--bind");
		exit(errno);
	}

	if ( listen(sockfd, 20) != 0 )
	{
		perror("socket--listen");
		exit(errno);
	}
	struct sockaddr_in client_addr;
	int addrlen=sizeof(client_addr);

	int clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &addrlen);
	return clientfd;
}
int main(int argc, char* argv[])
{   
	int port = PORT;
	int index;
	int c;
	char buffer[MAX_BUFFER];
	printf("receiver\n");
	opterr = 0;


	while ((c = getopt (argc, argv, "m:p:h:")) != -1){
		switch (c){
		  case 'm':
		  	mode = atoi(optarg);
		    break;
		  case 'p':	  
		    port = atoi(optarg);
		    break;
		  case 'h':
			memcpy(hostname, optarg, strlen(optarg));
			hostname[strlen(optarg)] = '\0';
		    break;	        
		  case '?':
		      printf (stderr,
		               "Unknown option character `\\x%x'.\n",
		               optopt);
		    return 1;
		  default:
		    abort ();
		}		
	}




	for (index = optind; index < argc; index++)
		printf ("Non-option argument % index: %d\n", argv[index],index);

	printf("port %d\nmode %d\nhostname %s\n",port, mode, hostname);
	
	if(mode < 1){
		printf("mode should be greater than 1\n");
		exit(1);
	}
	srand(time(NULL));  
    signal(SIGINT, intHandler);
	int clientfd;
	clientfd = start_listening(port);
	int data_size = recv(clientfd, buffer, MAX_BUFFER, 0);	
	printf("message received %s \n",buffer);
	int file_len = validate_hello_message(buffer);

	if(file_len){
		if(send_acknowledgement(clientfd, 0)){
			char* file = (char*) malloc(file_len);
			if(mode == 1){
				build_file_stop_and_wait(clientfd, file);
			}
			else{
				build_file_go_back_n(clientfd, file);
			}
			printf("final file %s ", file);
			char* opFileName = strcat(strtok(file_name, "."), "_output.txt");
			printf("output file name %s ", opFileName);
			FILE* fp1 = fopen(opFileName,  "w");
			fprintf(fp1, "%s", file);
		}
	}
	else{
			printf("Invalid hello message. Closing client connection\n");
			close(clientfd);
	}
	

	close(sockfd);
	return 0;
}