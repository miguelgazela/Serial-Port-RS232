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

void usage() {
    printf("USAGE:\n");
    printf("\t-f<filename>\n");
    printf("\t-p<portname>\n");
    printf("\t-s<blockFileSize>\n");
    printf("\t-r<maxRetransmissions>\n");
    printf("\t-t<timeoutTime>\n");
    printf("\t-b<baudrate>\n");
}

int main(int argc, char* argv[]) {
    unsigned int functionResult, fileblocksize = REGULAR_SIZE_DATAFIELD;
    unsigned int maxRetransmissions = DEFAULT_MAX_ATTEMPTS, timeoutTime = DEFAULT_TIMEOUT;
    int baudrate = -1, hasFilename = FALSE, hasPortname = FALSE, tempArgc = argc;
    char *filename, *portname;
    
    /* creating the application layer */
    applicationLayer* app = getNewApplicationLayer();
    setAs(app, TRANSMITTER);
    
    while ((tempArgc > 1) && (argv[1][0] == '-'))
    {
        switch (argv[1][1])
        {
            case 'f': { // filename
                hasFilename = TRUE;
                if(strlen(&argv[1][2]) > MAX_FILENAME) {
                    printf("Filename is too long for this program to aceept.\n");
                    return(-1);
                }
                filename = &argv[1][2];
            }
                break;
                
            case 'p': { // portname
                hasPortname = TRUE;
                portname = &argv[1][2];
            }
                break;
                
            case 's': // file blocks size
                if((fileblocksize = atoi(&argv[1][2])) == 0)
                    printf("The file block size is invalid. The default value will be used (4096).\n");
                break;
                
            case 'r': // num of retransmissions
                if((maxRetransmissions = atoi(&argv[1][2])) == 0)
                    printf("The max number of retransmissions is invalid. The default value will be used (3).\n");
                break;
                
            case 't': // timeout time
                if((timeoutTime = atoi(&argv[1][2])) == 0)
                    printf("The timeout time is invalid. The default value will be used (5).\n");
                break;
                
            case 'b': // baudrate
                if((baudrate = atoi(&argv[1][2])) == 0)
                    printf("The baudrate is invalid. The default value will be used (%d).\n",DEFAULT_BAUDRATE);
                break;
            case 'd': { // DEBUG MODE
                DEBUG_APP = TRUE;
                DEBUG_LINK = TRUE;
            }
                break;
                
            default:
                printf("Wrong Argument: %s\n", &argv[1][2]);
                usage();
        }
        
        ++argv;
        --tempArgc;
    }

    if(hasFilename == FALSE && hasPortname == FALSE) {
        printf("The program needs at least the name of the file to send, and the serial port to use.\n");
        return(-1);
    }
    else if(hasFilename == FALSE) {
        printf("The program needs the name of the file to send.\n");
        return(-1);
    }
    else if(hasPortname == FALSE) {
        printf("The program needs the serial port to use.\n");
        return(-1);
    }
    
    setFileBlockSize(app, fileblocksize);
    
    /* reading the file to be transmitted */
    functionResult = openFile(app, filename);
    
    if(functionResult == INEXISTENT_FILE) {
        printf("The file %s doesn't exist.\n", filename);
        free(app);
        return(-1);
    }
    else if (functionResult == FSEEK_ERROR) {
        printf("Error obtaining file size.\n");
        free(app);
        return(-1);
    }
    else if(functionResult == OK) {
        printf("\033[2J");
        printf("File %s opened with success.\n", app->filename);
        
        /* create the link layer */
        createNewLinkLayerOptions(portname, baudrate, maxRetransmissions, timeoutTime);
        
        char* str = calculateSize(app->originalFileSize);
        printf("\nSending file '%s' with size: %s\n", app->filename, str);
        
        functionResult = sendFile(app);
        
        str = calculateSize(LLayer->totalDataSent);
        printf("\nTotal data sent (including package and frame headers): %s\n", str);
        
        str = calculateSize(LLayer->totalDataSent - app->originalFileSize);
        printf("Extra data sent (not file data): %s\n", str);
        
        printf("Total packages sent: %lld of %lld\n", app->packetsSent, app->ctrlPkg.V_Pkg+2);
        printf("Number of timeout occurrences: %ld\n", LLayer->numTimeouts);
        printf("Number of received REJ responses: %lld\n", LLayer->numReceivedREJ);
        printf("Number of retransmitted frames: %lld\n", LLayer->numRetransmittedFrames);
        
        if(functionResult >= 0)
            printf("File %s sent with success.\n\n", app->filename);
        
        free(str);
    }
   
    free(app);
    return 0;
}
