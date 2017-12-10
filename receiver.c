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
#include <pthread.h>
#include <math.h>
static volatile int signal_flag = 1;
 int mode = 1;
int sockfd;
 struct sockaddr_in clientaddr;
 int clientlen;
 uint32_t packets;
char hostname[50], NUID[20], flag[50], file_name[30];
void intHandler(int dummy) {
    signal_flag = 0;
    close(sockfd);
    exit(1);
}

//sends acknowledgement asking for sequence number specified
int send_acknowledgement(int clientfd, uint32_t seqno) {
	char ackData[8];
	file_ack_packet_t file_ack;
	file_ack.seqno = seqno;
	
if 	( sendto(clientfd, &file_ack, sizeof(file_ack_packet_t), 0, 
	       (struct sockaddr *) &clientaddr, clientlen) == -1) {
	// if (send(clientfd, &file_ack, sizeof(file_ack_packet_t), 0)== -1){
		perror("send acknowledgement failed\n");
		exit(0);
		return 0;
	}
	return 1;
}

void build_file_sliding_window(int clientfd, FILE* fp1, int window_size){
	char buffer[MAX_BUFFER];
	memset(&buffer, 0, MAX_BUFFER);
	uint32_t seqno = 0;
	// TO store chunks for receiver window
	char buffer_file[window_size][MAX_BUFFER];
	file_packet_t f;
	while(recvfrom(clientfd, &f, sizeof(file_packet_t), 0, (struct sockaddr *) &clientaddr, &clientlen) > 0){

		// check if packet rerceived is within window size
		if(f.seqno >= seqno && f.seqno <= seqno + window_size || seqno == f.seqno){

			int chksum = f.chksum;
			uint8_t calculated = chksum8(f.data, f.len);			
			if(0)
			{
				printf("chksums calculated at receiver %d, sender: %d ", calculated, chksum);
				// printf("go_back_n chksums dont match  \n");
				int file_chunk_acknowledgement = send_acknowledgement(clientfd, seqno);	
				continue;
			}	
			else{

				// printf();	
				//copy into  receiver window buffer
				strcpy(buffer_file[(f.seqno-seqno) % window_size], f.data);
				//copy from window buffer into the file
				if (seqno == f.seqno){
					fprintf(fp1, "%s", buffer_file[0]);		
					// fprintf(fp1, "%s", f.data);		
					seqno++;								
				}
			}			

		}
		else{
			// printf("go_back_n Droppping packet got seq no %d expected %d end \n", f.seqno,seqno);	
		}

		
		// printf("go_back_n chksums calculated at receiver %d, sender: %d \n", calculated, chksum);
	
		// printf("requesting seqno: %d, received: %d , data: %s\n",seqno,f.seqno, f.data);
		int file_chunk_acknowledgement = send_acknowledgement(clientfd, seqno);	
		if(seqno >= packets+1){
			// printf("download complete seqno%d: packets:%d\n", seqno,packets);
			break;
		}
	}
}


void build_file_go_back_n(int clientfd, FILE* fp1){
	char buffer[MAX_BUFFER];
	memset(&buffer, 0, MAX_BUFFER);
	uint32_t seqno = 0;
	
	file_packet_t f;
	// while(recv(clientfd, buffer, MAX_BUFFER, 0) > 0){
	while(recvfrom(clientfd, &f, sizeof(file_packet_t), 0, (struct sockaddr *) &clientaddr, &clientlen) > 0){
		// memcpy(&f, buffer, sizeof(file_packet_t)); // "Deserialize"		

		if(seqno == f.seqno){
			int chksum = f.chksum;
			uint8_t calculated = chksum8(f.data, f.len);			
			if(0)
			{
				printf("chksums calculated at receiver %d, sender: %d ", calculated, chksum);
				// printf("go_back_n chksums dont match  \n");
				int file_chunk_acknowledgement = send_acknowledgement(clientfd, seqno);	
				continue;
			}	
			else{
				// printf();	
				fprintf(fp1, "%s", f.data);		
				seqno++;			
			}			

		}
		else{
			// printf("go_back_n Droppping packet got seq no %d expected %d end \n", f.seqno,seqno);	
		}

		
		// printf("go_back_n chksums calculated at receiver %d, sender: %d \n", calculated, chksum);
	
		// printf("requesting seqno: %d, received: %d , data: %s\n",seqno,f.seqno, f.data);
		int file_chunk_acknowledgement = send_acknowledgement(clientfd, seqno);	
		if(seqno >= packets+1){
			// printf("download complete seqno%d: packets:%d\n", seqno,packets);
			break;
		}
	}
}

