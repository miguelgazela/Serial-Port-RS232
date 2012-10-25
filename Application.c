#include "Application.h"

int DEBUG_APP = FALSE;

inline void loadBar(unsigned long long int x, unsigned long long int n, int r, int w)
{
    // Only update r times.
    if ( x % (n/r) != 0 ) return;
    
    // Calculuate the ratio of complete-to-incomplete.
    float ratio = x/(float)n;
    int   c     = ratio * w;
    
    // Show the percentage complete.
    printf("%3d%% [", (int)(ratio*100) );
    
    // Show the load bar.
    for (x = 0; x<c; x++)
        printf("=");
    
    for (x = c; x<w; x++)
        printf(" ");
    
    // ANSI Control codes to go back to the
    // previous line and clear it.
    printf("]\n\033[F\033[J");
}

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
    if(LLayer == NULL) {
        printf("There isn't an active link layer. File cannot be sent.");
        exit(-1);
    }
    
    if((app->fileDescriptor = llopen()) < 0)
        exit(-1);
    
    /* send start control package */
    setControlPackage(app);
    
    printf("\nSending file '%s' with size: %lld bytes\n", app->filename, app->originalFileSize);
    
    if(DEBUG_APP)
        printf("\nPackage control START\n");
    
    result = llwrite(app->fileDescriptor, (unsigned char*)&app->ctrlPkg, sizeof(controlPackage));
    
    if (result < 0) {
        perror("Package control delivery");
        exit(-1);
    }
    
    /* send file data packages */
    remainingFileBytes = app->originalFileSize;
    
    while (remainingFileBytes > 0) { /* while there's still data to send */
        
        fileData = (dataPackage*)malloc(sizeof(dataPackage));
        fileData->C = C_DATA;
        fileData->N = (unsigned char)packetsSent % 255;
        
        if(!DEBUG_APP) {
            if(app->ctrlPkg.V_Pkg < 50)
                loadBar(packetsSent, app->ctrlPkg.V_Pkg, app->ctrlPkg.V_Pkg, 70);
            else
                loadBar(packetsSent, app->ctrlPkg.V_Pkg, 50, 70);
        }
        
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
        
        if(DEBUG_APP)
            printf("Package number: %lld\nRemaining bytes to be sent: %lld\n", packetsSent, remainingFileBytes);
        
        result = llwrite(app->fileDescriptor, (unsigned char*)fileData, sizeof(dataPackage));
        
        if(result == -1) {
            printf("Error sending a data package.\n");
            exit(-1);
        }
        
        packetsSent++;
        free(fileData);
    }
    
    /* send end control package */
    app->ctrlPkg.C = C_END;
    
    if(DEBUG_APP)
        printf("Package control END\n");
    
    result = llwrite(app->fileDescriptor, (unsigned char*)&app->ctrlPkg, sizeof(controlPackage));
    
    if (result < 0) {
        perror("Package control delivery");
        exit(-1);
    }
    
    printf("Total bytes sent (including package and frame headers): %lld\n", LLayer->totalBytesSent);
    printf("Total packages sent (including control start & end): %lld\n\n", packetsSent+2);

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
    
    if(DEBUG_APP) {
        printf("Control Package: Start\n");
        printf("t_size: %d\tl_size: %d\tv_size: %lld\nt_name: %d\tl_name: %d\tv_name: %s\nt_pkg: %d\tl_pkg: %d\tv_pkg: %lld\n",
           app->ctrlPkg.T_Size, app->ctrlPkg.L_Size, app->ctrlPkg.V_Size, app->ctrlPkg.T_Name, app->ctrlPkg.L_Name,
           app->ctrlPkg.V_Name, app->ctrlPkg.T_Pkg, app->ctrlPkg.L_Pkg, app->ctrlPkg.V_Pkg);
    }
}

