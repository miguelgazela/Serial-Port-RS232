#include "Application.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char* argv[]) {
    int functionResult, fileblocksize;
    char* nameFile;
    
    if(argc != 3 && argc != 2 && argc != 4) {
        printf("USAGE:\n\ttransmitter <portname>\n\ttransmitter <portname> <filename>\n");
        printf("\ttransmitter <portname> <filename> <piecesFileSize>\n");
        exit(-1);
    }
    
    if(argc == 3 || argc == 4) {
        if (strlen(argv[2]) > MAX_FILENAME) {
            printf("Filename is too long for this program to accept.\n");
            exit(-1);
        }
        
        if(argc == 4) {
            fileblocksize = atoi(argv[3]);
            
            if(fileblocksize == 0)
                printf("The file block size is invalid. The default value will be used.\n");
        }
    }
    else if(argc == 2){
        nameFile = (char*)malloc(sizeof(char)*MAX_FILENAME);
        printf("File name (max 128 characters): ");
        scanf("%s", nameFile);
    }
    
    /* creating the application layer */
    applicationLayer* app = getNewApplicationLayer();
    setAs(app, TRANSMITTER);
    setFileBlockSize(app, fileblocksize);
    
    /* reading the file to be transmitted */
    if(argc == 3 || argc == 4)
        functionResult = openFile(app, argv[2]);
    else
        functionResult = openFile(app, nameFile);
    
    if(functionResult == INEXISTENT_FILE) {
        if(argc == 3 || argc == 4)
            printf("The file %s doesn't exist!\n", argv[2]);
        else if(argc == 2)
            printf("The file %s doesn't exist!\n", nameFile);
        
        free(app);
        exit(-1);
    }
    else if (functionResult == FSEEK_ERROR) {
        printf("Error obtaining file size.\n");
        
        free(app);
        exit(-1);
    }
    else if(functionResult == OK) {
        printf("\033[2J");
        printf("File %s opened with success.\n", app->filename);
        
        /* create the link layer */
        createNewLinkLayer(argv[1]);
        
        functionResult = sendFile(app);
        
        if(functionResult == OK) {
            
            printf("Number of timeout occurrences: %ld\n", LLayer->numTimeouts);
            printf("Number of received REJ responses: %ld\n", LLayer->numReceivedREJ);
            printf("File %s sent with success.\n\n", app->filename);
        }
            
    }
   
    free(app);
    return 0;
}