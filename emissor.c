/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <time.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define MAX_ATTEMPTS 3
#define TIMEOUT 3.0
#define MAX_SIZE 256

#define FALSE 0
#define TRUE 1

volatile int VALID_RESPONSE=FALSE;

unsigned char SET_COMMAND[] = {FLAG, A, C_SET, (A ^ C_SET), FLAG};
unsigned char UA_COMMAND[] = {FLAG, A, C_UA, (A ^ C_UA), FLAG};
unsigned char DISC_COMMAND[] = {FLAG, A, C_DISC, (A ^ C_DISC), FLAG};
unsigned char UA_ACK_0[] = {FLAG, A, RR, (A ^ RR), FLAG};
unsigned char UA_ACK_1[] = {FLAG, A, RR | 0x20, (A ^ (RR | 0x20)), FLAG};
unsigned char UA_REJ_0[] = {FLAG, A, REJ, (A ^ REJ), FLAG};
unsigned char UA_REJ_1[] = {FLAG, A, REJ | 0x20, (A ^ (REJ | 0x20)), FLAG};

struct termios oldtio,newtio;

int llopen(char* portName, int flag);
int llclose(int fd);

int llwrite(int fd, unsigned char* buffer, int length)
{
		newtio.c_oflag = OPOST;
		tcsetattr(fd, TCSANOW, &newtio);
			
		return (write(fd, buffer, length));
}

int llread(int fd, unsigned char* buffer, int length)
{
		int remaining=length, select_result;
		struct timeval Timeout;
		fd_set readfs;
		time_t tempo_ini = time(NULL);
		double tempo_restante = TIMEOUT;
			
		newtio.c_oflag = 0;
		tcsetattr(fd, TCSANOW, &newtio);				
			
		while(1)
		{
			Timeout.tv_sec = (long int) tempo_restante;			
			Timeout.tv_usec = (tempo_restante - Timeout.tv_sec) * 1000000;
			FD_SET(fd,&readfs);
			
			select_result = select(fd+1, &readfs, NULL, NULL, &Timeout);
						
			tempo_restante = TIMEOUT - difftime(time(NULL),tempo_ini);
			
			if(select_result == 0) {
				printf("Timeout ocurred\n");
				break;
			}			
			
			if(FD_ISSET(fd, &readfs)) {
				remaining -= read(fd, &buffer[length-remaining], remaining);
				
				if(remaining == 0)
					return length;
			}
		}
		
		if(remaining<length)
			return length-remaining;
		else
			return -1;	
}

int llopen(char * portName, int flag) {
	int fd, counter = 0, attempts = MAX_ATTEMPTS;
	int remaining, select_result;
	struct timeval Timeout;
	fd_set readfs;
	time_t tempo_ini = time(NULL);
	double tempo_restante = TIMEOUT;
   
    unsigned char UA_RESPONSE[5];
	
	/*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */

    fd = open(portName, O_RDWR | O_NOCTTY );
    if (fd < 0) {
		perror(portName); 
		return -1; 
	}

    if (tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      return -1;
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = OPOST;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */

  /* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) próximo(s) caracter(es)
  */
    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      return -1;
    }
    printf("New termios structure set\n");
    
    do {
		int set_attempts = MAX_ATTEMPTS;
		remaining = 5;
		while(remaining > 0 && set_attempts>0)
		{
			remaining -= write(fd, &SET_COMMAND[5-remaining], remaining);
			set_attempts--;
		}
		
		if(remaining==0)
			printf("SET_COMMAND SENT...\n");
		else
		{
			printf("CAN'T SEND SET_COMMAND...\n");
			return -1;
		}		
		
		newtio.c_oflag = 0;
		tcsetattr(fd,TCSANOW,&newtio);
		
		remaining = 5;
		
		while(1)
		{
			Timeout.tv_sec = (long int) tempo_restante;			
			Timeout.tv_usec = (tempo_restante - Timeout.tv_sec) * 1000000;
			FD_SET(fd,&readfs);
			
			select_result = select(fd+1, &readfs, NULL, NULL, &Timeout);
						
			tempo_restante = TIMEOUT - difftime(time(NULL),tempo_ini);
			
			if(select_result == 0) {
				printf("Timeout ocurred. Can't establish connection\n");
				exit(-1);
			}			
			
			if(FD_ISSET(fd, &readfs)) {
				remaining -= read(fd, &UA_RESPONSE[5-remaining], remaining);
				
				if(remaining == 0)
					break;
			}
		}
		
		int valid_ua_response=TRUE;
		counter=0;
		while(counter<5)
		{
			 if(UA_RESPONSE[counter] != UA_COMMAND[counter])
				{
					valid_ua_response=FALSE;
					break;
				}	
				counter++;
		}
		
		if(valid_ua_response == TRUE)
		{
			VALID_RESPONSE = TRUE;
			printf("Port is now open...\n");
		}
		else
			attempts--;
				
	} while (VALID_RESPONSE == FALSE && attempts>0);
	
	if (VALID_RESPONSE == FALSE)
	{
		printf("Can't get a valid UA_RESPONSE...\n");
		return -1;
	}

	return fd;
	
}

