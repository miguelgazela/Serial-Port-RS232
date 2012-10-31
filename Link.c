#include "Link.h"

int DEBUG_LINK = TRUE;

unsigned char SET_COMMAND[] = {FLAG, A, C_SET, (A ^ C_SET), FLAG};
unsigned char UA_COMMAND[] = {FLAG, A, C_UA, (A ^ C_UA), FLAG};
unsigned char DISC_COMMAND[] = {FLAG, A, C_DISC, (A ^ C_DISC), FLAG};

unsigned char UA_ACK_0[] = {FLAG, A, RR, (A ^ RR), FLAG};
unsigned char UA_ACK_1[] = {FLAG, A, RR | 0x20, (A ^ (RR | 0x20)), FLAG};
unsigned char UA_REJ_0[] = {FLAG, A, REJ, (A ^ REJ), FLAG};
unsigned char UA_REJ_1[] = {FLAG, A, REJ | 0x20, (A ^ (REJ | 0x20)), FLAG};

void prepareFrameToSend(unsigned char* buffer, int length);

dataFrame* frameToSend;

void createNewLinkLayer(char* portname) {
    
    LLayer = (linkLayer*) malloc (sizeof(linkLayer));
    
    if(LLayer == NULL) {
        printf("Error alocating memory for linkLayer.\n");
        exit(-1);
    }
    
    memcpy(LLayer->port, portname, sizeof(char)*20);
    
    LLayer->numMaxTransmissions = DEFAULT_MAX_ATTEMPTS;
    LLayer->baudrate = -1;
    LLayer->timeout = DEFAULT_TIMEOUT;
    LLayer->sequenceNumber = 0;
    LLayer->totalDataSent = 0;
    LLayer->numTimeouts = 0;
    LLayer->numReceivedREJ = 0;
    LLayer->numRetransmittedFrames = 0;
}

void createNewLinkLayerOptions(char* portname, int baudrate, unsigned int numMaxTransmissions, unsigned int timeout) {
    createNewLinkLayer(portname);
    
    LLayer->baudrate = baudrate;
    LLayer->numMaxTransmissions = numMaxTransmissions;
    LLayer->timeout = timeout;
    
    if(DEBUG_LINK) {
        printf("Max number of retransmissions: %d\n", LLayer->numMaxTransmissions);
        printf("Timeout time: %d\n", LLayer->timeout);
        // falta o do baudrate
    }
}


int changeBaudrate(int fd)
{
    struct serial_struct serial_port_info;
    int default_divisor, custom_divisor, actual_baudrate;
    
    //Obter configurações da porta de série
    if (ioctl(fd, TIOCGSERIAL, &serial_port_info) < 0)
    {
        perror("It's impossible to obtain serial port configs. Using default baudrate.\n");
        return -1;
    }
    
    default_divisor = serial_port_info.custom_divisor;
    custom_divisor = serial_port_info.baud_base/LLayer->baudrate;
    //actual_baudrate = serial_port_info.baud_base / serial_port_info.custom_divisor;
    actual_baudrate = serial_port_info.baud_base / custom_divisor;
    
    if(custom_divisor==0)
        custom_divisor=default_divisor;
    
    if (actual_baudrate < LLayer->baudrate * 0.95 || actual_baudrate > LLayer->baudrate * 1.05)
    {
        printf("It's impossible to define a satisfactory baudrate near the specified. Using default baudrate.\n");
        return -1;
    }
    
    serial_port_info.custom_divisor = custom_divisor;
    serial_port_info.flags &= ~ASYNC_SPD_MASK;
    serial_port_info.flags |= ASYNC_SPD_CUST;
	
    //Definir configurações da porta de série
    if (ioctl(fd, TIOCSSERIAL, &serial_port_info) < 0)
    {
        perror("It's impossible to obtain serial port configs. Using default baudrate.\n");
        return -1;
    }
    else
    {
        //B38400 permite utilizar baud rates non-standard
        cfsetispeed(&newtio, B38400);
        cfsetospeed(&newtio, B38400);
        return 0;
    }
}
 

int resetBaudrate(int fd)
{
    struct serial_struct serial_port_info;

    if (ioctl(fd, TIOCGSERIAL, &serial_port_info) < 0)
    {
        perror("It's impossible to obtain serial port configs. Failed to reset the baudrate to it's default value.\n");
        return -1;
    }

    serial_port_info.flags &= ~ASYNC_SPD_MASK;

    if (ioctl(fd, TIOCSSERIAL, &serial_port_info) < 0)
    {
        perror("Failed to reset the baudrate to it's default value.\n");
        return -1;
    }
	return 0;
}

