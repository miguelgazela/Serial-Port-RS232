#include "Link.h"

linkLayer* createLinkLayer(char* portname) {
    /* alocate memory */
    linkLayer* lLayerPtr = (linkLayer*) malloc (sizeof(linkLayer));
    
    if(lLayerPtr == NULL) {
        printf("Error alocating memory for linkLayer.\n");
        exit(-1);
    }
    
    memcpy(lLayerPtr->port, portname, sizeof(char)*20);
    
    (*lLayerPtr).numTransmissions = MAX_ATTEMPTS;
    /*(*lLayerPtr).baudrate = BAUDRATE;*/ /*TODO*/
    (*lLayerPtr).timeout = TIMEOUT;
    (*lLayerPtr).sequenceNumber = 0;
    
    return lLayerPtr;
}