#include "Link.h"

unsigned char SET_COMMAND[] = {FLAG, A, C_SET, (A ^ C_SET), FLAG};
unsigned char UA_COMMAND[] = {FLAG, A, C_UA, (A ^ C_UA), FLAG};
unsigned char DISC_COMMAND[] = {FLAG, A, C_DISC, (A ^ C_DISC), FLAG};

unsigned char UA_ACK_0[] = {FLAG, A, RR, (A ^ RR), FLAG};
unsigned char UA_ACK_1[] = {FLAG, A, RR | 0x20, (A ^ (RR | 0x20)), FLAG};
unsigned char UA_REJ_0[] = {FLAG, A, REJ, (A ^ REJ), FLAG};
unsigned char UA_REJ_1[] = {FLAG, A, REJ | 0x20, (A ^ (REJ | 0x20)), FLAG};

void prepareFrameToSend(unsigned char* buffer, int length);

dataFrame frameToSend;

void createNewLinkLayer(char* portname) {
    /* alocate memory */
    LLayer = (linkLayer*) malloc (sizeof(linkLayer));
    
    if(LLayer == NULL) {
        printf("Error alocating memory for linkLayer.\n");
        exit(-1);
    }
    
    memcpy(LLayer->port, portname, sizeof(char)*20);
    
    LLayer->numTransmissions = MAX_ATTEMPTS;
    /*(*lLayerPtr).baudrate = BAUDRATE;*/ /*TODO*/
    LLayer->timeout = TIMEOUT;
    LLayer->sequenceNumber = 0;
}

void prepareFrameToSend(unsigned char* buffer, int length) {
    int bufferIterator,packageFieldIterator;
    unsigned char BCC1,BCC2;
    
    frameToSend.F_BEG = FLAG;
    frameToSend.F_END = FLAG;
    frameToSend.df_A = A;
    frameToSend.df_C = (LLayer->sequenceNumber << 1);
    
    BCC1 = (frameToSend.df_A ^ frameToSend.df_C);
    
    if(BCC1 == FLAG) { //STUFFING BCC1
    	frameToSend.BCC1[0] = ESC;
    	frameToSend.BCC1[1] = 0x5E;
    }
    else if (BCC1 == ESC) {
    	frameToSend.BCC1[0] = ESC;
    	frameToSend.BCC1[1] = 0x5D;
    }
    else {
    	frameToSend.BCC1[0] = BCC1;
    }
    
    BCC2 = frameToSend.packageField[0];
    for(bufferIterator = 1; bufferIterator < length; bufferIterator++)
        BCC2 ^= frameToSend.packageField[bufferIterator];
    
    if(BCC2 == FLAG) { //STUFFING BCC2
    	frameToSend.BCC2[0] = ESC;
    	frameToSend.BCC2[1] = 0x5E;
    }
    else if (BCC2 == ESC) {
    	frameToSend.BCC2[0] = ESC;
    	frameToSend.BCC2[1] = 0x5D;
    }
    else {
    	frameToSend.BCC2[0] = BCC2;
    }
    
    memcpy(frameToSend.packageField, buffer, length);
    
    //STUFFING
    
    /* TODO:
     * Se relevante, arranjar uma forma de não enviar sempre o dobro dos bytes por causa do stuffing,
     * indicando no cabeçalho o tamanho em bytes do pacote de dados após stuffing. Sabendo isso, depois
     * o emissor não precisaria de enviar sempre 2*MAX_SIZE_PACKAGE_FIELD e o receptor poderia alocar apenas o espaço necessário.
     * Talvez seja pouco importante para efeitos deste trabalho.
     * */
    
    for(bufferIterator = 0, packageFieldIterator = 0; bufferIterator < length; bufferIterator++)
    {
    	if(buffer[bufferIterator] == FLAG)
    	{
    		frameToSend.packageField[packageFieldIterator] = ESC;
    		frameToSend.packageField[packageFieldIterator+1] = 0x5E;
    		packageFieldIterator += 2;
    	}
    	else if(buffer[bufferIterator] == ESC)
    	{
    		frameToSend.packageField[packageFieldIterator] = ESC;
    		frameToSend.packageField[packageFieldIterator+1] = 0x5D;
    		packageFieldIterator += 2;
    	}
    	else
    	{
    		frameToSend.packageField[packageFieldIterator] = buffer[bufferIterator];
    		packageFieldIterator++;
    	}
    }
}

