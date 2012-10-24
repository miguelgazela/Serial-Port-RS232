#include "Application.h"

applicationLayer* getNewApplicationLayer() {
    /* allocate memory */
    applicationLayer* appPtr = (applicationLayer*)malloc(sizeof(applicationLayer));
    
    if(appPtr == NULL) {
        printf("Error allocating memory for new applicationLayer.\n");
        exit(-1);
    }
    
    return appPtr;
}

void setAs(applicationLayer* app, int flag) {
    app->status = flag;
}

int getStatus(const applicationLayer* app) {
    return app->status;
}

void setFileDescriptor(applicationLayer* app, int fd) {
    app->fileDescriptor = fd;
}

int getFileDescriptor(const applicationLayer* app) {
    return app->fileDescriptor;
}

int openFile(applicationLayer* app, char* filename) {
    
    if((app->pFile = fopen(filename, "rb")) == NULL)
        return INEXISTENT_FILE;
    
    // obtain original file size
    if(fseek(app->pFile, 0, SEEK_END) != 0)
        return FSEEK_ERROR;
    
    app->originalFileSize = ftell(app->pFile);
    rewind(app->pFile);
    
    app->filename = filename;
    return OK;
}

int sendFile(applicationLayer* app) {
    unsigned long long int remainingFileBytes, packetsSent = 0;
    int remaining, result;
    dataPackage* fileData;
    
    /* open the link layer */
    if((app->fileDescriptor = llopen()) < 0)
        exit(-1);
    
    /* send start control package */
    setControlPackage(app);
    
    result = llwrite(app->fileDescriptor, &app->ctrlPkg, sizeof(controlPackage));
    
    if (result < 0) {
        perror("Package control delivery");
        exit(-1);
    }
    
    /* send file data packages */
    remainingFileBytes = app->originalFileSize;
    
    printf("Original remaining file bytes %lld\n", remainingFileBytes); /* TODO: remove */
    
    while (remainingFileBytes > 0) { /* while there's still data to send */
        fileData = (dataPackage*)malloc(sizeof(dataPackage));
        
        fileData->C = C_DATA;
        fileData->N = (unsigned char)packetsSent % 255;
        
        if(remainingFileBytes >= MAX_SIZE_DATAFIELD) {
            
            fileData->L1 = 0; fileData->L2 = 1;
            remaining = MAX_SIZE_DATAFIELD;
            
            while (TRUE) {
                remaining -= fread(&fileData->dataField[MAX_SIZE_DATAFIELD-remaining], 1, remaining, app->pFile);
                
                if(remaining == 0) /* successfully read all bytes */ {
                    remainingFileBytes -= MAX_SIZE_DATAFIELD;
                    break;
                }
            }
        }
        else {
            fileData->L1 = remainingFileBytes; fileData->L2 = 0;
            remaining = remainingFileBytes;
            
            while (TRUE) {
                remaining -= fread(&fileData->dataField[remainingFileBytes-remaining], 1, remaining, app->pFile);
                
                if(remaining == 0) {
                    remainingFileBytes = 0;
                    break;
                }
            }
        }
        
        printf("Updated remaining file bytes %lld\nPackage number: %lld\n", remainingFileBytes, packetsSent);
        
        result = llwrite(app->fileDescriptor, fileData, sizeof(dataPackage));
        
        if(result == -1) { /* TODO: deve sair aqui? Ou tbm deve fazer tentativas??? */
            printf("Error sending a data package.\n");
            exit(-1);
        }
        
        packetsSent++;
        free(fileData);
    }
    
    /* send end control package */
    app->ctrlPkg.C = C_END;
    
    result = llwrite(app->fileDescriptor, &app->ctrlPkg, sizeof(controlPackage));
    
    if (result < 0) {
        perror("Package control delivery");
        exit(-1);
    }

    return OK;
}

void setControlPackage(applicationLayer* app) {
    app->ctrlPkg.C = C_START;
    
    /* file size TLV */
    app->ctrlPkg.T_Size = T_SIZE;
    app->ctrlPkg.L_Size = (unsigned char)sizeof(unsigned long long int); /* size_t = 8 */
    app->ctrlPkg.V_Size = app->originalFileSize;
    
    /* filename TLV */
    app->ctrlPkg.T_Name = T_NAME;
	app->ctrlPkg.L_Name = MAX_FILENAME;
    memcpy(app->ctrlPkg.V_Name, app->filename, strlen(app->filename));
    
    /* number of data packages needed in ideal conditions */
    app->ctrlPkg.T_Pkg = T_PKG;
    app->ctrlPkg.L_Pkg = (unsigned char)sizeof(unsigned long long int);
    
    if((app->originalFileSize % MAX_SIZE_DATAFIELD) == 0)
        app->ctrlPkg.V_Pkg = (unsigned long long int)app->originalFileSize/MAX_SIZE_DATAFIELD;
    else
        app->ctrlPkg.V_Pkg = (unsigned long long int)ceil((double)app->originalFileSize/MAX_SIZE_DATAFIELD);
    
    /* TODO: retirar isto */
    printf("ESTADO DO CTRL PACKAGE\n");
	printf("t_size: %d l_size: %d v_size: %lld t_name: %d l_name: %d v_name: %s t_pkg: %d l_pkg: %d v_pkg: %lld\n",
           app->ctrlPkg.T_Size, app->ctrlPkg.L_Size, app->ctrlPkg.V_Size, app->ctrlPkg.T_Name, app->ctrlPkg.L_Name,
           app->ctrlPkg.V_Name, app->ctrlPkg.T_Pkg, app->ctrlPkg.L_Pkg, app->ctrlPkg.V_Pkg);
}

