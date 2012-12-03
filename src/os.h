/****************************************************************************
* AC Mud written by Michael "borlaK" Morrison and Jason "Pip" Wallace       *
*                   acmud@borlak.org              piptastic+acmud@gmail.com *
*                                                                           *
* Read ../doc/licence.txt for the terms of use.  One of the terms of use is *
* not to remove these headers.                                              *
****************************************************************************/

/*
operating system dependant stuff
*/
#if defined(WIN32)
#define vsnprintf	_vsnprintf
#define close(fd)	closesocket(fd)
#include <winsock2.h>
#include <sys/timeb.h>
#include <stdio.h>
#endif

#if !defined(WIN32)
#include <sys/time.h>
#include <pthread.h>
#include <netinet/in.h>
#endif

