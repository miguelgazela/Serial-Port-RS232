#include "Application.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char* argv[]) {
    int functionResult;
    
    if(argc != 3) {
        printf("USAGE: transmitter <portname> <filename>\n");
        exit(-1);
    }
    
    if (strlen(argv[2]) > MAX_FILENAME) {
        printf("Filename is too long for this program to accept.\n");
        exit(-1);
    }
    
    /* creating the application layer */
    applicationLayer* app = getNewApplicationLayer();
    setAs(app, TRANSMITTER);
    
    /* reading the file to be transmitted */
    functionResult = openFile(app, argv[2]);
    
    if(functionResult == INEXISTENT_FILE) {
        printf("The file %s doesn't exist!\n", argv[2]);
        free(app);
        exit(-1);
    }
    else if (functionResult == FSEEK_ERROR) {
        printf("Error obtaining file size.\n");
        free(app);
        exit(-1);
    }
    else if(functionResult == OK) {
        printf("File %s opened with success. File size: %lld bytes.\n", app->filename, app->originalFileSize);
        
        /* create the link layer */
        createNewLinkLayer(argv[1]);
        
        functionResult = sendFile(app);
        
    }
   
    
    free(app);
    return 0;
}