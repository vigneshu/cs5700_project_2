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


char* build_file_go_back_n(int clientfd, char* file){
	char buffer[MAX_BUFFER];
	uint32_t seqno = 0;
	
	file_packet_t f;
	// while(recv(clientfd, buffer, MAX_BUFFER, 0) > 0){
	while(recvfrom(clientfd, &f, sizeof(file_packet_t), 0, (struct sockaddr *) &clientaddr, &clientlen) > 0){
		// memcpy(&f, buffer, sizeof(file_packet_t)); // "Deserialize"		

		if(seqno == f.seqno){
			// int chksum = f.chksum;
			// uint8_t calculated = chksum8(f.data, f.len);			
			// if(calculated != chksum)
			// {
			// 	// printf("chksums calculated at receiver %d, sender: %d ", calculated, chksum);
			// 	printf("go_back_n chksums dont match  \n");
			// }	
			// else{	
			// 	strncat (file, f.data, f.len);			
			// 	seqno++;
			// }
			strncat (file, f.data, f.len);			
				seqno++;


		}
		else{
			// printf("go_back_n Droppping packet got seq no %d expected %d end \n", f.seqno,seqno);	
		}
		int chksum = f.chksum;
		uint8_t calculated = chksum8(f.data, f.len);			
		if(calculated != chksum)
		{
			// printf("chksums calculated at receiver %d, sender: %d ", calculated, chksum);
			// printf("go_back_n chksums dont match  \n");
						return NULL;
		}	
	
		
		// printf("go_back_n chksums calculated at receiver %d, sender: %d \n", calculated, chksum);
	
		// printf("requesting seqno: %d, received: %d , data: %s",seqno,f.seqno, f.data);
		int file_chunk_acknowledgement = send_acknowledgement(clientfd, seqno);	
		if(seqno >= packets){
			// printf("download complete seqno%d: packets:%d\n", seqno,packets);
			break;
		}
	}
	return file;
}
char* build_file_stop_and_wait(int clientfd, char* file){
	char buffer[MAX_BUFFER];
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
			if(calculated != chksum)
			{
				// printf("chksums calculated at receiver %d, sender: %d ", calculated, chksum);
				// printf("go_back_n chksums dont match  \n");
			}			
			else{
				strncat (file, f.data, f.len);			
				seqno++;
				int file_chunk_acknowledgement = send_acknowledgement(clientfd, seqno);			
			}

		}
		else{
			// printf("stop_and_wait Droppping packet got seq no %d before %d \n", seqno,f.seqno);	
		}
		if(seqno >= packets){
			printf("download complete\n");
			break;
		}

	}
	return file;

    
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
		printf("mode of sender doesnt match receiver");
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
		      printf ("Unknown option character \n");
		    return 1;
		  default:
		    abort ();
		}		
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
			char* file = (char*) malloc(file_len);
			printf("Please wait. File downloading...\n");
			if(mode == 1){
				build_file_stop_and_wait(clientfd, file);
			}
			else{
				build_file_go_back_n(clientfd, file);
			}
			char folder[100] = "./output/";
			// char* opFileName = strcat("./output/", file_name);
			char* opFileName = strcat(folder, file_name);
			// char* opFileName = "./output/a.txt";
			// char* opFileName = strcat(strtok(file_name, "."), "_output.txt");
			// printf("output file name ");
			// printf("\noutput file name %s\n ", opFileName);
			FILE* fp1 = fopen(opFileName,  "wb");
			fprintf(fp1, "%s", file);
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