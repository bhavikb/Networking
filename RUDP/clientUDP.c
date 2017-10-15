#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <strings.h>
#include <arpa/inet.h>
#include <time.h>

#define ALPHA 0.125
#define BETA 0.25
struct packet {
	int sequence;
	int ack;
	int isACK;
	int receive_window;
	char data[1400];
};

struct rtt_queue_type {
	int sequence;
	struct timeval time;
};

struct rtt_queue_type rtt_queue[1000];
int rtt_queue_index = 0;

char * create_packet(int sequence, int ack, int isACK, int receive_window, char *data)
{
	char *buffer = (char *) calloc(1, 1024 + (sizeof(char) * strlen(data)));
	sprintf(buffer,"%d|%d|%d|%d|%s",sequence, ack,isACK, receive_window, data);

	return buffer;
}

struct packet *parse_packet(char *buffer)
{
    char *context, *token;
    struct packet *pack;

    pack = (struct packet *)calloc(1, sizeof(struct packet));

    token = strtok_r(buffer, "|", &context);
    pack->sequence = atoi(token);

    token = strtok_r(NULL, "|",&context);
    pack->ack = atoi(token);

    token = strtok_r(NULL, "|",&context);
    pack->isACK = atoi(token);

    token = strtok_r(NULL, "|",&context);
    pack->receive_window = atoi(token);

    token = strtok_r(NULL, "|",&context);
	if(token != NULL)
    strcpy(pack->data, token);

    return pack;
}

struct rtt_queue_type log_start_time(int next_sequence)
{
	rtt_queue[rtt_queue_index].sequence = next_sequence;
	gettimeofday(&rtt_queue[rtt_queue_index].time, NULL);
//	printf("seq:%d\n",rtt_queue[rtt_queue_index].sequence);
//	printf("time: %ld",rtt_queue[rtt_queue_index].time.tv_sec * 1000000L + rtt_queue[rtt_queue_index].time.tv_usec);
	rtt_queue_index++;
}

struct timeval get_start_time(int ack_sequence)
{
	int i;
	for( i = 0; i < rtt_queue_index; i++)
	{
		if(ack_sequence == rtt_queue[i].sequence)
			return rtt_queue[i].time;
	}
	return NULL;
}

