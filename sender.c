
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
#include <pthread.h>
#include <math.h>
#include <pthread.h>
char fileName[50];
 struct sockaddr_in serveraddr;
 int serverlen, retry_count = 0;


int send_file_chunk(int fd, char buffer[], uint32_t size, uint32_t seqno) {
	file_packet_t pac;
	// pac.data = buffer;
	strcpy(pac.data, buffer);
	pac.seqno = seqno;
	pac.len = size;
	pac.chksum = chksum8(buffer, size);

	// if (send(fd, &pac, sizeof(file_packet_t), 0) == -1) {
	if ( sendto(fd, &pac, sizeof(file_packet_t), 0, 
	       (struct sockaddr *) &serveraddr, serverlen) == -1) {
	// if (send_temp(fd, initial_ack_packet) == -1) {
	  perror("file chunk send Error");
	  return 0;
	}
	return 1;	
		
}

int send_hello_msg(int fd, int mode) {
	FILE * fp = fopen(fileName, "rb");
	if (fp == NULL) {
			perror("Server- File NOT FOUND 404");
			return 0;
		}
	fseek(fp, 0L, SEEK_END);
	uint32_t size = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	fclose (fp);

	 initial_ack_packet_t ack;
	ack.magic_number = 2376;
	ack.file_len = size;	 

	strcpy(ack.file_name, fileName); 
	ack.mode = mode;	 
	char *data = (char*) malloc (sizeof(initial_ack_packet_t));
	memcpy((void*)data, (void*)&ack, sizeof(initial_ack_packet_t)); // "Serialize"

	// if (send(fd, &ack, sizeof(initial_ack_packet_t), 0) == -1) {
	if ( sendto(fd, &ack, sizeof(initial_ack_packet_t), 0, 
	       (struct sockaddr *) &serveraddr, serverlen) == -1) {
	 perror("ACK start communiocation  Error");
	 return 0 ;
	}
	return 1;
		
}


//connects to the serve, given hostname and port
int connect_server(char *host_name, int port) {
	int fd;
	
	// fd = socket(AF_INET, SOCK_STREAM, 0);//TCP/IP network and TCP protocol.
	fd = socket(AF_INET, SOCK_DGRAM, 0);//UDP
    struct timeval timeout;      
    // timeout.tv_sec = 0;
    timeout.tv_sec = TIMEOUT_SECS;
    timeout.tv_usec = TIMEOUT_U_SECS;

    if (setsockopt (fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
                sizeof(timeout)) < 0)
        perror("setsockopt failed\n");
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
    memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;   
	serveraddr.sin_port =  htons(port);    

	serveraddr.sin_addr.s_addr = 
        *(long*)addr_list[0]; 
    serverlen = sizeof(serveraddr);
  //   if (!connect(fd, (const struct sockaddr* )&serveraddr, sizeof(serveraddr)) == -1) {// connect to server
		// perror("could not connect to host");
  //       return -1;
  //   }
    return fd;
}


void * wait_ack_go_back_n(void *t_data) 
{

	pthread_data_t *thread_data = (pthread_data_t*) t_data;
	char buffer[MAX_BUFFER];
	int fd = thread_data->fd;
	while(thread_data->transfer_complete == 0){
		file_ack_packet_t s;
		int data_size = recvfrom(fd, &s, sizeof(file_ack_packet_t), 0,
		 (struct sockaddr *) &serveraddr, &serverlen);

		// int data_size = recv(fd, buffer, MAX_BUFFER, 0);
		 if (errno == EWOULDBLOCK) {
		 	thread_data->timeout = 1;
		 	errno = 0;
		 	continue;
		 }	
		if(data_size == -1){
			// printf("recv error %d", errno);
			// break;
		}		 
		// memcpy(&s, buffer, sizeof(file_ack_packet_t)); // "Deserialize"	
		if(s.seqno >= thread_data->total_packets){// when receiver requests for one more than actually available, terminate
			printf("Completed transfer of file\n");
			thread_data->transfer_complete = 1;
			break;
		}	
		if(s.seqno > thread_data->sb){
			// printf("moving window sm:%d, sb:%d",thread_data->sm,thread_data->sb);
			thread_data->sm = (thread_data->sm - thread_data->sb) + s.seqno;
			thread_data->sb = s.seqno;
			retry_count = 0;

		}
	}
}