int llopen() {
	int fd, counter = 0, attempts = MAX_ATTEMPTS, remaining, selectResult, setAttempts, validUAresponse, validResponse;
	struct timeval Timeout;
	fd_set readfs;
	time_t initialTime = time(NULL);
	double remainingTime = TIMEOUT;
    unsigned char UA_RESPONSE[5];
	
	/*
     Open serial port device for reading and writing and not as controlling tty
     because we don't want to get killed if linenoise sends CTRL-C.
     */
    
    fd = open(LLayer->port, O_RDWR | O_NOCTTY );
    if (fd < 0) {
		perror(LLayer->port);
		return -1;
	}
    
    /*
    if (tcgetattr(fd,&oldtio) == -1) { 
        perror("tcgetattr");
        return -1;
    }
     */
    
    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = OPOST;
    
    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    
    /*
     VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
     leitura do(s) próximo(s) caracter(es)
     */
    tcflush(fd, TCIOFLUSH);
    
    /*
    if (tcsetattr(fd,TCSANOW,&newtio) == -1) {
        perror("tcsetattr");
        return -1;
    }
     */
    
    do {
		setAttempts = MAX_ATTEMPTS;
		remaining = 5;
        
		while(remaining > 0 && setAttempts > 0)
		{
			remaining -= write(fd, &SET_COMMAND[5-remaining], remaining);
			setAttempts--;
		}
		
		if(remaining == 0)
			printf("SET_COMMAND SENT...\n");
		else {
			printf("CAN'T SEND SET_COMMAND...\n");
			return -1;
		}
		
		newtio.c_oflag = 0;
		/*tcsetattr(fd,TCSANOW,&newtio);*/
		
		remaining = 5;
		
		while(TRUE)
		{
			Timeout.tv_sec = (long int) remainingTime;
			Timeout.tv_usec = (remainingTime - Timeout.tv_sec) * 1000000;
			FD_SET(fd,&readfs);
			
			selectResult = select(fd+1, &readfs, NULL, NULL, &Timeout);
            
			remainingTime = TIMEOUT - difftime(time(NULL),initialTime);
			
			if(selectResult == 0) {
				printf("Timeout ocurred. Can't establish connection\n");
				exit(-1);
			}
			
			if(FD_ISSET(fd, &readfs)) {
				remaining -= read(fd, &UA_RESPONSE[5-remaining], remaining);
				
				if(remaining == 0)
					break;
			}
		}
		
		validUAresponse = TRUE;
        validResponse = FALSE;
        
		counter = 0;
        
		while(counter < 5)
		{
            if(UA_RESPONSE[counter] != UA_COMMAND[counter])
            {
                validUAresponse = FALSE;
                break;
            }
            counter++;
		}
		
		if(validUAresponse == TRUE)
		{
			validResponse = TRUE;
			printf("Port %s is now open...\n", LLayer->port);
		}
		else
			attempts--;
        
	} while (validResponse == FALSE && attempts>0);
	
	if (validResponse == FALSE)
	{
		printf("Can't get a valid UA_RESPONSE...\n");
		return -1;
	}
	return fd;
}

