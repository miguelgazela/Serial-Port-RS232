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
    
    app->filename = filename;
    return OK;
}

int loadFile(applicationLayer* app) {
    
    // obtain original file size
    if(fseek(app->pFile, 0, SEEK_END) != 0)
        return FSEEK_ERROR;
    
    app->originalFileSyze = ftell(app->pFile);
    rewind(app->pFile);
    
    // loading file into memory
    if((app->originalFile = (unsigned char*)malloc(sizeof(unsigned char)*app->originalFileSyze)) == NULL) {
        printf("Error allocating memory for file data.\n");
        exit(-1);
    }
    
    return OK;
}