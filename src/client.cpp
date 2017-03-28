#include <fcntl.h>
#include <string.h>
#include <sstream>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <netinet/in.h>
#include <resolv.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <chrono>
using Clock = std::chrono::high_resolution_clock;
using Ms = std::chrono::milliseconds;
using Sec = std::chrono::seconds;
template<class Duration>
using TimePoint = std::chrono::time_point<Clock, Duration>;

typedef std::chrono::high_resolution_clock Time;
typedef std::chrono::milliseconds ms;
typedef std::chrono::duration<float> fsec;

int msgLength = 128;

class TCPClient{
	private:
		int freeSpace = 1024;
		int rateControl = 512; // byte / s
		bool streamFinished = false;
		bool UDPnotRunning = true;
		int reqState = 0; // 1 = play, 2 = pause, 3 = stop, 4 = streaming
		struct sockaddr_in my_addr;
		int bytecount;
		//int bytecount2;
		//int buffer_len=0;
		int hsock;
		int * p_int;
		int err;
		bool Req(){
			bool out = true;
			if (reqState == 1) {
				playReq();
				reqState = 4;
			} else if (reqState == 2)
				pauseReq();
			else if (reqState == 3) {
				// Chisura del client UDP
				stopReq();
				close(hsock);
				out = false;
			}
			return out;
		}
		void playReq(){
			// Invio del comando PLAY al server
			char msg2[msgLength] = "CLIENT INFO: play";
			//printf("Sending %s\n", msg2);
			if( (bytecount=send(hsock, msg2, msgLength,0)) == -1){
				fprintf(stderr, "Error sending data %d\n", errno);
				return;
			}
			printf("Sent: %s\n", msg2);
			//delete msg2;
		}
		void pauseReq(){
			// Invio del comando PAUSE al server
			char msg2[msgLength] = "CLIENT INFO: pause";
			//printf("Sending %s\n", msg2);
			if( (bytecount=send(hsock, msg2, msgLength,0)) == -1){
				fprintf(stderr, "Error sending data %d\n", errno);
				return;
			}
			printf("Sent: %s\n", msg2);
			usleep(500000);
			freeSpace = 1024; // consumo dati buffer
			reqState = 1; // una volta consumati (callback): richiesta di Play
		}
		void stopReq(){
			// Invio del comando STOP al server
			char msg2[msgLength] = "CLIENT INFO: stop";
			//printf("Sending %s\n", msg2);
			if( (bytecount=send(hsock, msg2, msgLength,0)) == -1){
				fprintf(stderr, "Error sending data %d\n", errno);
				return;
			}
			printf("Sent: %s\n", msg2);
			//delete msg2;
		}
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
			}
			free(p_int);
			my_addr.sin_family = AF_INET ;
			my_addr.sin_port = htons(tcp_port);

			memset(&(my_addr.sin_zero), 0, 8);
			my_addr.sin_addr.s_addr = inet_addr(host_name);
			if( connect( hsock, (struct sockaddr*)&my_addr, sizeof(my_addr)) == -1 ){
				if((err = errno) != EINPROGRESS){
					fprintf(stderr, "Error connecting socket %d\n", errno);
				}
			}
		}
		
		bool Send(char* msg, int msg_length){
			printf("Send function begins\n");
			// Invio del pacchetto CLIENT INFO: stream name
			printf("Sending: %s\n", msg);
			if( (bytecount=send(hsock, msg, msg_length,0))== -1){
				fprintf(stderr, "Error sending data %d\n", errno);
				return false;
			}
			
			// Attesa della risposta del server con SERVER INFO
			if((bytecount = recv(hsock, msg, msg_length, 0))== -1) {
				fprintf(stderr, "Error receiving data %d\n", errno);
				//goto FINISH;
				return false;
			}
			printf("Received: %s\n", msg);
			
			// INVIO pacchetti UDP sulle porte indicate dal server (per aprire le porte)
			printf("*** Opening UDP port ***\n");
			// ...
			
			// Richiesta di PLAY
			reqState = 1;
			Req();

			auto t0 = Time::now();
			
			while(true) { // riceve lo stream e continua a controllare che il buffer non sia pieno
				if (!Req()) // Se lo streaming è finito
					return true;
				// variabili condivise: spazio libero buffer,
				// info dal server: percentuale di completamento
				//printf("reqState: %d\n",reqState);
				if (streamFinished){ 
					reqState = 3;
				} else if ((freeSpace<=0)&&(reqState==4)){ // Se il buffer è pieno e il sistema è ancora in streaming
					reqState = 2; // Imposto lo stato della richiesta al server su PAUSE
				} else if (reqState == 4) { // Se il buffer non è pieno e devo fare streaming
					if ((freeSpace > 0)&&(UDPnotRunning)){
						/* Riceve UDP packet attraverso una funzione bloccante in un thread separato,
							parametri: *streamFinished, *freeSpace, porta
							riceve pacchetto UDP
							decodifica
							aggiorna streamFinished
							aggiorna freeSpace
						*/
						usleep(10000);
						//UDPnotRunning = UDP;
						UDPnotRunning = false;
					} else if (freeSpace<=0) {
						reqState = 2;
					} else {
						auto t1 = Time::now();
						fsec fs = t1 - t0;
						ms d = std::chrono::duration_cast<ms>(fs);
						int millis = d.count();
						freeSpace = freeSpace - 1;
						usleep(1000);
						if (millis>5000)
							streamFinished = true;
					}
				}
			}
		}
};

int main(int argv, char** argc){
	char host_name[] = "127.0.0.1";
	int tcp_port= 1101;
	TCPClient my_tcp(tcp_port, host_name);
	
	/*
	char buffer[msgLength];
	memset(buffer, '\0', msgLength);
	printf("Enter some text to send to the server (press enter)\n");
	fgets(buffer, 1024, stdin);
	//msgLength = strlen(buffer);
	buffer[msgLength-1]='\0';
	*/
	char clientInfo[msgLength] = "CLIENT INFO: stream name";
	my_tcp.Send(clientInfo,msgLength);
	
}