int llclose(int fd) {
	int counter = 0, attempts = MAX_ATTEMPTS, remaining, selectResult, DISCattempts, validDISCresponse, validResponse, UAattempts;
	struct timeval Timeout;
	fd_set readfs;
	time_t initialTime = time(NULL);
	double remainingTime = TIMEOUT;
	unsigned char DISC_RESPONSE[5];
	
    do {
        
		DISCattempts = MAX_ATTEMPTS;
		remaining = 5;
        
		while(remaining > 0 && DISCattempts > 0)
		{
			remaining -= write(fd, &DISC_COMMAND[5-remaining], remaining);
			DISCattempts--;
		}
		
		if(remaining == 0)
			printf("DISC_COMMAND SENT...\n");
		else
		{
			printf("CAN'T SEND DISC_COMMAND...\n");
			return -1;
		}
		
		newtio.c_oflag = 0;
		tcsetattr(fd,TCSANOW,&newtio);
		
		remaining = 5;
		
		while(TRUE)
		{
			Timeout.tv_sec = (long int) remainingTime;
			Timeout.tv_usec = (remainingTime - Timeout.tv_sec) * 1000000;
			FD_SET(fd,&readfs);
			
			selectResult = select(fd+1, &readfs, NULL, NULL, &Timeout);
            
			remainingTime = TIMEOUT - difftime(time(NULL),initialTime);
			
			if(selectResult == 0) {
				printf("Timeout ocurred. Can't close connection\n");
				return(-1);
			}
			
			if(FD_ISSET(fd, &readfs)) {
				remaining -= read(fd, &DISC_RESPONSE[5-remaining], remaining);
				
				if(remaining == 0)
					break;
			}
		}
		
		validDISCresponse = TRUE;
		counter = 0;
        
		while(counter < 5)
		{
            if(DISC_RESPONSE[counter] != DISC_COMMAND[counter])
            {
                validDISCresponse = FALSE;
                break;
            }
            counter++;
		}
		
		if(validDISCresponse == TRUE)
		{
			validResponse = TRUE;
			printf("Port %s is now closing...\n", LLayer->port);
			
			UAattempts = MAX_ATTEMPTS;
            remaining = 5;
            
			newtio.c_oflag = OPOST;
			tcsetattr(fd,TCSANOW,&newtio);
			
			while(remaining > 0 && UAattempts > 0)
			{
				remaining -= write(fd, &UA_COMMAND[5-remaining], remaining);
				UAattempts--;
			}
			if(remaining==0)
				printf("UA_COMMAND SENT...\n");
			else
			{
				printf("CAN'T SEND UA_COMMAND...\n");
				return -1;
			}
		}
		else
			attempts--;
        
	} while (validResponse == FALSE && attempts>0);
	
	if (validResponse == FALSE)
	{
		printf("Can't get a valid disconnection response...\n");
		return -1;
	}
	
	if (tcsetattr(fd,TCSANOW,&oldtio) == -1) {
        perror("tcsetattr");
        return -1;
    }
    
    return close(fd);
}

int llwrite(int fd, unsigned char* buffer, int length) {
    int answer_result, counter, bytesWritten, attempts = MAX_ATTEMPTS, validAnswer = FALSE;
    unsigned char UA_ACK_RECEIVED[5], UA_ACK_EXPECTED[5];
    
    FILE* oFile = fopen("enviado", "ab"); /* TODO: remover */
    
    if(LLayer->sequenceNumber == 0)
        memcpy(UA_ACK_EXPECTED, UA_ACK_1, 5); /* waits for the next frame */
	else
        memcpy(UA_ACK_EXPECTED, UA_ACK_0, 5);
    
    prepareFrameToSend(buffer, length);
    
    newtio.c_oflag = OPOST;
    /*tcsetattr(fd, TCSANOW, &newtio);*/
    
    fwrite(&frameToSend, 1, sizeof(dataFrame), oFile); /* TODO: remover */
    fclose(oFile);
    
    printf("SIZE dataframe: %d\n", sizeof(dataFrame));
    
    return length;
}

int llread(int fd, unsigned char* buffer, int length)
{
    int remaining=length, select_result;
    struct timeval Timeout;
    fd_set readfs;
    time_t tempo_ini = time(NULL);
    double tempo_restante = TIMEOUT;
    
    newtio.c_oflag = 0;
    tcsetattr(fd, TCSANOW, &newtio);
    
    while(1)
    {
        Timeout.tv_sec = (long int) tempo_restante;
        Timeout.tv_usec = (tempo_restante - Timeout.tv_sec) * 1000000;
        FD_SET(fd,&readfs);
        
        select_result = select(fd+1, &readfs, NULL, NULL, &Timeout);
        
        tempo_restante = TIMEOUT - difftime(time(NULL),tempo_ini);
        
        if(select_result == 0) {
            printf("Timeout ocurred\n");
            break;
        }
        
        if(FD_ISSET(fd, &readfs)) {
            remaining -= read(fd, &buffer[length-remaining], remaining);
            
            if(remaining == 0)
                return length;
        }
    }
    
    if(remaining<length)
        return length-remaining;
    else
        return -1;	
}