#ifndef APPLICATION_H
#define APPLICATION_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Link.h"
#include <math.h>

typedef struct {
	unsigned char C, T_Size, L_Size;
	unsigned long long int V_Size;
	unsigned char T_Name, L_Name;
	char V_Name[MAX_FILENAME];
   	unsigned char T_Pkg, L_Pkg;
	unsigned long long int V_Pkg;
} controlPackage;

typedef struct {
    unsigned char C, N, L2, L1;
    unsigned char dataField[1];
} dataPackage;

typedef struct {
    int status, fileDescriptor, fileblocksize;
    FILE* pFile;
    char* filename;
    unsigned long long int originalFileSize, packetsSent; /* 64 bits */
    unsigned char* buff;
    controlPackage ctrlPkg;
} applicationLayer;

enum statusFlags {TRANSMITTER, RECEIVER};
enum applicationLayerErrors {INEXISTENT_FILE = 61, FSEEK_ERROR};
enum applicationLayerPackageC {C_DATA = 0, C_START, C_END};
enum applicationLayerPackageT {T_SIZE = 0, T_NAME, T_PKG};

applicationLayer* getNewApplicationLayer();

void setAs(applicationLayer* app, int flag);
int getStatus(const applicationLayer* app);

int openFile(applicationLayer* app, char* filename);
int sendFile(applicationLayer* app);

void setControlPackage(applicationLayer* app);
void setFileBlockSize(applicationLayer* app, int fileblocksize);

#endif
