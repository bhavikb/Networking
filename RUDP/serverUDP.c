#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>


struct packet {
    int sequence;
    int ack;
    int isACK;
    int receive_window;
    char data[1400];
};

char * create_char_response(struct packet *pack)
{
	char *temp;
	char *delim = "|";
	sprintf(temp, "%d|%d|%d|%d|%s\0",pack->sequence, pack->ack, pack->isACK, pack->receive_window, pack->data);
	return temp;
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
    strcpy(pack->data, token);

	return pack;
}
struct packet *create_ack_response(struct packet *request_pack)
{
	struct packet *response_pack;
	response_pack = (struct packet*) calloc(1, sizeof(struct packet));
	
	response_pack->sequence = -1;
	response_pack->ack = request_pack->sequence + strlen(request_pack->data);
	response_pack->isACK = 1;
	response_pack->receive_window = 12;
	strcpy(response_pack->data,"");

	return response_pack;
}
int main(int argc, char *argv[])
{
	int sockfd, newsockfd, portno, n;
	struct sockaddr_in serv_addr,cli_addr;
	socklen_t clilen;
	char buffer[1500];
	char *char_response;
	char *filename;
	FILE * fp;
	struct packet *pack, *response_pack;
	int lastByteRecv = 0;	
	if(argc < 2)
	{
		printf("\n usage: %s <port no>",argv[0]);
		return 0;
	}
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
  error("ERROR opening socket");
	fprintf(stdout,"Socket created");

	bzero((char *) &serv_addr, sizeof(serv_addr));
	portno = atoi(argv[1]);

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
  	error("ERROR on binding");

	clilen = sizeof(cli_addr);
	printf("waiting for req...");

	while(1)
	{
		n = recvfrom(sockfd,buffer,1500,0,(struct sockaddr *)&cli_addr,&clilen);
		printf("\nRequest---->%s\n", buffer);
		pack = parse_packet(buffer);

		//do processing
		response_pack = create_ack_response(pack);
		if((lastByteRecv) != pack->sequence)
		{
			printf("Possible loss of packets..received seq: %d\n", pack->sequence);
			response_pack->ack = lastByteRecv;
		}
		else
		{
			lastByteRecv = pack->sequence + strlen(pack->data);
		}
		char_response = create_char_response(response_pack);
		printf("Response <--- %s\n\n",char_response);
	//	if(!(response_pack->ack == 11))
		sendto(sockfd,char_response,1500,0,(struct sockaddr *)&cli_addr,sizeof(cli_addr));
	}
//	printf("\nReceived req:\n%s",buffer);
//	filename = parseHTTP(buffer);
/*
	fp = fopen(filename, "r");
	if(fp == NULL)
	{	
		printf("Error opening file");
	}
	file_contents = (char *) calloc(256,  sizeof(char));
	//Sending the contents of the file
	while((fgets(file_contents, 256, fp)) != NULL)
	{ 
		sendto(sockfd,pack,sizeof(struct packet),0,(struct sockaddr *)&cli_addr,sizeof(cli_addr));
  	fprintf(stdout,"Received the following:\n");
  	fprintf(stdout, "%s",pack->data);
	}
	//Lastly, sending a blank message to client to indicate end of transmission
//	sendto(sockfd, "", 0, 0, (struct sockaddr *)&cli_addr,sizeof(cli_addr));
//	print_time();*/
}
