/* Joshua Caddell
 * CS 372 Project 2
 *
 * Description: ftserver will handle requests from client scripts
 * 	to transfer this scripts directory listing and text files
 * 	found in said directory. 
 *
 * this code was adapted from examples found on:
 * http://www.linuxhowtos.org/C_C++/socket.htm 
 * http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <netdb.h>

void error(const char *msg)//function to make error handling simpler
{
    	fprintf(stderr, "%s\n", msg);
    	exit(1);
}



int makeDataSocket(int port, char host[], int size);
void recvall(int s, char text[], int size);
int strsplit(char *token[], char s[]);
void getDirectoryListing(char files[]);
void getFile(char filename[], char text[], int textsize, int socket);
int sendall(int s, char *buf, int *len);
void encrypt(char keyText[], char message[], char encrypted[]);

int main(int argc, char *argv[])
{
    	int len, sockfd, newsockfd, datasockfd, portno, dataportno;
    	socklen_t clilen;
    	struct sockaddr_in cli_addr;
    	int n;
	char *client, message[1000000], buffer[256], *cmd[3], hostname[1024];
	  
	
	//check command line usage
	if (argc < 2) 
		error("USAGE ERROR, no port provided");
     
	portno = atoi(argv[1]);
	
	if(portno < 1024 || portno > 65535 || portno == 30020 || portno == 30021)
		error("Invalid port number");

	sockfd = makeControlConnection(portno);
	
	while(1)//loop on accept to take incoming connections
	{

		printf("\nServer open on port %d\n", portno);
	
		//accept new connections
		clilen = sizeof(cli_addr);
     		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
     		if (newsockfd < 0) 
          		error("ERROR on accept");
			
		client = inet_ntoa(cli_addr.sin_addr);
		printf("\nConnection from %s\n", client);
		
		//wait to recevie a command from client
		recvall(newsockfd, buffer, sizeof(buffer));
	
		len = strsplit(cmd, buffer);
		dataportno = atoi(cmd[len - 1]);

		memset(message, 0, sizeof(message));

		if(strcmp(cmd[0], "-l") == 0) 
		{
			strcpy(buffer, "valid");//send ACK to client
			len = strlen(buffer);
			sendall(newsockfd, buffer, &len);
			
			//create data connection
			datasockfd = makeDataSocket(dataportno, hostname, sizeof(hostname));
			
			printf("\nSending directory listing to %s:%d\n", client, dataportno);
			
			//get directory list and send to client		
			getDirectoryListing(message);
			len = strlen(message);
			sendall(datasockfd, message, &len);
			
			close(datasockfd);
		}
		else if(strcmp(cmd[0], "-g") == 0)
		{	
			len = 5;
			sendall(newsockfd, "valid", &len);//send ACK to client
			
			//create data connection
			datasockfd = makeDataSocket(dataportno, hostname, sizeof(hostname));
		
			printf("\n'%s' requested by %s:%d\n", cmd[1], client, dataportno);
			
			//get file and send to client
			getFile(cmd[1], message, sizeof(message), datasockfd);
						
			close(datasockfd);
		}
		else //if invalid command, send message to client
		{
			strcpy(buffer, "invalid");
			len = strlen(buffer);
			sendall(newsockfd, buffer, &len);
		}

		close(newsockfd);
	}
	//the program shouldn't ever reach here
	close(sockfd);
     
	return 0; 
}



/*Source of this function is http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#sendman
 *sends data in buf through socket s and keeps track of what was sent to deal 
with partial sends.  */
int sendall(int s, char *buf, int *len)
{
	int total = 0;        // how many bytes we've sent
	int bytesleft = *len; // how many we have left to send
	int n;

	while(total < *len) {//loop until all of the data is sent
		n = send(s, buf+total, bytesleft, 0);
		if (n == -1) { break; }
		total += n;
		bytesleft -= n;
	}

	*len = total; // return number actually sent here

	return n==-1?-1:0; // return -1 on failure, 0 on success
}

/*
 * Function to recieve data from a socket.
 * Paraments: Socket, pointer to char, size of parameter 2
 * Returns nothing but char array will contain data from 
 * 	the socket
 */
void recvall(int s, char text[], int size)
{
	int n;
	memset(text, 0, size);
	n = read(s, text, size-1);
	if(n < 0)
		error("Error receiving message");

}

/*
 * Function to split elements of a string into 
 * 	an array of strings.
 * Parameters: array of strings, string to be split
 * Returns: number of elements and array of strings 
 * 	is filled
 */
