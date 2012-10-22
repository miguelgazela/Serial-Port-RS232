#include "Application.h"
#include "defs.h"

int main(int argc, char* argv[]) {
    int functionResult;
    
    if(argc != 3) {
        printf("USAGE: transmitter <portname> <filename>\n");
        exit(-1);
    }
    
    /* creating the application layer */
    applicationLayer* app = getNewApplicationLayer();
    setAs(app, TRANSMITTER);
    
    /* reading the file to be transmitted */
    functionResult = openFile(app, argv[2]);
    
    if(functionResult == INEXISTENT_FILE) {
        printf("The file %s doesn't exist!\n", argv[2]);
        exit(-1);
    }
    
    functionResult = loadFile(app);
    
    if(functionResult == OK) {
        printf("File %s loaded with success. File size: %ld bytes.\n", app->filename, app->originalFileSyze);
        
    }
    else if (functionResult == FSEEK_ERROR) {
        printf("Error obtaining file size.\n");
        exit(-1);
    }
    
    return 0;
}