#ifndef PACKETHEADERS_H
#define PACKETHEADERS_H

#define MAX_BUFFER 256
#define CHUNK_SIZE 100
#define PORT 15125
#define TIMEOUT_SECS 1
#define TIMEOUT_U_SECS 500
#define TIMEOUT_U_SECS_L 100
#include <pthread.h>
typedef struct file_ack_packet {
	uint32_t seqno;

}file_ack_packet_t;

typedef struct file_packet {
	uint16_t chksum; 
	uint32_t len;
	uint32_t seqno;
	char data[CHUNK_SIZE]; 
}file_packet_t;


typedef struct initial_ack_packet {
	 int magic_number; //senderId
	uint32_t file_len;
	char file_name[30];
	uint8_t mode;

}initial_ack_packet_t;

uint8_t chksum8(char buff[], size_t len)
{
    unsigned int sum;
    for ( sum = 0 ; len != 0 ; len-- )
        sum += *(buff++); 
    return (uint8_t)sum;
}
// }

typedef struct {
    pthread_mutex_t input_lock;
    pthread_cond_t input_cond;
    size_t n_bytes;
    int fd;
    int window_size;
    int sm;
    int n;
    int sb;
    int packet_to_send;
    int transfer_complete;
    long total_packets;
    int timeout;
} pthread_data_t;

void print_message(const char* buf, int buf_len)
{
    int i = 0, written = 0, j =0, last = 0;
    for (i=0; i<buf_len; i++) {
        unsigned char uc = buf[i];
        if (i%16 == 0)
            printf("0x%04x:   ", i);
        written += printf("%02x", uc);
        if ((i+1)%2 == 0)
            written += printf(" ");
        if ((i+1)%16 == 0 || i == buf_len-1) {
            if (i == buf_len-1)
                 last = i+1 - (i+1)%16;
            else
                last = i - 15;
            for (j=written; j<40; j++)
                printf(" ");
            written = 0;
            printf("\t");
            for (j=last; j<=i; j++) {
                if (buf[j] < 32 || buf[j] > 127)
                    printf(".");
                else
                    printf("%c", buf[j]);
            }
            printf("\n");
        }
    }
    printf("\n\n");
}
#endif