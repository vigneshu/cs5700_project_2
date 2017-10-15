
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include "packet_headers.h"


char fileName[50];
unsigned char * serialize_int(unsigned char *buffer, int value)
{
  /* Write big-endian int value into buffer; assumes 32-bit int and 8-bit char. */
  buffer[0] = value >> 24;
  buffer[1] = value >> 16;
  buffer[2] = value >> 8;
  buffer[3] = value;
  return buffer + 4;
}

unsigned char * serialize_char(unsigned char *buffer, char value)
{
  buffer[0] = value;
  return buffer + 1;
}

unsigned char * serialize_temp(unsigned char *buffer, struct initial_ack_packet *value)
{
  buffer = serialize_int(buffer, value->magic_number);
  buffer = serialize_int(buffer, value->file_len);
  return buffer;
}

// int send_temp(int fd, const struct initial_ack_packet *temp)
// {
//   unsigned char buffer[32], *ptr;
// //  data = (unsigned char*)malloc(sizeof(regn));
//   // ptr = serialize_temp(buffer, temp);
//   htonl(1000);

//   return send(fd, ptr - buffer, sizeof(struct initial_ack_packet), 0);
//   // return sendto(socket, buffer, ptr - buffer, 0, dest, dlen) == ptr - buffer;
// }

int send_file_chunk(int fd, char buffer[], int size, int seqno) {
	file_packet_t pac;
	// pac.data = buffer;
	strcpy(pac.data, buffer);
	pac.seqno = seqno;
	pac.len = size;
	pac.chksum = chksum8(buffer, size);

	if (send(fd, &pac, sizeof(file_packet_t), 0) == -1) {
	// if (send_temp(fd, initial_ack_packet) == -1) {
	  perror("file chunk send Error");
	  return 0;
	}
	return 1;	
		
}

int send_ack(int fd) {
	FILE * fp = fopen(fileName, "r");
	if (fp == NULL) {
			perror("Server- File NOT FOUND 404");
			return;
		}
	fseek(fp, 0L, SEEK_END);
	uint16_t size = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	fclose (fp);

	 initial_ack_packet_t ack;
	// ack.magic_number = htonl(12345);
	// ack.file_len = htonl(size);
	ack.magic_number = 2376;
	ack.file_len = size;	 
	char *data = (char*) malloc (sizeof(initial_ack_packet_t));
	memcpy((void*)data, (void*)&ack, sizeof(initial_ack_packet_t)); // "Serialize"

	printf("ack file_len %d ack\n", ack.file_len);
	printf("ack magic_number %d ack\n", ack.magic_number);
	printf("data %s end\n", data);
	printf("len(data) %d end\n", strlen(data));
	printf("size of file %d \n", size);
	printf("sizeof(initial_ack_packet_t) %x \n", sizeof(initial_ack_packet_t));
	printf("sizeof(ack) %x \n", sizeof(ack));
	// if (send(fd, &data, sizeof(data), 0) == -1) {
	// if(send(fd, "hello", strlen("hello"), 0)){
	if (send(fd, &ack, sizeof(initial_ack_packet_t), 0) == -1) {
	// if (send_temp(fd, initial_ack_packet) == -1) {
	  perror("ACK start communiocation  Error");
	 return 0 ;
	}
	return 1;
		
}


//connects to the serve, given hostname and port
int connect_server(char *host_name, int port) {
	struct sockaddr_in addr;
	int fd;
	fd = socket(AF_INET, SOCK_STREAM, 0);//TCP/IP network and TCP protocol.
    struct timeval timeout;      
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    if (setsockopt (fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
                sizeof(timeout)) < 0)
        error("setsockopt failed\n");
	if(fd == -1)
	{
	  printf("Error opening socket\n");
	  return -1;
	}
	struct hostent *host;
	if ((host = gethostbyname(host_name)) == NULL) {  // get the host info from host name
        printf("gethostbynamesss");
        return -1;
    }
    struct in_addr **addr_list = (struct in_addr **)host->h_addr_list;
    memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;   
	addr.sin_port =  htons(port);       
	addr.sin_addr.s_addr = 
        *(long*)addr_list[0]; 
    if (!connect(fd, (const struct sockaddr* )&addr, sizeof(addr)) == -1) {// connect to server
		perror("could not connect to host");
        return -1;
    }
    return fd;
}


int start_file_share_go_back_n(int fd) {
}
int start_file_share_stop_and_wait(int fd) {
	FILE* fp = fopen(fileName, "rb");
	if (fp == NULL) {
		perror("Sender- File NOT FOUND 404");
		return -1;
	}
	unsigned char file_chunk[CHUNK_SIZE];
	fseek(fp, 0L, SEEK_END);
	long size = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	int seqno = 0;
	size_t bytesRead =  0;
	do
	{
		fseek( fp, CHUNK_SIZE * seqno, SEEK_SET );
		bytesRead = fread(file_chunk, 1, CHUNK_SIZE, fp);
		printf("Read %d bytes from file, content is: %s\n  ",bytesRead, file_chunk);
		if(send_file_chunk(fd, file_chunk, bytesRead, seqno)){
			char buffer[MAX_BUFFER];
			//todo timeout
			int data_size = recv(fd, buffer, MAX_BUFFER, 0);
			 if (errno == EWOULDBLOCK) {
			 	printf("timeout sending again \n");
			 	continue;
			 }
			file_ack_packet_t s;
			memcpy(&s, buffer, sizeof(file_ack_packet_t)); // "Deserialize"	
			if (s.seqno == seqno + 1){//receiver received last packet
				printf("Receiver received packet %d and is requesting next one \n",seqno);
				seqno++;
			}
			else{
				printf("Sending again. Receiver requesting seq number %d \n",s.seqno);
				seqno = s.seqno;

				continue;
			}
		}
	} while (bytesRead > 0);

	fclose (fp);
	return size;	
}
// Sends hello message to sever, starting the communication
int start_communication(int fd) {
	int success = send_ack(fd);
	if(success){
		char buffer[MAX_BUFFER];
		int data_size = recv(fd, buffer, MAX_BUFFER, 0);
		file_ack_packet_t s;
		printf("sender received from receiver %s \n", buffer);
		memcpy(&s, buffer, sizeof(file_ack_packet_t)); // "Deserialize"	
		printf("seq number requested is  %d \n", s.seqno);
		
		if (s.seqno == 0){
			return 1;
		}
		return 0;
	}
	else{
		return 0;
	}
	return 1;
	
}


int main(int argc, char* argv[]) {

	int port = PORT, mode = 1, c;
	char hostname[50];


	while ((c = getopt (argc, argv, "m:p:h:f:")) != -1){
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
   		  case 'f':
			memcpy(fileName, optarg, strlen(optarg));
			fileName[strlen(fileName)] = '\0';
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
	printf("port %d\nmode %d\nhostname %sfileName %s\n",port, mode, hostname, fileName);
	if(mode < 1){
		printf("mode should be greater than 1\n");
		exit(1);
	}	

 	int fd = connect_server(hostname, port);
 	int success = start_communication(fd);
 	if(success){
 		if(mode == 1){
 			start_file_share_stop_and_wait(fd);
 		}
 		else{
			start_file_share_go_back_n(fd);
 		}
 	}
 	close(fd);
 	return 0;
}