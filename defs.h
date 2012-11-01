#ifndef DEFS_H
#define DEFS_H

/* GENERAL DEFINITIONS */

#define FALSE 0
#define TRUE 1

#define MAX_FILENAME 128
#define MAX_SIZE_DATAFIELD 65535
#define MIN_SIZE_DATAFIELD 128
#define REGULAR_SIZE_DATAFIELD 4096

#define OK 1

#define DIM(x) (sizeof(x)/sizeof(*(x)))

int DEBUG_APP = FALSE;
int DEBUG_LINK = FALSE;

#endif