void prepareFrameToSend(unsigned char* buffer, int length) {
    unsigned int bufferIterator,packageFieldIterator, bytesStuffed = 0, extraPackageFieldSize;
    unsigned char BCC1,BCC2;
    unsigned char tempBCC1[2], tempBCC2[2];
    
    BCC1 = (A ^ (LLayer->sequenceNumber << 1));
    
    /* STUFFING BCC1 */
    
    if(BCC1 == FLAG) {
    	tempBCC1[0] = ESC;
    	tempBCC1[1] = 0x5E;
    }
    else if (BCC1 == ESC) {
    	tempBCC1[0] = ESC;
    	tempBCC1[1] = 0x5D;
    }
    else {
    	tempBCC1[0] = BCC1;
    }
    
    /* STUFFING BCC2 */
    
    BCC2 = buffer[0];
    
    for(bufferIterator = 1; bufferIterator < length; bufferIterator++)
        BCC2 ^= buffer[bufferIterator];
    
    if(BCC2 == FLAG) {
    	tempBCC2[0] = ESC;
    	tempBCC2[1] = 0x5E;
    }
    else if (BCC2 == ESC) {
    	tempBCC2[0] = ESC;
    	tempBCC2[1] = 0x5D;
    }
    else {
    	tempBCC2[0] = BCC2;
    }
    
    /* calculate how many bytes will need to be stuffed */
    for(bufferIterator = 0; bufferIterator < length; bufferIterator++)
        if (buffer[bufferIterator] == FLAG || buffer[bufferIterator] == ESC)
            bytesStuffed++;
    
    extraPackageFieldSize = (length - 2) + bytesStuffed + 3; /* 3 for the BCC2[2] and the FLAG at the end*/
    
    if(DEBUG_LINK) {
        printf("Number of package field bytes stuffed: %d\n", bytesStuffed);
        printf("Extra package field size: %d bytes\n", extraPackageFieldSize);
    }
    
    frameToSend = (dataFrame*)malloc(sizeof(dataFrame) + extraPackageFieldSize);
    
    /* set frame header */
    frameToSend->F_BEG = FLAG;
    frameToSend->df_A = A;
    frameToSend->df_C = (LLayer->sequenceNumber << 1);
    frameToSend->extraPackageFieldSize = extraPackageFieldSize;
    memcpy(frameToSend->BCC1, tempBCC1, 2);
    
    memcpy(frameToSend->packageField, buffer, length);
    
    //STUFFING
    
    for(bufferIterator = 0, packageFieldIterator = 0; bufferIterator < length; bufferIterator++)
    {
    	if(buffer[bufferIterator] == FLAG)
    	{
    		frameToSend->packageField[packageFieldIterator] = ESC;
    		frameToSend->packageField[packageFieldIterator+1] = 0x5E;
    		packageFieldIterator += 2;
    	}
    	else if(buffer[bufferIterator] == ESC)
    	{
    		frameToSend->packageField[packageFieldIterator] = ESC;
    		frameToSend->packageField[packageFieldIterator+1] = 0x5D;
    		packageFieldIterator += 2;
    	}
    	else
    	{
    		frameToSend->packageField[packageFieldIterator] = buffer[bufferIterator];
    		packageFieldIterator++;
    	}
    }
    
    /* set frame tail */
    memcpy(&frameToSend->packageField[frameToSend->extraPackageFieldSize-1], tempBCC2, 2);
    frameToSend->packageField[frameToSend->extraPackageFieldSize+1] = FLAG;
    
    if(DEBUG_LINK) {
        printf("Total data frame size: %d bytes\n", (int)sizeof(dataFrame) + extraPackageFieldSize);
        printf("BCC2[0]: 0x%X BCC2[1]: 0x%X\n", frameToSend->packageField[frameToSend->extraPackageFieldSize-1], frameToSend->packageField[frameToSend->extraPackageFieldSize]);
        printf("FLAG at data frame tail: 0x%X\n\n",  frameToSend->packageField[frameToSend->extraPackageFieldSize+1]);
    }
}

