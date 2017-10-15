#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdint.h>
#include <netinet/tcp.h>

void error(char *msg)
{
  perror(msg);
  exit(0);
}
void create_http_request(char *filename, char buffer[256], char *host, char *conn_type)
{
	buffer = strcat(buffer, "GET /");
	buffer = strcat(buffer, filename);
	buffer = strcat(buffer, " HTTP/1.1\nHost: ");
	buffer = strcat(buffer, host);
	buffer = strcat(buffer, "\nConnection: ");
	buffer = strcat(buffer, conn_type);
	buffer = strcat(buffer, "\n\n");
}

int main(int argc, char *argv[])
{
	int sockfd, portno, n,nfiles, val =1;
	int32_t bytes_to_read = 0;
  	struct sockaddr_in serv_addr;
  	struct hostent *server;
	char sendbuffer[256], recvbuffer[256];
	char conn_type[15];
	FILE *fp;
	char *file_contents;
	struct timeval start, end;
	if (argc < 5) {
		fprintf(stderr,"usage %s hostname port connection_type file_name\n", argv[0]);
      	exit(0);
    }
	portno = atoi(argv[2]);
  	sockfd = socket(AF_INET, SOCK_STREAM, 0);
  	if (sockfd < 0) 
  		error("ERROR opening socket");
	
	fprintf(stdout, "getting hostbyname");
	server = gethostbyname(argv[1]);
	if (server == NULL)
	{
  	fprintf(stderr,"ERROR, no such host");
  	exit(0);
	}
	
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr,
      (char *)&serv_addr.sin_addr.s_addr,
      server->h_length);
	serv_addr.sin_port = htons(portno);

	if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
		error("ERROR connecting");

	if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val)) < 0)
		printf("Error setting TCP_NODELAY");

  	bzero(sendbuffer,256);
	if(strcmp(argv[3],"np") == 0)
	{//non persistent connection
		strcpy(conn_type, "Close");
		create_http_request(argv[4], sendbuffer, argv[1], conn_type);
		fprintf(stdout, "SENDING REQUEST: \n%s\n", sendbuffer);
		gettimeofday(&start, NULL);
  		n = write(sockfd,sendbuffer,strlen(sendbuffer));
  		if (n < 0)
    		error("ERROR writing to socket");
  		bzero(recvbuffer,256);
	
		n = read(sockfd, (char *) &bytes_to_read,sizeof(bytes_to_read));
        bytes_to_read = ntohl(bytes_to_read);
        printf("\nBytes to read...%d",bytes_to_read);

		printf("\n\nwritten:%d,\nRESPONSE FROM SERVER:\n",n);
  		while((n = read(sockfd,recvbuffer,255)) > 0)
		{
			printf("\nbytesread:%d\n%s",n,recvbuffer);
			bytes_to_read -= n;
			if(bytes_to_read <= 0)
				break;
		}
		gettimeofday(&end, NULL);
		printf("\nStart time: %ld.%d",start.tv_sec, start.tv_usec);
		printf("\nEnd time: %ld.%d", end.tv_sec, end.tv_usec);
		printf("\nRTT: %ld\n", ((end.tv_sec * 1000000 + end.tv_usec)
		  - (start.tv_sec * 1000000 + start.tv_usec)));
  		if (n < 0)
    		error("ERROR reading from socket");
	}
	else if(strcmp(argv[3],"p") == 0)
	{
		strcpy(conn_type, "keep-alive");
		fp = fopen(argv[4], "r");
		if(fp == NULL)
		{
			fprintf(stderr, "Error reading file");
		}
		file_contents = (char*)calloc(256, sizeof(char));
		fgets(file_contents,256,fp);
		nfiles = atoi(file_contents);

		while(fgets(file_contents,256,fp))
		{
			if(nfiles > 1)
				strcpy(conn_type,"keep-alive");
			else
				strcpy(conn_type, "Close");
			nfiles--;
			//using strtok to remove trailing \n characters
			gettimeofday(&start,NULL);
			create_http_request(strtok(file_contents,"\n"), sendbuffer, argv[1], conn_type);
			fprintf(stdout, "SENDING REQUEST: \n%s\nlength:%lu", sendbuffer,strlen(sendbuffer));

			n = write(sockfd,sendbuffer,strlen(sendbuffer));
        	if (n < 0)
            	error("ERROR writing to socket");
        	bzero(sendbuffer,256);
			bzero(recvbuffer,256);
        	printf("\n\nwritten:%d,\nRESPONSE FROM SERVER:\n",n);
			
			n = read(sockfd, (char *) &bytes_to_read,sizeof(bytes_to_read));
			bytes_to_read = ntohl(bytes_to_read);
			printf("\nBytes to read...%d",bytes_to_read);
	
      while((n = read(sockfd,recvbuffer,255)) > 0)
			{
				bytes_to_read -= n;
				printf("%s",recvbuffer);
				if(bytes_to_read <= 0)
					break;
				bzero(recvbuffer,256);
			}
			gettimeofday(&end, NULL);
			printf("\nStart time: %ld.%d",start.tv_sec, start.tv_usec);
			printf("\nEnd time: %ld.%d", end.tv_sec, end.tv_usec);
			printf("\nRTT: %ld\n", ((end.tv_sec * 1000000 + end.tv_usec)
      - (start.tv_sec * 1000000 + start.tv_usec)));
      if (n < 0)
      	error("ERROR reading from socket");
			bzero(recvbuffer,256);
		}
	}
  return 0;
}

