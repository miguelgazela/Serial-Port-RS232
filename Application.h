#ifndef APPLICATION_H
#define APPLICATION_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Link.h"

typedef struct {
    unsigned long long int V_Size;
    char V_Name[MAX_FILENAME];
    unsigned char C, T_Size, L_Size, T_Name, L_Name;
} controlPackage;


typedef struct {
    unsigned char C, N, L2, L1;
    unsigned char dataField[MAX_SIZE_DATAFIELD];
} dataPackage;

typedef struct {
    int status;
    int fileDescriptor;
    
    FILE* pFile;
    char* filename;
    unsigned long long int originalFileSyze; /* 64 bits */
    unsigned char* buff;
    controlPackage ctrlPkg;
} applicationLayer;

enum statusFlags {TRANSMITTER, RECEIVER};
enum applicationLayerErrors {INEXISTENT_FILE = 61, FSEEK_ERROR};
enum applicationLayerPackageC {C_DATA = 0, C_START, C_END};
enum applicationLayerPackageT {T_SIZE = 0, T_NAME};

applicationLayer* getNewApplicationLayer();

void setAs(applicationLayer* app, int flag);
int getStatus(const applicationLayer* app);

void setFileDescriptor(applicationLayer* app, int fd);
int getFileDescriptor(const applicationLayer* app);

int openFile(applicationLayer* app, char* filename);
int sendFile(applicationLayer* app);

void setControlPackage(applicationLayer* app);

#endif
