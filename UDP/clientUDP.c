#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <strings.h>
#include <arpa/inet.h>

void create_http_request(char *filename, char buffer[256], char *host)
 {
   buffer = strcat(buffer, "GET /");
   buffer = strcat(buffer, filename);
   buffer = strcat(buffer, " HTTP/1.1\r\nHost: ");
   buffer = strcat(buffer, host);
   buffer = strcat(buffer, "\nConnection: close\n");
  
 }
int main(int argc, char **argv)
{
  int sockfd,n, total_bytes_recv;
  struct sockaddr_in serv_addr,cli_addr;
  char sendbuffer[256];
  char recvbuffer[256];

  if (argc != 2)
  {
 printf("usage: %s <IP address> <port> <file>\n", argv[0]);
     exit(1);
  }

  sockfd=socket(AF_INET,SOCK_DGRAM,0);
  bzero(&serv_addr,sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr=inet_addr(argv[1]);
  serv_addr.sin_port=htons(atoi(argv[2]));

	create_http_request(argv[3], sendbuffer, argv[1]);
	fprintf(stdout, "\nSending Request:\n%s", sendbuffer);
	sendto(sockfd,sendbuffer,strlen(sendbuffer),0, (struct sockaddr *)&serv_addr,sizeof(serv_addr));	
	total_bytes_recv = 0;
	while((n=recvfrom(sockfd,recvbuffer,256,0,NULL,NULL)) > 0)
	{
		total_bytes_recv += n;
		fprintf(stdout, "%s", recvbuffer);
	}
	fprintf(stdout, "Total number of Bytes recieved: %d", total_bytes_recv);
}