int llopen() {
	int fd, counter = 0, attempts = DEFAULT_MAX_ATTEMPTS, remaining, selectResult, setAttempts, validUAresponse, validResponse;
	struct timeval Timeout;
	fd_set readfs;
	time_t initialTime = time(NULL);
	double remainingTime = LLayer->timeout;
    unsigned char UA_RESPONSE[5];
	
	/*
     Open serial port device for reading and writing and not as controlling tty
     because we don't want to get killed if linenoise sends CTRL-C.
     */
    
    if ((fd = open(LLayer->port, O_RDWR | O_NOCTTY )) < 0) {
		perror(LLayer->port);
		return -1;
	}
    
    if (tcgetattr(fd,&oldtio) == -1) { 
        perror("tcgetattr");
        return -1;
    }
    
    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = DEFAULT_BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = OPOST;
    
    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    
    /*
     VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
     leitura do(s) próximo(s) caracter(es)
     */
    
    //Se o baudrate não for o default
	if(LLayer->baudrate!=-1)
		changeBaudrate(fd);
    
    tcflush(fd, TCIOFLUSH);
    
    if (tcsetattr(fd,TCSANOW,&newtio) == -1) {
        perror("tcsetattr");
        return -1;
    }
    
    if(DEBUG_LINK)
        printf("Opening serial port %s\n", LLayer->port);
    
    do {
		setAttempts = LLayer->numMaxTransmissions;
		remaining = 5;
        
		while(remaining > 0 && setAttempts > 0)
		{
			remaining -= write(fd, &SET_COMMAND[5-remaining], remaining);
			setAttempts--;
		}
		
		if(remaining == 0) {
            if(DEBUG_LINK)
                printf("SET_COMMAND sent!\n");
        }
		else {
			printf("Can't send SET_COMMAND.\n");
			return -1;
		}
		
		newtio.c_oflag = 0;
		tcsetattr(fd,TCSANOW,&newtio);
		
		remaining = 5;
		
		while(TRUE)
		{
            if(DEBUG_LINK)
                printf("Waiting for UA RESPONSE\n");
            
			Timeout.tv_sec = (long int) remainingTime;
			Timeout.tv_usec = (remainingTime - Timeout.tv_sec) * 1000000;
			FD_SET(fd,&readfs);
			
			selectResult = select(fd+1, &readfs, NULL, NULL, &Timeout);
            
			remainingTime = LLayer->timeout - difftime(time(NULL),initialTime);
			
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
            if(DEBUG_LINK)
                printf("Port %s is open!\n", LLayer->port);
		}
		else
			attempts--;
        
	} while (validResponse == FALSE && attempts > 0);
	
	if (validResponse == FALSE)
	{
		printf("Can't get a valid UA_RESPONSE...\n");
		return -1;
	}
	return fd;
}