int strsplit(char *token[], char s[])
{	
	//get first element
	token[0] = strtok(s, " ");

	//-l commands do not contain a filename
	if(strcmp(token[0], "-l") == 0)
	{
		token[1] = strtok(NULL, " ");
		return 2;
	}		
	else
	{
		token[1] = strtok(NULL, " ");
		token[2] = strtok(NULL, " ");
		return 3;
	}
}

/* 
 * Function to get the directory listing 
 * Parament: pointer to char array
 * Returns nothing but char array will contain
 * 	directory listing 
 *
** Portions of this function were adapted from
** 	https://www.lemoda.net/c/recursive-directory/
*/
void getDirectoryListing(char files[])
{
	DIR *dp; 			//directory stream pointer
	struct dirent *ep;	//directory entry pointer
	char file[32];
	dp = opendir(".");
	ep = readdir(dp);
	
	while(ep)//loop until there are no more entries
	{
		strcpy(file, ep->d_name);

		if((strcmp(file, ".") != 0) && (strcmp(file, "..") != 0))
		{
			//append new element and a space
			strcat(files, ep->d_name);
			strcat(files, " ");
		}
		
		ep = readdir(dp); //get next entry
	}
	
	closedir(dp);
}

/*
 * Function to get the file requested by the client
 * Parameters: name of file, pointer to char to hold file contentes,
 * 	size of param 2, and socket file descriptor
 * Returns: nothing
 */
void getFile(char filename[], char text[], int textsize, int socket)
{
	FILE *fp;
	char line[2056];
	int bytesleft = textsize;
	int len;
	
	fp = fopen(filename, "r");
	
	if(!fp)
	{
		strcpy(text, "file not found");
		printf("\n%s\n", text);
	}
	else
	{
		printf("\nSending '%s'\n", filename);
		
		//iterates until no more lines in file
		while(fgets(line, 2056, fp) != NULL)
		{
			//check if there is space
			//in text[]
			if(bytesleft < sizeof(line))
			{
				//send data
				len = strlen(text);
				sendall(socket, text, &len);
				
				//reset text[] and bytesleft
				memset(text, 0, textsize);
				bytesleft = textsize;
			}
			
			//append new line to text[]
			strcat(text, line);
			bytesleft -= strlen(line);
		}
		
		fclose(fp);
		
	}
	
	//send data or "file not found"
	len = strlen(text);
	sendall(socket, text, &len);
}

/*
 * Function to create a data connection
 * Parameters: port number, host name, size of host string
 * Returns: File descriptor for the data connection socket
 *
** source: exclient.c from http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html
*/
int makeDataSocket(int port, char host[], int size)
{
    	int sockfd, portno, n;
    	struct sockaddr_in serv_addr;
    	struct hostent *server;

    	char buffer[256];
    
    	portno = port;
		
	//create a socket
    	sockfd = socket(AF_INET, SOCK_STREAM, 0);
    	if (sockfd < 0) 
        	error("ERROR opening socket");
    	server = gethostbyname("localhost");
    	if (server == NULL) {
        	fprintf(stderr,"ERROR, no such host\n");
        	exit(0);
    	}	
    	
	//fill sockaddr struct with host name and port 
	bzero((char *) &serv_addr, sizeof(serv_addr));
    	serv_addr.sin_family = AF_INET;
    	bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    	serv_addr.sin_port = htons(portno);
    	    
    	sleep(1);//ensure ftclient is listening before trying to connect 

    	if(connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
    	{
	    	perror("ERROR: ");
	    	exit(1);
    	}	    
	
	return sockfd;
	
}

/*
 * Function to set up the initial control connection
 * Parameter: port number
 * Returns file descriptor for the control socket
 */
int makeControlConnection(portno)
{
	int sockfd;
	struct sockaddr_in serv_addr;
	
		//create a new socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
    	if (sockfd < 0) 
       		error("ERROR opening socket");
     
	//zero out struct and set properties
	bzero((char *) &serv_addr, sizeof(serv_addr));
    	serv_addr.sin_family = AF_INET;
    	serv_addr.sin_addr.s_addr = INADDR_ANY;//will work for localhost
    	serv_addr.sin_port = htons(portno); //convert port number to big endian byte order
    
	//http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#setsockoptman
	int optval = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
	
	// bind socket to the address set up above
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
        error("ERROR on binding");
     
	listen(sockfd,5);//start listening on the socket for connections
	
	return sockfd;
}
