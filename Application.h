#ifndef APPLICATION_H
#define APPLICATION_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "defs.h"

typedef struct {
    int status;
    int fileDescriptor;
    
    FILE* pFile;
    char* filename;
    long originalFileSyze;
    unsigned char* originalFile;
}applicationLayer;

enum statusFlags {TRANSMITTER, RECEIVER};

applicationLayer* getNewApplicationLayer();

void setAs(applicationLayer* app, int flag);
int getStatus(const applicationLayer* app);

void setFileDescriptor(applicationLayer* app, int fd);
int getFileDescriptor(const applicationLayer* app);

int openFile(applicationLayer* app, char* filename);
int loadFile(applicationLayer* app);

#endif