long calculate_rtt(long estimatedRTT, int ack_sequence)
{
	struct timeval start, end;
	long devRTT =0, timeout, sampleRTT;
	start = get_start_time(ack_sequence);
	gettimeofday(&end, NULL);
	sampleRTT = (end.tv_sec - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec;
	printf("SampleRTT: %d", sampleRTT);
	estimatedRTT = (1 - ALPHA)*estimatedRTT + ALPHA * sampleRTT;
	printf("ERTT: %ld\n",estimatedRTT);
	devRTT = (1 - BETA) *devRTT + BETA * abs(sampleRTT - estimatedRTT);
	printf("DEVRTT: %ld\n", devRTT);
	timeout = estimatedRTT + 4 * devRTT;
	printf("TIMEOUT: %ld", timeout);
	return timeout;
}
long timeval_to_long(struct timeval time)
{
	return ((time.tv_sec * 1000000L) + time.tv_usec);
}
int main(int argc, char **argv)
{
  	int sockfd,n, total_bytes_recv;
  	struct sockaddr_in serv_addr,cli_addr;
  	char *sendpacket;
	struct packet *pack;
	char *recvbuffer;
	char * data_to_send;
	int window[10];
	int window_size = 3;
	int window_index = 0;
	int nextseq = 0;
	int lastAcknowledged = 0;
	int nAck_recv = 0;
	int lastByteSent = 0;
	int duplicateACKcount = 0;
	int npackets_sent = 0;
	struct timeval start_time, end_time;
	long estimatedRTT = 0, sampleRTT = 0;
  	if (argc != 3)
  	{
		printf("usage: %s <IP address> <port> <file>\n", argv[0]);
     	exit(1);
  	}

  	sockfd=socket(AF_INET,SOCK_DGRAM,0);
  	bzero(&serv_addr,sizeof(serv_addr));
  	serv_addr.sin_family = AF_INET;
  	serv_addr.sin_addr.s_addr=inet_addr(argv[1]);
  	serv_addr.sin_port=htons(atoi(argv[2]));

//	pack = (struct packet *) calloc(1, sizeof(struct packet));

	while(window_index < window_size)
	{
		sendpacket = create_packet(nextseq, -1, 0, 12, "bhavik shah");
		printf("\nSending %s ----->\n", sendpacket);
		sendto(sockfd, sendpacket,sizeof(char) *1500,0, (struct sockaddr *)&serv_addr,sizeof(serv_addr)); 
		nextseq += strlen("bhavik shah");
		window[window_index] = nextseq;
		window_index++;
		lastByteSent = nextseq - 1;
		npackets_sent++;
		log_start_time(nextseq);
	}

	window_index = 0;
	recvbuffer = (char *) calloc(1, sizeof(char) *1500);
    bzero(recvbuffer, sizeof(recvbuffer));
//	total_bytes_recv = 0;
	while((n=recvfrom(sockfd,recvbuffer,1500,0,NULL,NULL)) > 0)
	{
//		total_bytes_recv += n;
		fprintf(stdout, "\nReceived:%s\n", recvbuffer);
		pack = parse_packet(recvbuffer);
//		printf("\nsec: %ld usec:%ld",start_time.tv_sec, start_time.tv_usec);
		if(pack->isACK == 1)
		{
			nAck_recv++;
			if(nAck_recv == 1)
			{//get initial sampleRTT
				gettimeofday(&end_time, NULL);
				start_time = get_start_time(pack->ack);
				sampleRTT = timeval_to_long(end_time) - timeval_to_long(start_time);
				printf("sampleRTT = %ld\n", sampleRTT);
			}
			else if(nAck_recv == 2)
			{//2nd packet - initialize estimatedRTT to previous sampleRTT and calculate new sampleRTT
				estimatedRTT = sampleRTT;
				gettimeofday(&end_time, NULL);
                start_time = get_start_time(pack->ack);
                sampleRTT = timeval_to_long(end_time) - timeval_to_long(start_time);
				printf("sampleRTT = %ld, estimatedRTT = %ld \n", sampleRTT, estimatedRTT);
			}
			else
			{	//calculate_rtt(1, pack->sequence);
				gettimeofday(&end_time, NULL);
                start_time = get_start_time(pack->ack);
				printf("RTT = %ld", timeval_to_long(end_time) - timeval_to_long(start_time));
			}
			printf("Handling ack request\n");
			if(pack->ack == window[window_index])
			{
				window[window_index] = -1;
				printf("Received Ack for sequence number%d\n", pack->ack);
				lastAcknowledged = pack->ack;
				window_index++;
			}
			else
			{//out of order ack received //sending request again
				if(pack->ack > window[window_index])
				{
					printf("Ack > expected received the last request must have reached\n the req -- %s\n", recvbuffer);
					lastAcknowledged = pack->ack;
				}
				else if(pack->ack < window[window_index])
				{
					printf("Maybe duplicate ACK: %d\nIgnoring this event...\n", pack->ack);
					duplicateACKcount++;
					if(duplicateACKcount == 3)
					{
						printf("Fast retransmit 3 duplicate ack received\n");
						sendpacket = create_packet(pack->ack, -1, 0, 12, "bhavik shah");
        				printf("\nRe-Sending %s", sendpacket);
        				sendto(sockfd, sendpacket,sizeof(char) *1500,0, (struct sockaddr *)&serv_addr,sizeof(serv_addr));
						duplicateACKcount = 0;
					}
				}
			}
		}	
	}
//	recvbuffer = (char *) calloc(1, sizeof(char) *1500);
//	bzero(recvbuffer, sizeof(recvbuffer));
//	n=recvfrom(sockfd,recvbuffer,sizeof(char) *1500,0,NULL,NULL);
//	if(n == -1 || n == 0)
//		printf("error receiv");
//	else
//	fprintf(stdout, "\nReceived:%s", recvbuffer);
//	fprintf(stdout, "Total number of Bytes recieved: %d", total_bytes_recv);
}
