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
#include <pthread.h>
#include <stdexcept>

int msgLength = 128;

void* SocketHandler(void*);
void* SocketHandler(void* lp){
	// Ho ricevuto una connessione in ingresso da un client (TCP)
	int *csock = (int*)lp;
	
	// Attendo la ricezione di Client Info
	char clientInfo[msgLength];
	int bytecount;
	memset(clientInfo, 0, msgLength);
	if((bytecount = recv(*csock, clientInfo, msgLength, 0))== -1){
		fprintf(stderr, "Error receiving data %d\n", errno);
		free(csock);
	}
	printf("Received: %s\n", clientInfo);
	
	// Creazione Data Server e Encoder
	printf("*** Creation of Data Server and Encoder ***\n");
	
	// Invio al client del pacchetto Server Info (UDP Range, Dimensione video, Errori, UEP Params)
	char serverInfo[msgLength] = "SERVER INFO: udp port";
	printf("Sending: %s\n",serverInfo);
	if((bytecount = send(*csock, serverInfo, msgLength, 0))== -1){
		fprintf(stderr, "Error sending data %d\n", errno);
		//goto FINISH;
	}
	/*
	while (true) {
		//Attesa del pacchetto Client info con Play / Pause / Stop
		//buffer = char[1024];
		
		// buffer contiene PLAY / PAUSE / STOP
			//	se è STOP
				
					close(*csock);
					free(csock);
					Distrugge Encoder, Data Server
					return 0;		
				
			//	se è PLAY
			//		recv non bloccante
			//		Fa girare l'encoder che manda via UDP
			//			UDP chiama encode / next packet
			//	se è PAUSE
			//		recv bloccante
	}*/
	// Attendo il comando PLAY
	char command[msgLength];
	if((bytecount = recv(*csock, command, msgLength, 0))== -1){
		fprintf(stderr, "Error receiving data %d\n", errno);
		free(csock);
	}
	printf("Received: %s\n", command);
	return 0;
}

class TCPServer{
	private:
		struct sockaddr_in my_addr;
		int hsock;
		int * p_int ;
		//int err;
		char err_desc [100];
		socklen_t addr_size;
		int* csock;
		sockaddr_in sadr;
		pthread_t thread_id;		
	
	public:
		TCPServer(int tcp_port){
			using namespace std;
			addr_size = 0;
			thread_id = 0;
			//Inizializing socket
			hsock = socket(AF_INET, SOCK_STREAM, 0);
			if (hsock == -1){
				//printf("Error initializing socket %d\n", errno);
				free(csock);
				throw runtime_error("Error initializing TCP socket");
			}
			p_int = (int*)malloc(sizeof(int));
			*p_int = 1;
			
			if( (setsockopt(hsock, SOL_SOCKET, SO_REUSEADDR, (char*)p_int, sizeof(int)) == -1 )||
				(setsockopt(hsock, SOL_SOCKET, SO_KEEPALIVE, (char*)p_int, sizeof(int)) == -1 )){
				//printf("Error setting options %d\n", errno);
				free(p_int);
				free(csock);
				throw runtime_error("Error setting options of TCP socket");
			}
			free(p_int);

			my_addr.sin_family = AF_INET ;
			my_addr.sin_port = htons(tcp_port);

			memset(&(my_addr.sin_zero), 0, 8);
			my_addr.sin_addr.s_addr = INADDR_ANY;
		}
		
		int Listen(){
			using namespace std;
			if( bind( hsock, (sockaddr*)&my_addr, sizeof(my_addr)) == -1 ){
				sprintf(err_desc,"Error binding TCP socket, make sure nothing else is listening on this port %d\n",errno);
				free(csock);
				throw runtime_error(err_desc);
			}
			if(listen( hsock, 10) == -1 ){
				sprintf(err_desc, "Error listening TCP socket %d\n",errno);
				free(csock);
				throw runtime_error(err_desc);
			}
		 
			//Now lets do the server stuff

			addr_size = sizeof(sockaddr_in);
			 
			while(true){
				printf("waiting for a connection\n");
				csock = (int*)malloc(sizeof(int));
				if((*csock = accept( hsock, (sockaddr*)&sadr, &addr_size))!= -1){
					printf("---------------------\nReceived connection from %s\n",inet_ntoa(sadr.sin_addr));
					pthread_create(&thread_id,0,&SocketHandler, (void*)csock );
					pthread_detach(thread_id);
				} else {
					fprintf(stderr, "Error accepting %d\n", errno);
				}
			}
			return 0;
		}

};

int main(int argv, char** argc){
	int tcp_port= 1101;
	TCPServer my_tcp(tcp_port);
	my_tcp.Listen();
	return 0;
}