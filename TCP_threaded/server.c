#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/tcp.h>

#define NTHREADS 5
#define BUF_SIZE 256

struct HTTPHeader
{
	char *protocol, *filename, *conn_type, *host, *version;
};

void err(char *msg)
{
	perror(msg);
	return;
}
/*
 *  * Function parses the request and return a HTTP header structure with appropriate values
 *   */
struct HTTPHeader* parseHTTP(char *buffer)
{
	char *line1, *line2, *line3,  *context, *context2,*context3,*line, *token, *subtoken;
	char *delim1 = "\n", *delim2 = " ";
	struct HTTPHeader *header;
	int i=1;
	if(strcmp(buffer,"") == 0 || buffer == NULL)
		return NULL;

	header = (struct HTTPHeader *)malloc(sizeof(struct HTTPHeader));
	line1 = strtok_r(buffer, "\n",&context);
	line2 = strtok_r(NULL, "\n",&context);
	line3 = strtok_r(NULL, "\n",&context);
	
	header->protocol = strtok_r(line1, " ",&context2);
	header->filename = strtok_r(NULL, " ",&context2);
	header->filename++;
	header->version = strtok_r(NULL, "\n",&context2);
	
	strtok_r(line2, " ",&context2);
	header->host = strtok_r(NULL, "\n",&context2);
	
	strtok_r(line3, " ",&context2);
	header->conn_type = strtok_r(NULL, "\n",&context2);
	printf("\nConn: %s",header->conn_type);
	return header;
}
/*
 *  * Create Http 200 response
 *   */

char *create_http_response_200(char *file_contents)
{
	char *response200;
	
	response200 = (char *) calloc(2048, sizeof(char));

	response200 = strcat(response200, "HTTP/1.1 200 OK\nContent-type: text/html\n\n");

	return response200;
}

char *create_http_response_400()
{
	char *response400;

	response400 = (char *) calloc(1024, sizeof(char));

	response400 = strcat(response400, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\n");
	response400 = strcat(response400, "<html><body><h1>Bad Request</h1></body></html>");

	return response400;
}
/*
 *  * Create Http 404 response
 *   */
char * create_http_response_404()
{
	char *response404;

	response404 = (char *) calloc(1024, sizeof(char));
	
	response404 = strcat(response404, "HTTP/1.1 404 Not Found\nConnection: close\nContent-Type: text/html\n\n");
	response404 = strcat(response404, "<html><body>\n\t<h1>404 Not Found</h1>\n");
	response404 = strcat(response404, "\t The requested file does not exists\n</body></html>");

	return response404;
}

char * create_http_response(char *file_contents)
{
	if(file_contents == NULL)
	{// File does not exists
		return create_http_response_404();
	}
	return create_http_response_200(file_contents);
}
/*
 *  * Function opens the requested file and send the file contents as HTTP response
 *   */

int send_file_contents(int cli_fd, char *filename)
{
	FILE *fp;
	char *response;
	int total_char_read = 0, n;
	char filebuf[256] = {0};
	int bytes_to_send = 0;
	fp = fopen(filename, "r");
	if(fp == NULL)
	{//File not found hence send 404 response
		err("File does not exists\n Returning 404...");
		response = create_http_response(NULL);
		bytes_to_send = strlen(response);
		printf("\nTotal bytes to be sent..%d\n", bytes_to_send);
    	bytes_to_send = htonl(bytes_to_send);
    	n = write(cli_fd, (const char *)&bytes_to_send, sizeof(bytes_to_send));
		n = write(cli_fd, response, strlen(response));
		if (n < 0) printf("ERROR writing to socket");
		return 0;
	}
	//Calculate the number of bytes in the file
	fseek(fp, 0L, SEEK_END);
	bytes_to_send = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	response = create_http_response("");
	bytes_to_send += strlen(response);// add the length of response to the number of bytes

	printf("\nTotal bytes to be sent..%d\n", bytes_to_send);
	bytes_to_send = htonl(bytes_to_send);
	n = write(cli_fd, (const char *)&bytes_to_send, sizeof(bytes_to_send));

	n = write(cli_fd, response, strlen(response));
	while(fgets(filebuf, 255, fp))
	{
		n = write(cli_fd, filebuf, strlen(filebuf));
		if(n<0)
			printf("Error writing to socket");
		bzero(filebuf,256);
	}
	fclose(fp);
	return 1;
}

void *handle_client(void *clienfd)
{
	char buff[256];
	int n;
	char *response, *file_contents,*context; 
	struct HTTPHeader *header;
	int cli = *(int *)clienfd; 
	do
	{
		bzero(buff,256);
  		n = read( cli, buff, 255);
		buff[n] = '\0';
  		if (n < 0)
		{ 
			err("ERROR reading from socket");
			return;
		}
 
	  	fprintf(stdout,"\n\nREQUEST:\n%s", buff);

  		header = parseHTTP(buff);
		if(header == NULL)
		{//something went wrong while parsing request
			response = create_http_response_400();
			n = write(cli, response, strlen(response));
            if (n < 0) err("ERROR writing to socket");
            close(cli);
            return;
		}
  		if(strcmp(header->filename, "") == 0 || header->filename == NULL)
  		{// Not an expected request send 400 Bad Request
    		response = create_http_response_400();
    		n = write(cli, response, strlen(response));
    		if (n < 0) err("ERROR writing to socket");
			close(cli);
   			return; 
  		}
 		else if(!send_file_contents(cli, header->filename))
		{
			fprintf(stderr, "Error reading file");
		}
	}while(strcmp(header->conn_type,"Close") != 0);
	close(cli);
}
int main(int argc, char *argv[])
{
	int servfd, clienfd, portno, fd_buff[NTHREADS], nconn = NTHREADS, i, val = 1;
	socklen_t clilen;
	char buffer[BUF_SIZE];
	char *filename, *file_contents, *response;
	struct sockaddr_in serv_addr, cli_addr;
	pthread_t thread[NTHREADS];
	int th_ret[NTHREADS];
	if(argc < 2) {
		err("Please specify the port number");
		exit(0);
	}	
	servfd = socket(AF_INET, SOCK_STREAM, 0);
	if(servfd < 0)
	{
		printf("Error in creating sockets");
	}
	printf("servfd: %d", servfd);
	bzero((char *) &serv_addr, sizeof(serv_addr));

	portno = atoi(argv[1]);
		
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	fprintf(stdout,"Binding...");
	if (bind(servfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
  	err("ERROR on binding");

	listen(servfd, NTHREADS);

	fprintf(stdout,"Listening...");

	clilen = sizeof(cli_addr);
	
	while(1)
	{
		clienfd = accept(servfd, (struct sockaddr *) &cli_addr, &clilen);
		
		if (setsockopt(clienfd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val)) < 0)
            fprintf(stderr, "Error setting TCP_NODELAY option");	

		th_ret[nconn] = pthread_create( &thread[nconn], NULL, handle_client, (void *)&clienfd);
	}
	for( i = 1; i <= NTHREADS; i++)
	{
		pthread_join( thread[i], NULL);
	}
  	close(servfd);
	return 0;
}
