#ifndef LINK_H
#define LINK_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BAUDRATE B38400
#define MAX_ATTEMPTS 3
#define TIMEOUT 3.0
#define MAX_SIZE 256

typedef struct {
    char port[20];
    int baudrate;
    unsigned int sequenceNumber;
    unsigned int timeout;
    unsigned int numTransmissions;
    char frame[MAX_SIZE];
} linkLayer;

linkLayer* createNewLinkLayer(char* portname);

#endif
