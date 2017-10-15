#define PORT 27993
#define MAX_BUFFER 256
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
#define CHUNK_SIZE 2

typedef struct file_packet {
	uint16_t chksum; 
	uint16_t len;
	uint32_t seqno;
	char data[500]; 
}file_packet_t;

uint8_t chksum8(char buff[], size_t len)
{
    unsigned int sum;       // nothing gained in using smaller types!
    for ( sum = 0 ; len != 0 ; len-- )
        sum += *(buff++);   // parenthesis not required!
    return (uint8_t)sum;
}

typedef struct file_ack_packet {
	int seqno;

}file_ack_packet_t;
typedef struct ack_packet {
	int magic_number; 
	int file_len;

}ack_packet_t;
static volatile int signal_flag = 1;
int sockfd;
char hostname[50], NUID[20], flag[50];
void intHandler(int dummy) {
    signal_flag = 0;
    close(sockfd);
    exit(1);
}



char* build_file(int clientfd, char* file){
	char buffer[MAX_BUFFER];
	int seqno = 0;
	while(recv(clientfd, buffer, MAX_BUFFER, 0) > 0){
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
	// ack_packet_t *s = (ack_packet_t *)buffer;
	ack_packet_t s;
	// int magic_number = ntohl(s->magic_number);
	// int file_len = ntohl(s->file_len);
	memcpy(&s, buffer, sizeof(ack_packet_t)); // "Deserialize"	
	int magic_number = s.magic_number;
	int file_len = s.file_len;
	printf("magic number : %d, len: %d ", magic_number, file_len);
	return file_len;
}

//sends acknowledgement asking for sequence number specified
// int send_acknowledgement(int clientfd, file_packet_t pac) {
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

int main(int argc, char* argv[])
{   
	int port = PORT;
	
	if (argc != 2) {
    	printf("please follow the format : %s localhost\n", argv[0]);
    	exit(1);
  	}
	memcpy(hostname, argv[1], strlen(argv[1]));
	hostname[strlen(argv[1])] = '\0';
	int clientfd;;
    signal(SIGINT, intHandler);
	struct sockaddr_in self;
	char buffer[MAX_BUFFER];
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
	char expression[100],NUID[20];

	char hostname[1024];
	hostname[1023] = '\0';
	gethostname(hostname, 1023);

	struct sockaddr_in client_addr;
	int addrlen=sizeof(client_addr);

	clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &addrlen);
	int data_size = recv(clientfd, buffer, MAX_BUFFER, 0);	
	printf("message received %s \n",buffer);
	int file_len = validate_hello_message(buffer);
	if(file_len){
		if(send_acknowledgement(clientfd, 0)){
			char* file = (char*) malloc(file_len);
			build_file(clientfd, file);
			printf("final file %s ", file);
		}
	}
	else{
			printf("Invalid hello message. Closing client connection\n");
			close(clientfd);
	}
	
	// while (1)
	// {

	// }

	// while (1)
	// {	
	// 	int no_operations = rand() % 100;
	// 	int i = 0;
	// 	for(i = 0;i< no_operations;i++){
	// 		int num1 = rand() % 1000;
	// 		int num2 = rand() % 1000;
	// 		int operation = rand() % 4;
	// 		char operator = ' ';
	// 		if (operation == 0) {
	// 		  operator = '+';
	// 		} else if (operation == 1) {
	// 		  operator = '-';
	// 		} else if (operation == 2) {
	// 		  operator = '/';
	// 		} else if (operation == 3) {
	// 		  operator = '*';
	// 		}
	// 		sprintf(expression, "cs5700fall2017 STATUS %d %c %d\n", num1, operator, num2);
	// 		send(clientfd, expression, strlen(expression), 0);
	// 		int solution_size = recv(clientfd, buffer, MAX_BUFFER, 0);
	// 		buffer[solution_size] = '\0';
	// 		printf("response_solution from client %s", buffer);
	// 		int answer = validate_solution_message(buffer);
	// 		int expected_answer = 0;
	// 		if (operation == 0) {
	// 		  expected_answer = num1 + num2;
	// 		} else if (operation == 1) {
	// 		  expected_answer = num1 - num2;
	// 		} else if (operation == 2) {
	// 		  expected_answer = num1 / num2;
	// 		} else if (operation == 3) {
	// 		  expected_answer = num1 * num2;
	// 		}

	// 		if(answer != expected_answer){
	// 			printf("wrong solution. Closing client connection\n");
	// 			close(clientfd);
	// 			continue;
	// 		}
	// 	}
	// 	sprintf(expression, "cs5700fall2017 %s BYE\n", flag);
	// 	printf("sending expression %s", expression);
	// 	send(clientfd, expression, strlen(expression), 0);
	// 	close(clientfd);
	// }
	close(sockfd);
	return 0;
}