void build_file_stop_and_wait(int clientfd, FILE* fp1){
	char buffer[MAX_BUFFER];
	memset(&buffer, 0, MAX_BUFFER);
	uint32_t seqno = 0;
	file_packet_t f;
	while(recvfrom(clientfd, &f, sizeof(file_packet_t), 0, (struct sockaddr *) &clientaddr, &clientlen) > 0){
	// while(recv(clientfd, buffer, MAX_BUFFER, 0) > 0){
		// printf("stop_and_wait before\n");
		// print_message(buffer, sizeof(file_packet_t));
		// printf("stop_and_wait after\n");
		
		// int magic_number = ntohl(s->magic_number);
		// int file_len = ntohl(s->file_len);
		// memcpy(&f, buffer, sizeof(file_packet_t)); // "Deserialize"	
		if(seqno == f.seqno){
			int chksum = f.chksum;
			uint8_t calculated = chksum8(f.data, f.len);			
			if(0)
			{
				printf("chksums calculated at receiver %d, sender: %d ", calculated, chksum);
				// printf("go_back_n chksums dont match  \n");
				int file_chunk_acknowledgement = send_acknowledgement(clientfd, seqno);		
				continue;
			}			
			else{
				fprintf(fp1, "%s", f.data);		
				seqno++;
				int file_chunk_acknowledgement = send_acknowledgement(clientfd, seqno);			
			}

		}
		else{
			// printf("stop_and_wait Droppping packet got seq no %d before %d \n", seqno,f.seqno);	
		}
		if(seqno >= packets+1){
			printf("download complete\n");
			break;
		}

	} 
}
// validates the first message received from client to see if it is a hwllo message
uint32_t validate_hello_message(initial_ack_packet_t s){
	// initial_ack_packet_t s;
	
	// memcpy(&s, buffer, sizeof(initial_ack_packet_t)); // "Deserialize"	
	int magic_number = s.magic_number;
	uint32_t file_len = s.file_len;
		// printf("s.file_name %s",s.file_name);
	memcpy(file_name, s.file_name, strlen(s.file_name));
	if(mode !=  s.mode){
		printf("mode of sender doesnt match receiver\n");
		exit(0);
		return 0;
	}
	// printf("magic number : %d, len: %d file_name: %sfileName", magic_number, file_len, file_name);
	return file_len;
}


int create_socket(int port) {
	struct sockaddr_in serveraddr;
	// char buffer[MAX_BUFFER];
	srand(time(NULL));
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
    // if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("Socket");
		exit(errno);
	}

	bzero(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(port);
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);   

    if ( bind(sockfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) != 0 )
	{
		perror("socket--bind");
		exit(errno);
	}

	// if ( listen(sockfd, 20) != 0 )
	// {
	// 	perror("socket--listen");
	// 	exit(errno);
	// }

	return sockfd;

}
int start_listening(int sockfd) {
	struct sockaddr_in client_addr;
	int addrlen=sizeof(client_addr);
	// int clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &addrlen);
	// return clientfd;
	return sockfd;
}
int main(int argc, char* argv[])
{   
	int port = PORT;
	int index;
	int c;
	char buffer[MAX_BUFFER];
	// printf("receiver\n");
	opterr = 0;
	clientlen = sizeof(clientaddr);
	int receiver_window = 1;
	while ((c = getopt (argc, argv, "m:p:h:r:")) != -1){
		switch (c){
		  case 'm':
		  	mode = atoi(optarg);
		    break;
		  case 'p':	  
		    port = atoi(optarg);
		    break;
		  case 'r':	 
		  // receiver_window = 5; 
		    receiver_window = atoi(optarg);
		    break;	    
		  case 'h':
			memcpy(hostname, optarg, strlen(optarg));
			hostname[strlen(optarg)] = '\0';
		    break;
		  case '?':
		      printf ("Unknown option character \n");
		    return 1;
		  default:
		    abort ();
		}		
	}
 	if (receiver_window > 1 && receiver_window != mode){
 		printf("sender window and receiver window size must be same\n");
 		exit(0);
 	}



	// printf("port %d\nmode %d\nhostname %s\n",port, mode, hostname);
	
	if(mode < 1){
		printf("mode should be greater than 1\n");
		exit(1);
	}
	srand(time(NULL));  
    signal(SIGINT, intHandler);
	int clientfd;
	uint32_t file_len;
	sockfd = create_socket(port);
    while(1){
		clientfd = start_listening(sockfd);
		initial_ack_packet_t s;
		int data_size = recvfrom(sockfd, &s, sizeof(initial_ack_packet_t), 0, (struct sockaddr *) &clientaddr, &clientlen);


		// int data_size = recv(clientfd, buffer, MAX_BUFFER, 0);	
		 if (errno == EWOULDBLOCK) {
		 	errno = 0;
		 	continue;
		 }
		 if(data_size>-1){
		// printf("message received %s \n",buffer);
			file_len = validate_hello_message(s);
			packets =  ceil(file_len/CHUNK_SIZE);
		// printf("size of file: %d , packets: %d \n",file_len,packets);
			if(file_len != 0){
				break;
			}		
		}
	}


	if(file_len){
		if(send_acknowledgement(clientfd, 0)){
			printf("Please wait. File downloading...\n");
			
			// char folder[100] = "./output/";
			int pid = getpid();
			char* fname = strtok(file_name, ".");
			char* extension = strtok(NULL, ".");
			char buffer[100];
			snprintf(buffer, 100,"%s%d%s%s", fname, getpid(), ".", extension);
			// snprintf(buffer, 100,"%s%s%d%s%s",folder, fname, getpid(), ".", extension);
			FILE* fp1 = fopen(buffer,  "wb");
			if(mode == 1){
				build_file_stop_and_wait(clientfd, fp1);
			}
			else if (receiver_window == 1 && mode > 1){
				build_file_go_back_n(clientfd, fp1);
			}
			else if (receiver_window > 1 && mode > 1){
				printf("\n\n\nbuilding file sliding window \n\n\n");
				build_file_sliding_window(clientfd, fp1, receiver_window);
			}
			printf("\noutput file name %s\n ", buffer);
			
			// fprintf(fp1, "%s", file);
			fclose(fp1);
			printf("File download complete... The file should be available under the output folder\n");
		}
	}
	else{
			printf("Invalid hello message. Closing client connection\n");
			close(clientfd);
	}
	

	close(sockfd);
	return 0;
}