#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <netinet/in.h>
#include <resolv.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

class TCPClient{
	private:
		struct sockaddr_in my_addr;
		int bytecount;
		int bytecount2;
		//int buffer_len=0;
		int hsock;
		int * p_int;
		int err;
	public:
		TCPClient(int tcp_port, char* host_name){
			hsock = socket(AF_INET, SOCK_STREAM, 0);
			if(hsock == -1){
				printf("Error initializing socket %d\n",errno);
				//goto FINISH;
			}
			p_int = (int*)malloc(sizeof(int));
			*p_int = 1;
			if( (setsockopt(hsock, SOL_SOCKET, SO_REUSEADDR, (char*)p_int, sizeof(int)) == -1 )||
				(setsockopt(hsock, SOL_SOCKET, SO_KEEPALIVE, (char*)p_int, sizeof(int)) == -1 ) ){
				printf("Error setting options %d\n",errno);
				free(p_int);
				//goto FINISH;
			}
			free(p_int);
			my_addr.sin_family = AF_INET ;
			my_addr.sin_port = htons(tcp_port);

			memset(&(my_addr.sin_zero), 0, 8);
			my_addr.sin_addr.s_addr = inet_addr(host_name);
			if( connect( hsock, (struct sockaddr*)&my_addr, sizeof(my_addr)) == -1 ){
				if((err = errno) != EINPROGRESS){
					fprintf(stderr, "Error connecting socket %d\n", errno);
					//goto FINISH;
				}
			}
		}
		
		bool Send(char* msg, int msg_length){
			/* Invia pacchetto CLIENT INFO:
				STREAM NAME
				PLAY / PAUSE / STOP
				UDP RANGE (?) (USELESS)
				BUFFER STATE
				LINK STATE
			*/
			if( (bytecount=send(hsock, msg, msg_length,0))== -1){
				fprintf(stderr, "Error sending data %d\n", errno);
				//goto FINISH;
				return false;
			}
			printf("Sent bytes %d\n", bytecount);
			
			// Aspetta la risposta del server che conterrà SERVER INFO
			if((bytecount = recv(hsock, msg, msg_length, 0))== -1) {
				fprintf(stderr, "Error receiving data %d\n", errno);
				//goto FINISH;
				return false;
			}
			printf("Recieved bytes %d\nServer Info Received: \"%s\"\n", bytecount, msg);
			
			// Si mette in ascolto sulle porte UDP indicate da Server Info (apre un nuovo thread)
			// Invia il comando PLAY al server
			char msg2[] = "PLAY";
			int msg2len = 10;
			if( (bytecount2=send(hsock, msg2, msg2len,0)) == -1){
				fprintf(stderr, "Error sending data %d\n", errno);
				//goto FINISH;
				return false;
			}
			printf("Sent bytes %d\nSent command \"%s\"\n", bytecount2, msg2);
			close(hsock);
			return true;
		}
};

int main(int argv, char** argc){
	//char* msg_example = 'asd';
	char host_name[] = "127.0.0.1";
	int tcp_port= 1101;
	TCPClient my_tcp(tcp_port, host_name);
	
	int buffer_len = 1024; // lunghezza massima
	char buffer[buffer_len];
	memset(buffer, '\0', buffer_len);
	printf("Enter some text to send to the server (press enter)\n");
	fgets(buffer, 1024, stdin);
	//buffer_len = strlen(buffer);
	buffer[buffer_len-1]='\0';
	
	my_tcp.Send(buffer,buffer_len);
	
}
