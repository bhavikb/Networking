#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>

void error(const char *msg)
{
    perror(msg);
    exit(1);
}
/*
 * Function parses the request HTTP
 */
char *parseHTTP(char * buffer)
{
  char *filename, *request, *protocol, *context;
 	filename = (char *)calloc(50, sizeof(char));
  request = strstr(buffer, "GET");
  if(request == NULL) return NULL;
 
  protocol = strtok_r(request, " ", &context);
  if(protocol == NULL) return NULL;
 
  filename = strtok_r(NULL, " ", &context);
  if(filename == NULL) return NULL;
  
  return ++filename;
}
void print_time()
{
	struct timeval tim;
	gettimeofday(&tim, NULL);
	fprintf(stdout, "Last Byte sent at: %ld", tim.tv_sec * 1000000 + tim.tv_usec);
}

int main(int argc, char *argv[])
{
	int sockfd, newsockfd, portno, n;
	struct sockaddr_in serv_addr,cli_addr;
	socklen_t clilen;
	char buffer[256];
	char *file_contents;
	char *filename;
	FILE * fp;
	
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
	n = recvfrom(sockfd,buffer,256,0,(struct sockaddr *)&cli_addr,&clilen);
	printf("\nReceived req:\n%s",buffer);
	filename = parseHTTP(buffer);

	fp = fopen(filename, "r");
	if(fp == NULL)
	{	
		printf("Error opening file");
	}
	file_contents = (char *) calloc(256,  sizeof(char));
	//Sending the contents of the file
	while((fgets(file_contents, 256, fp)) != NULL)
	{ 
		sendto(sockfd,file_contents,strlen(file_contents),0,(struct sockaddr *)&cli_addr,sizeof(cli_addr));
  	fprintf(stdout,"Received the following:\n");
  	fprintf(stdout, "%s",buffer);
	}
	//Lastly, sending a blank message to client to indicate end of transmission
	sendto(sockfd, "", 0, 0, (struct sockaddr *)&cli_addr,sizeof(cli_addr));
	print_time();
}
