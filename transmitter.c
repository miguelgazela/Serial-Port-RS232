#include "Application.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <inttypes.h>

static const char     *sizes[]   = { "EiB", "PiB", "TiB", "GiB", "MiB", "KiB", "B" };
static const uint64_t  exbibytes = 1024ULL * 1024ULL * 1024ULL *
1024ULL * 1024ULL * 1024ULL;

char * calculateSize(uint64_t size)
{
    char* result = (char *) malloc(sizeof(char) * 20);
    uint64_t  multiplier = exbibytes;
    int i;
    
    for (i = 0; i < DIM(sizes); i++, multiplier /= 1024)
    {
        if (size < multiplier)
            continue;
        if (size % multiplier == 0)
            sprintf(result, "%" PRIu64 " %s", size / multiplier, sizes[i]);
        else
            sprintf(result, "%.1f %s", (float) size / multiplier, sizes[i]);
        return result;
    }
    strcpy(result, "0");
    return result;
}

int main(int argc, char* argv[]) {
    unsigned int functionResult, fileblocksize = REGULAR_SIZE_DATAFIELD;
    unsigned int maxTransmissions, timeoutTime;
    int baudrate = -1;
    char* nameFile;
    
    if(argc != 3 && argc != 2 && argc != 4 && argc != 5 && argc != 6) {
        printf("USAGE:\n");
		printf("\ttransmitter <portname>\n");
		printf("\ttransmitter <portname> <filename>\n");
        printf("\ttransmitter <portname> <filename> <piecesFileSize>\n");
		printf("\ttransmitter <portname> <filename> <maxTransmissions> <timeoutTime>\n");
        printf("\ttransmitter <portname> <filename> <baudrate> <maxTransmissions> <timeoutTime>\n");
        exit(-1);
    }
    
    if(argc == 3 || argc == 4 || argc == 5 || argc == 6) {
        if (strlen(argv[2]) > MAX_FILENAME) {
            printf("Filename is too long for this program to accept.\n");
            exit(-1);
        }
        
        if(argc == 4) {
            fileblocksize = atoi(argv[3]);
            
            if(fileblocksize == 0)
                printf("The file block size is invalid. The default value will be used (4096).\n");
        }
        else if (argc == 5) {
            
            if((maxTransmissions = atoi(argv[3])) == 0) {
                printf("The max number of transmissions is invalid. The default value will be used (3).\n");
                maxTransmissions = DEFAULT_MAX_ATTEMPTS;
            }
            
            if((timeoutTime = atoi(argv[4])) == 0) {
                printf("The timeout time is invalid. The default value will be used (5).\n");
                timeoutTime = DEFAULT_TIMEOUT;
            }
        }
        else if (argc == 6) {
            
			if((baudrate = atoi(argv[3])) == 0) {
				printf("The baudrate is invalid. The default value will be used (%d).\n",DEFAULT_BAUDRATE);
                baudrate=-1;
            }
            
            if((maxTransmissions = atoi(argv[4])) == 0) {
                printf("The max number of transmissions is invalid. The default value will be used (3).\n");
                maxTransmissions = DEFAULT_MAX_ATTEMPTS;
            }
            
            if((timeoutTime = atoi(argv[5])) == 0) {
                printf("The timeout time is invalid. The default value will be used (5).\n");
                timeoutTime = DEFAULT_TIMEOUT;
            }
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
    if(argc == 3 || argc == 4 || argc == 5 || argc == 6)
        functionResult = openFile(app, argv[2]);
    else
        functionResult = openFile(app, nameFile);
    
    if(functionResult == INEXISTENT_FILE) {
        if(argc == 3 || argc == 4 || argc == 5 || argc == 6)
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
        if(argc == 5 || argc == 6)
            createNewLinkLayerOptions(argv[1], baudrate, maxTransmissions, timeoutTime);
        else
            createNewLinkLayer(argv[1]);
        
        char* str = calculateSize(app->originalFileSize);
        printf("\nSending file '%s' with size: %s\n", app->filename, str);
        
        functionResult = sendFile(app);
        
        if(functionResult == OK) {
            str = calculateSize(LLayer->totalBytesSent);
            printf("Total data sent (including package and frame headers): %s\n", str);
            str = calculateSize(LLayer->totalBytesSent - app->originalFileSize);
            printf("Extra data sent (not file data): %s\n", str);
            printf("Total packages sent (including control start & end): %lld\n", app->packetsSent+2);
            printf("Number of timeout occurrences: %ld\n", LLayer->numTimeouts);
            printf("Number of received REJ responses: %ld\n", LLayer->numReceivedREJ);
            printf("File %s sent with success.\n\n", app->filename);
            free(str);
        }
            
    }
   
    free(app);
    return 0;
}
