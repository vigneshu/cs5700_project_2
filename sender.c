
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

// void ALARMhandler(int sig)
// {
//   signal(SIGALRM, SIG_IGN);           ignore this signal       
//   printf("Hello\n");
//   signal(SIGALRM, ALARMhandler);     /* reinstall the handler    */
// }

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

int send_hello_msg(int fd, int mode) {
	FILE * fp = fopen(fileName, "r");
	if (fp == NULL) {
			perror("Server- File NOT FOUND 404");
			return 0;
		}
	fseek(fp, 0L, SEEK_END);
	uint16_t size = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	fclose (fp);

	 initial_ack_packet_t ack;
	ack.magic_number = 2376;
	ack.file_len = size;	 

	strcpy(ack.file_name, fileName); 
	ack.mode = mode;	 
	char *data = (char*) malloc (sizeof(initial_ack_packet_t));
	memcpy((void*)data, (void*)&ack, sizeof(initial_ack_packet_t)); // "Serialize"

	printf("ack mode %d ack\n", ack.mode);
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
    timeout.tv_sec = TIMEOUT_SECS;
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



// void * send_chunk_go_back_n(void *t_data)
// { 
//     pthread_data_t *d = (pthread_data_t *)t_data;
//     int fd = d->fd;
//     int window_size = d->window_size;
//     size_t local_byte_count = 0;
//     do {
//         pthread_mutex_lock(&d->input_lock);
//         local_byte_count = fread(d->in_buf, sizeof(unsigned char),
//                 BUF_LENGTH_VALUE, stdin);

//         d->n_bytes += local_byte_count;
//         if (d->n_bytes > 0) { 
//             fprintf(stdout, "PRODUCER ...signaling consumer...\n");
//             pthread_cond_signal(&d->input_cond);
//             fprintf(stdout, "PRODUCER ...consumer signaled...\n");
//         }
//         pthread_mutex_unlock(&d->input_lock);

//         // This is added to slow down the producer so that we can observe
//         // multiple consumptions.
//         sleep(1);
//     } while (local_byte_count >  0);



//     return NULL;
// }

// void * wait_ack_go_back_n(void *t_data) 
// {
//     pthread_data_t *d = (pthread_data_t *)t_data;
//     int fd = d->fd;
//     int window_size = d->window_size;    
//     while (1) {
//         pthread_mutex_lock(&d->input_lock);
//         while (d->n_bytes == 0) {
//             fprintf(stdout, "CONSUMER entering wait \n");
//             pthread_cond_wait(&d->input_cond, &d->input_lock);
//         }
//         fprintf(stdout, "CONSUMER ...consuming chunk...\n");
//         d->n_bytes = 0;
//         fprintf(stdout, "CONSUMER ...chunk consumed...\n");
//         pthread_mutex_unlock(&d->input_lock);
//         fflush(stdout);
//     }

// }

void * wait_ack_go_back_n(void *t_data) 
{

	pthread_data_t *thread_data = (pthread_data_t*) t_data;
	char buffer[MAX_BUFFER];
	int fd = thread_data->fd;
	while(thread_data->transfer_complete == 0){
		int data_size = recv(fd, buffer, MAX_BUFFER, 0);
		printf("buffer %s\n",buffer);
		 if (errno == EWOULDBLOCK) {
		 	printf("timeout wait_ack_go_back_n \n");
		 	thread_data->timeout = 1;
		 	continue;
		 }
		if(thread_data->fd == 0){
			return;
		} 
		// if(data_size == -1){
		// 	printf("recv error %d", errno);
		// 	break;
		// }		 
		file_ack_packet_t s;
		memcpy(&s, buffer, sizeof(file_ack_packet_t)); // "Deserialize"	
		printf("go_back_n seq number requested is  %d thread_data->total_packets %d : \n", s.seqno,thread_data->total_packets);	
		if(s.seqno >= thread_data->total_packets){
			printf("Completed\n");
			thread_data->transfer_complete = 1;
			break;
		}	

		if(s.seqno > thread_data->sb){
			thread_data->sm = (thread_data->sm - thread_data->sb) + s.seqno;
			thread_data->sb = s.seqno;
			printf("moving window sm:%d, sb:%d",thread_data->sm,thread_data->sb);
		}
	}
}


int start_file_share_go_back_n(int fd, int window_size) {
	FILE* fp = fopen(fileName, "rb");
	if (fp == NULL) {
		perror("Sender- File NOT FOUND 404");
		return -1;
	}
	unsigned char file_chunk[CHUNK_SIZE];
	fseek(fp, 0L, SEEK_END);
	long size = ftell(fp);
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
	printf("len of file: %d \n", size);
	thread_data->total_packets = ceil(size/CHUNK_SIZE);
	thread_data->sm = n;
	thread_data->packet_to_send = 0;
	thread_data->transfer_complete = 0;
	thread_data->timeout = 0;
	thread_data->fd = fd;
	size_t bytesRead =  0;
	pthread_t acknowledgement_receiver_thread = NULL;
	pthread_create(&acknowledgement_receiver_thread, NULL, wait_ack_go_back_n, (void *) thread_data);
	
	int seqno = 0;

	while(!thread_data->transfer_complete){
		int sb = thread_data->sb, sm = thread_data->sm;//, seqno = thread_data->packet_to_send;
		fseek( fp, CHUNK_SIZE * seqno, SEEK_SET );
		
		while(seqno < sm){
			bytesRead = fread(file_chunk, 1, CHUNK_SIZE, fp);
			// printf("go_back_n Read %d bytes from file, content is: %s, seqno : %d, i: %d, sm: %d\n  ",bytesRead, file_chunk,seqno, i , sm);
			if(send_file_chunk(fd, file_chunk, bytesRead, seqno)){
				seqno++;
			}			
		}
		//the following loop will either wait for window to move or for a timeout
		while(thread_data->sm == sm ){
			// printf("waiting for timeout or window move\n");
			if(thread_data->timeout){
				printf("timeout sending again \n");
				seqno = thread_data->sb;
				thread_data->timeout = 0;
				break;
			}
			if (thread_data->transfer_complete){
				break;
			}
			usleep(100);

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
	long size = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	int seqno = 0;
	size_t bytesRead =  0;
	do
	{
		fseek( fp, CHUNK_SIZE * seqno, SEEK_SET );
		bytesRead = fread(file_chunk, 1, CHUNK_SIZE, fp);
		printf("stop_and_wait Read %d bytes from file, content is: %s\n  ",bytesRead, file_chunk);
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
int start_communication(int fd, int mode) {
	int success = send_hello_msg(fd, mode);
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
	// signal(SIGALRM, ALARMhandler);

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
		               "Unknown option character ",
		               optopt);
		    return 1;
		  default:
		    abort ();
		}		
	}
	memcpy(fileName, "a.txt", strlen("a.txt"));
	printf("port %d\nmode %d\nhostname %sfileName %s\n",port, mode, hostname, fileName);
	if(mode < 1){
		printf("mode should be greater than 1\n");
		exit(1);
	}	

 	int fd = connect_server(hostname, port);
 	int success = start_communication(fd, mode);
 	if(success){
 		if(mode == 1){
 			start_file_share_stop_and_wait(fd);
 		}
 		else{
			start_file_share_go_back_n(fd, mode);
 		}
 	}
 	close(fd);
 	return 0;
}