int llclose(int fd) {
	int counter = 0, attempts = MAX_ATTEMPTS;
	int remaining, select_result;
	struct timeval Timeout;
	fd_set readfs;
	time_t tempo_ini = time(NULL);
	double tempo_restante = TIMEOUT;
	unsigned char DISC_RESPONSE[5];
	
	 do {
		 
		int disc_attempts = MAX_ATTEMPTS;
		remaining = 5;
		while(remaining > 0 && disc_attempts>0)
		{
			remaining -= write(fd, &DISC_COMMAND[5-remaining], remaining);
			disc_attempts--;
		}
		
		if(remaining==0)
			printf("DISC_COMMAND SENT...\n");
		else
		{
			printf("CAN'T SEND DISC_COMMAND...\n");
			return -1;
		}
		
		newtio.c_oflag = 0;
		tcsetattr(fd,TCSANOW,&newtio);
		
		remaining = 5;
		
		while(1)
		{
			Timeout.tv_sec = (long int) tempo_restante;			
			Timeout.tv_usec = (tempo_restante - Timeout.tv_sec) * 1000000;
			FD_SET(fd,&readfs);
			
			select_result = select(fd+1, &readfs, NULL, NULL, &Timeout);
						
			tempo_restante = TIMEOUT - difftime(time(NULL),tempo_ini);
			
			if(select_result == 0) {
				printf("Timeout ocurred. Can't close connection\n");
				return(-1);
			}			
			
			if(FD_ISSET(fd, &readfs)) {
				remaining -= read(fd, &DISC_RESPONSE[5-remaining], remaining);
				
				if(remaining == 0)
					break;
			}
		}
		
		int valid_disc_response=TRUE;
		counter=0;
		while(counter<5)
		{
			 if(DISC_RESPONSE[counter] != DISC_COMMAND[counter])
				{
					valid_disc_response=FALSE;
					break;
				}	
				counter++;
		}
		
		if(valid_disc_response == TRUE)
		{
			VALID_RESPONSE = TRUE;
			printf("Port is now closing...\n");
			
			int ua_attempts = MAX_ATTEMPTS;
			newtio.c_oflag = OPOST;
			tcsetattr(fd,TCSANOW,&newtio);	
			remaining = 5;
			while(remaining > 0 && ua_attempts>0)
			{
				remaining -= write(fd, &UA_COMMAND[5-remaining], remaining);
				ua_attempts--;
			}	
			if(remaining==0)			
				printf("UA_COMMAND SENT...\n");			
			else
			{
				printf("CAN'T SEND UA_COMMAND...\n");
				return -1;
			}
		}
		else
			attempts--;
				
	} while (VALID_RESPONSE == FALSE && attempts>0);
	
	if (VALID_RESPONSE == FALSE)
	{
		printf("Can't get a valid disconnection response...\n");
		return -1;
	}
	
	if (tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      return -1;
    }
    
    return close(fd);
}