// AN array called timeout_block is used to maintain which block got a timeout
void * wait_ack_sliding_window(void *t_data) 
{

	pthread_data_t *thread_data = (pthread_data_t*) t_data;
	char buffer[MAX_BUFFER];
	int fd = thread_data->fd;
	while(thread_data->transfer_complete == 0){
		file_ack_packet_t s;
		int data_size = recvfrom(fd, &s, sizeof(file_ack_packet_t), 0,
		 (struct sockaddr *) &serveraddr, &serverlen);

		// int data_size = recv(fd, buffer, MAX_BUFFER, 0);
		 if (errno == EWOULDBLOCK) {
		 	thread_data->timeout_block[thread_data->sb % thread_data->window_size] = 1;
		 	thread_data->timeout = 1;
		 	errno = 0;
		 	continue;
		 }	
		if(data_size == -1){
			// printf("recv error %d", errno);
			// break;
		}		 
		// memcpy(&s, buffer, sizeof(file_ack_packet_t)); // "Deserialize"	
		if(s.seqno >= thread_data->total_packets){// when receiver requests for one more than actually available, terminate
			printf("Completed transfer of file\n");
			thread_data->transfer_complete = 1;
			break;
		}	
		if(s.seqno > thread_data->sb){
			// printf("moving window sm:%d, sb:%d",thread_data->sm,thread_data->sb);
			thread_data->sm = (thread_data->sm - thread_data->sb) + s.seqno;
			thread_data->sb = s.seqno;
			retry_count = 0;

		}
	}
}

// The sending part for the sliding window is the similiar to go back n. 
// The acknowledgement receive is handled differently 
int start_file_share_sliding_window(int fd, int window_size) {
	FILE* fp = fopen(fileName, "rb");
	if (fp == NULL) {
		perror("Sender- File NOT FOUND 404");
		return -1;
	}
	unsigned char file_chunk[CHUNK_SIZE];
	fseek(fp, 0L, SEEK_END);
	uint32_t size = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	int i = 0;
	int n  = window_size, sb = 0, sm = n + 1, windows_to_read = n + 1;
	pthread_data_t *thread_data = (pthread_data_t*)malloc(sizeof(pthread_data_t));
	thread_data->n_bytes = 0;
	thread_data->n = window_size;
	thread_data->sb = 0;
	thread_data->total_packets =  ceil(size/CHUNK_SIZE);
	
	// printf("thread_data->total_packets: %d, size: %d",thread_data->total_packets,size);
	// printf("thread_data->total_packets:  size:");
	thread_data->sm = n;
	thread_data->packet_to_send = 0;
	thread_data->transfer_complete = 0;
	thread_data->timeout = 0;
	thread_data->fd = fd;
	size_t bytesRead =  0;
	pthread_t acknowledgement_receiver_thread;
	pthread_create(&acknowledgement_receiver_thread, NULL, wait_ack_sliding_window, (void *) thread_data);
	
	uint32_t seqno = 0;

	while(!thread_data->transfer_complete){
		int sb = thread_data->sb, sm = thread_data->sm;//, seqno = thread_data->packet_to_send;
		fseek( fp, CHUNK_SIZE * seqno, SEEK_SET );
		
		while(seqno < sm){
			bytesRead = fread(file_chunk, 1, CHUNK_SIZE, fp);
			file_chunk[bytesRead] = '\0';
			// printf("go_back_n Read %lu bytes from file, content is: %s, seqno : %d, i: %d, sm: %d\n  ",bytesRead, file_chunk,seqno, i , sm);
			
			if(send_file_chunk(fd, file_chunk, bytesRead, seqno)){
				seqno++;
			}			
		}
		//the following loop will either wait for window to move or for a timeout
		while(thread_data->sm == sm ){
			int i = 0;
			for (i = 0; i < thread_data->window_size; i++){
				if (thread_data->timeout_block[thread_data->sb % thread_data->window_size] == 1)
				{
					printf ("sending block %d again because of timeout \n", thread_data->sb);
					printf ("sending block again because of timeout \n");
					thread_data->timeout_block[thread_data->sb % thread_data->window_size] = 0;
					if(!send_file_chunk(fd, file_chunk, bytesRead, seqno)){
						printf("sending failed");
					}
				}
			} 
			if(thread_data->timeout){
				printf("timout go back sliding window\n");
				seqno = thread_data->sb;
				thread_data->timeout = 0;
				retry_count++;
				break;
			}
			if (thread_data->transfer_complete){
				break;
			}
			usleep(100);

		}
		if(retry_count > MAX_RETRY_COUNT){
				printf("receiver crashed. Stopping sender go back n\n");
				exit(1);
				break;
		}
		
	}

	fclose (fp);
	free(thread_data);
	pthread_join(acknowledgement_receiver_thread, NULL);
	return size;	
}

