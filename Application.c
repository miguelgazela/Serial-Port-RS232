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
    
    app->originalFileSyze = ftell(app->pFile);
    rewind(app->pFile);
    
    app->filename = filename;
    return OK;
}

int sendFile(applicationLayer* app) {
    int remaining;
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
    
    fileData = (dataPackage*)malloc(sizeof(dataPackage));
    
    if(app->originalFileSyze <= 256) { /* it only needs one package */
        remaining = app->originalFileSyze;
        
        while(TRUE) {
            remaining -= fread(&fileData->dataField[app->originalFileSyze-remaining], 1, remaining, app->pFile);
            
            if (remaining == 0)
                break;
        }
    }
    
    /* EXPERIMENTAR ISTO E DEPOIS APAGAR */
    for(result = 0; result < app->originalFileSyze; result++)
        printf("CHAR number: %d value: %x\n", result, fileData[result]);
    
    
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
    app->ctrlPkg.L_Size = sizeof(unsigned long long int); /* size_t = 8 */
    app->ctrlPkg.V_Size = app->originalFileSyze;
    
    /* filename TLV */
    app->ctrlPkg.T_Name = T_NAME;
    app->ctrlPkg.L_Name = (unsigned char)strlen(app->filename);
    memcpy(app->ctrlPkg.V_Name, app->filename, strlen(app->filename));
}

