#ifndef LINK_H
#define LINK_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <termios.h>
#include <sys/select.h>
#include "defs.h"

#define BAUDRATE B38400
#define MAX_ATTEMPTS 3
#define TIMEOUT 3.0
#define MAX_FRAME_SIZE 266

#define FLAG 0x7E
#define A 0x03
#define C_SET 0x03
#define C_UA 0x07
#define C_DISC 0x0B
#define RR 0x05
#define REJ 0x01



typedef struct {
    char port[20];
    int baudrate;
    unsigned int sequenceNumber;
    unsigned int timeout;
    unsigned int numTransmissions;
    char frame[MAX_FRAME_SIZE];
} linkLayer;

linkLayer* LLayer;
struct termios oldtio,newtio;

void createNewLinkLayer(char* portname);

int llopen();
int llclose(int fd);
int llwrite(int fd, void* buffer, int length);
int llread(int fd, void* buffer, int length);

#endif