int start_file_share_go_back_n(int fd, int window_size) {
	FILE* fp = fopen(fileName, "rb");
	if (fp == NULL) {
		perror("Sender- File NOT FOUND 404");
		return -1;
	}
	unsigned char file_chunk[CHUNK_SIZE];
	fseek(fp, 0L, SEEK_END);
	uint32_t size = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	int i = 0;
	/*
    pthread_t sender_thread = NULL;
    pthread_t acknowledgement_receiver_thread = NULL;
    pthread_data_t *thread_data = NULL;
    thread_data->n_bytes = 0;
    thread_data = malloc(sizeof(pthread_data_t));
    pthread_mutex_init(&(thread_data->input_lock), NULL);
    pthread_cond_init(&(thread_data->input_cond), NULL);

    pthread_create(&sender_thread, NULL, send_chunk_go_back_n, (void *) thread_data);
    pthread_create(&acknowledgement_receiver_thread, NULL, wait_ack_go_back_n, (void *) thread_data);

    pthread_join(sender_thread, NULL);
    pthread_join(acknowledgement_receiver_thread, NULL);
	*/
	int n  = window_size, sb = 0, sm = n + 1, windows_to_read = n + 1;
	pthread_data_t *thread_data = (pthread_data_t*)malloc(sizeof(pthread_data_t));
	thread_data->n_bytes = 0;
	thread_data->n = window_size;
	thread_data->sb = 0;
	thread_data->total_packets =  ceil(size/CHUNK_SIZE);
	
	// printf("thread_data->total_packets: %d, size: %d",thread_data->total_packets,size);
	// printf("thread_data->total_packets:  size:");
	thread_data->sm = n;
	thread_data->packet_to_send = 0;
	thread_data->transfer_complete = 0;
	thread_data->timeout = 0;
	thread_data->fd = fd;
	size_t bytesRead =  0;
	pthread_t acknowledgement_receiver_thread;
	pthread_create(&acknowledgement_receiver_thread, NULL, wait_ack_go_back_n, (void *) thread_data);
	
	uint32_t seqno = 0;

	while(!thread_data->transfer_complete){
		int sb = thread_data->sb, sm = thread_data->sm;//, seqno = thread_data->packet_to_send;
		fseek( fp, CHUNK_SIZE * seqno, SEEK_SET );
		
		while(seqno < sm){
			bytesRead = fread(file_chunk, 1, CHUNK_SIZE, fp);
			file_chunk[bytesRead] = '\0';
			// printf("go_back_n Read %lu bytes from file, content is: %s, seqno : %d, i: %d, sm: %d\n  ",bytesRead, file_chunk,seqno, i , sm);
			
			if(send_file_chunk(fd, file_chunk, bytesRead, seqno)){
				seqno++;
			}			
		}
		//the following loop will either wait for window to move or for a timeout
		while(thread_data->sm == sm ){
			if(thread_data->timeout){
				printf("timout go back n\n");
				seqno = thread_data->sb;
				thread_data->timeout = 0;
				retry_count++;
				break;
			}
			if (thread_data->transfer_complete){
				break;
			}
			usleep(100);

		}
		if(retry_count > MAX_RETRY_COUNT){
				printf("receiver crashed. Stopping sender go back n\n");
				exit(1);
				break;
		}
		
	}

	fclose (fp);
	free(thread_data);
	pthread_join(acknowledgement_receiver_thread, NULL);
	return size;		
}
int start_file_share_stop_and_wait(int fd) {
	FILE* fp = fopen(fileName, "rb");
	if (fp == NULL) {
		perror("Sender- File NOT FOUND 404");
		return -1;
	}
	unsigned char file_chunk[CHUNK_SIZE];
	fseek(fp, 0L, SEEK_END);
	uint32_t size = ftell(fp);
	int total_packets = ceil(size/CHUNK_SIZE);
	fseek(fp, 0L, SEEK_SET);
	uint32_t seqno = 0;
	size_t bytesRead =  0;
	do
	{
		fseek( fp, CHUNK_SIZE * seqno, SEEK_SET );
		bytesRead = fread(file_chunk, 1, CHUNK_SIZE, fp);
		file_chunk[bytesRead] = '\0';
		if(send_file_chunk(fd, file_chunk, bytesRead, seqno)){
			char buffer[MAX_BUFFER];
			//todo timeout
			file_ack_packet_t s;
			int data_size = recvfrom(fd, &s, sizeof(file_ack_packet_t), 0,
		 			(struct sockaddr *) &serveraddr, &serverlen);		
			// int data_size = recv(fd, buffer, MAX_BUFFER, 0);
			 if (errno == EWOULDBLOCK) {
			 	errno = 0;
			 	retry_count++;
 				if(retry_count > MAX_RETRY_COUNT){
					printf("receiver crashed. Stopping sender stop and wait \n");
					exit(1);
					break;
				}
			 	continue;
			 }
			
			// memcpy(&s, buffer, sizeof(file_ack_packet_t)); // "Deserialize"	
			if (s.seqno == seqno + 1){//receiver received last packet
				// printf("Receiver received packet %d and is requesting next one , total packets : %d\n",seqno,total_packets);
				seqno++;
				retry_count = 0;
			}
			else{
				seqno = s.seqno;

				continue;
			}

			if(seqno >= total_packets){// when receiver requests for one more than actually available, terminate
				printf("Completed transfer of file stop and wait\n");
				// printf("total_packets: %d, s.seqno:%d\n",total_packets,s.seqno);
				break;
			}
		}
	} while (1);

	fclose (fp);
	return size;	
}
// Sends hello message to sever, starting the communication
int start_communication(int fd, int mode) {
	int success = send_hello_msg(fd, mode);
	if(success){
		char buffer[MAX_BUFFER];
		file_ack_packet_t s;
		int data_size = recvfrom(fd, &s, sizeof(file_ack_packet_t), 0,
		 			(struct sockaddr *) &serveraddr, &serverlen);		
		// int data_size = recv(fd, buffer, MAX_BUFFER, 0);
		
		// memcpy(&s, buffer, sizeof(file_ack_packet_t)); // "Deserialize"	
		
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
	// signal(SIGALRM, ALARMhandler);
	int receiver_window = 1;
	int sliding_window = 0;
	while ((c = getopt (argc, argv, "m:p:h:f:r:")) != -1){
		switch (c){
		  case 'm':
		  	mode = atoi(optarg);
		    break;	    
		  case 'p':	  
		    port = atoi(optarg);
		    break;
		  case 'r':	  
		    receiver_window = atoi(optarg);
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
		      printf ("Unknown option character \n");
		    return 1;
		  default:
		    abort ();
		}		
	}
	// printf("port %d\nmode %d\nhostname %sfileName %s\n",port, mode, hostname, fileName);
	if(mode < 1){
		printf("mode should be greater than 1\n");
		exit(1);
	}	
	serverlen = sizeof(serveraddr);
 	int fd = connect_server(hostname, port);
 	int success = start_communication(fd, mode);
 	if (receiver_window > 1 && receiver_window != mode){
 		printf("sender window and receiver window size must be same\n");
 		exit(0);
 	}
 	if(success){
 		printf("Please wait. Sending file...\n");
		if(mode == 1){
			start_file_share_stop_and_wait(fd);
		}
		else if (receiver_window == 1){
			start_file_share_go_back_n(fd, mode);
		}
		else if (receiver_window > 1){
			printf("\n\nsending file sliding window \n\n\n\n");
			start_file_share_sliding_window(fd, mode);
			
		}

 	}
 	close(fd);
 	return 0;
}