int llclose(int fd) {
	int counter = 0, attempts = LLayer->numMaxTransmissions, remaining, selectResult, DISCattempts, validDISCresponse, validResponse, UAattempts;
	struct timeval Timeout;
	fd_set readfs;
	time_t initialTime = time(NULL);
	double remainingTime = LLayer->timeout;
	unsigned char DISC_RESPONSE[5];
    
    if(DEBUG_LINK)
        printf("Closing serial port %s\n", LLayer->port);
	
    do {
        
		DISCattempts = LLayer->numMaxTransmissions;
		remaining = 5;
        
		while(remaining > 0 && DISCattempts > 0)
		{
			remaining -= write(fd, &DISC_COMMAND[5-remaining], remaining);
			DISCattempts--;
		}
		
		if(remaining == 0) {
            if(DEBUG_LINK)
                printf("DISC_COMMAND sent!\n");
        }
		else
		{
			printf("Can't send DISC_COMMAND.\n");
			resetBaudrate(fd);
			return -1;
		}
		
		newtio.c_oflag = 0;
		tcsetattr(fd,TCSANOW,&newtio);
		
		remaining = 5;
		
		while(TRUE)
		{
            if(DEBUG_LINK)
                printf("Waiting for DISC RESPONSE\n");
            
			Timeout.tv_sec = (long int) remainingTime;
			Timeout.tv_usec = (remainingTime - Timeout.tv_sec) * 1000000;
			FD_SET(fd,&readfs);
			
			selectResult = select(fd+1, &readfs, NULL, NULL, &Timeout);
            
			remainingTime = LLayer->timeout - difftime(time(NULL),initialTime);
			
			if(selectResult == 0) {
				printf("Timeout ocurred. Can't close connection\n");
				resetBaudrate(fd);
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
            if(DEBUG_LINK)
                printf("Port %s is closing.\n", LLayer->port);
			
			UAattempts = DEFAULT_MAX_ATTEMPTS;
            remaining = 5;
            
			newtio.c_oflag = OPOST;
			tcsetattr(fd,TCSANOW,&newtio);
			
			while(remaining > 0 && UAattempts > 0)
			{
				remaining -= write(fd, &UA_COMMAND[5-remaining], remaining);
				UAattempts--;
			}
			if(remaining==0) {
                if(DEBUG_LINK)
                    printf("UA_COMMAND sent!\n");
            }
			else
			{
				printf("Can't send UA_COMMAND.\n");
				resetBaudrate(fd);
				return -1;
			}
		}
		else
			attempts--;
        
	} while (validResponse == FALSE && attempts>0);
	
	if (validResponse == FALSE)
	{
		printf("Can't get a valid disconnection response.\n");
		resetBaudrate(fd);
		return -1;
	}
	
	resetBaudrate(fd);

	if (tcsetattr(fd,TCSANOW,&oldtio) == -1) {
        	perror("tcsetattr");
       	 	return -1;
    	}
    
    return close(fd);
}

int llwrite(int fd, unsigned char* applicationPackage, int length) {
    int answer_result, bytesWritten, attempts = LLayer->numMaxTransmissions, validAnswer = FALSE;
    unsigned char UA_ACK_RECEIVED[5], UA_ACK_EXPECTED[5], POSSIBLE_REJ[5];
        
    if(LLayer->sequenceNumber == 0) {
        memcpy(UA_ACK_EXPECTED, UA_ACK_1, 5); /* waits for the next frame */
        memcpy(POSSIBLE_REJ, UA_REJ_0, 5);
    }
	else {
        memcpy(UA_ACK_EXPECTED, UA_ACK_0, 5);
        memcpy(POSSIBLE_REJ, UA_REJ_1, 5);
    }
    
    prepareFrameToSend(applicationPackage, length);
    
    newtio.c_oflag = OPOST;
    tcsetattr(fd, TCSANOW, &newtio);
    
    while(validAnswer==FALSE && attempts>0)
	{
		//SENDING FRAME
        printf("Sending New Frame - Sequence Number: %d\n", LLayer->sequenceNumber);
        
		bytesWritten = write(fd, frameToSend, sizeof(dataFrame) + frameToSend->extraPackageFieldSize);
        
		if(bytesWritten < sizeof(dataFrame) + frameToSend->extraPackageFieldSize)
		{
			validAnswer=FALSE;
			attempts--;
			continue;
		}
        
		//CHECKING ANSWER
        
        printf("Reading Response\n");
		answer_result = llread(fd,UA_ACK_RECEIVED,5);

		if(answer_result == 5)
		{
		    if(memcmp(UA_ACK_RECEIVED, UA_ACK_EXPECTED, 5) == 0)
		        validAnswer = TRUE;
	     
		    else if(memcmp(UA_ACK_RECEIVED, POSSIBLE_REJ, 5) == 0) {
		        validAnswer = FALSE;
                
                if(DEBUG_LINK) {
                    if(attempts > 1)
                        printf("Received REJ response. Sending same frame again\n");
                    else
                        printf("Received REJ response. No more attempts of retransmission are allowed\n");
                }
		        
                LLayer->numReceivedREJ++;
            }
		    else {
                if(DEBUG_LINK) {
                    if(attempts > 1)
                        printf("Received invalid response. Sending same frame again\n");
                    else
                        printf("Received invalid response. No more attempts of retransmission are allowed\n");
                }
		        validAnswer = FALSE;
			}
		}
		else
		{
            if(DEBUG_LINK) {
                if(attempts > 1)
                    printf("Couldn't receive a possible response. Sending same frame again\n");
                else
                    printf("Couldn't receive a possible response. No more attempts of retransmission are allowed\n");
            }
			validAnswer=FALSE;
		}
        
		attempts--;
	}
    
    free(frameToSend);
    
	if(validAnswer)
	{
		LLayer->sequenceNumber = (LLayer->sequenceNumber + 1) % 2; // 0+1%2=1, 1+1%2=0
        LLayer->totalDataSent += bytesWritten;
		return length;
	}
	else
		return -1;
}

int llread(int fd, unsigned char* buffer, int length)
{
    int remaining = length, select_result;
    struct timeval Timeout;
    fd_set readfs;
    time_t tempo_ini = time(NULL);
    double tempo_restante = LLayer->timeout;
    
    newtio.c_oflag = 0;
    tcsetattr(fd, TCSANOW, &newtio);
    
    while(1)
    {
        Timeout.tv_sec = (long int) tempo_restante;
        Timeout.tv_usec = (tempo_restante - Timeout.tv_sec) * 1000000;
        FD_SET(fd,&readfs);
        
        select_result = select(fd+1, &readfs, NULL, NULL, &Timeout);
        
        tempo_restante = LLayer->timeout - difftime(time(NULL),tempo_ini);
        
        if(select_result == 0) {
            printf("Timeout ocurred\n");
            LLayer->numTimeouts++;
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
