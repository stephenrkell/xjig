#ifndef _SYSTEM_H
#define _SYSTEM_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <assert.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <math.h>
#include <fcntl.h>
#include <string.h>

typedef unsigned char UINT8;
typedef signed char INT8;
typedef unsigned short UINT16;
typedef signed short INT16;
typedef unsigned long UINT32;
const UINT32 MAX_UINT32 = 0xffffffff;
typedef signed long INT32;

// *** All routines should use terminate() rather than exit(1) ***
void terminate();


extern "C" {

extern int socket (int __family, int __type, int __protocol);

/************** Signals ****************/

#ifdef SunOS
   #define SET_SIGNAL(signum, sighandler) signal(signum, sighandler)
   #define MAX_SIGNAL 31
   #define SIG_HANDLER int handler() {terminate(); return 0; }
#endif

#ifdef DEC
   #define SET_SIGNAL(signum, sighandler) signal(signum, sighandler)
   #define MAX_SIGNAL 31
   #define SIG_HANDLER void handler(int) {terminate(); }
//   #define SIG_HANDLER void handler() {terminate(); }
#endif

#ifdef HPUX
   #ifndef _INCLUDE__STDC__
      #define _INCLUDE__STDC__
   #endif

   #ifndef _PROTOTYPES
      #define _PROTOTYPES
   #endif

   #define SET_SIGNAL(signum, sighandler) signal(signum, sighandler)
   #define MAX_SIGNAL 32
   #define SIG_HANDLER void handler(int) {terminate(); }
#endif

#include <signal.h>

/************* Files *************/

#if defined(SunOS) || defined(DEC)
   #define FD_SET_CAST fd_set *
#endif

#ifdef HPUX
   #define FD_SET_CAST int *
#endif

#ifdef DEC
//   #define SET_RLIM_MAX max_file_num.rlim_max = 1000
   extern int getdtablesize();
   #define SET_RLIM_MAX max_file_num.rlim_max = getdtablesize()
#else
   #define SET_RLIM_MAX \
   if(getrlimit(RLIMIT_NOFILE, &max_file_num)) { \
      fprintf(stderr, "Error %i on getrlimit()\n", errno); \
      terminate(); \
   }
#endif

/************* Sockets ***********/

#if defined(SunOS) || defined(DEC)
extern int select (int __width, fd_set * __readfds,
		   fd_set * __writefds, fd_set * __exceptfds,
		   struct timeval * __timeout);
#endif

#if !defined(HPUX) && !defined(__linux__)
int setsockopt(int s, int leve, int optname, char *optval, int optlen);
extern int bind (int __sockfd, struct sockaddr *__my_addr,
		 int __addrlen);

extern int send(int s, UINT8 *msg, int len, int flags);
extern int recv(int s, char *buf, int len, int flags);
extern int connect(int s, struct sockaddr *name, int namelen);
extern int accept(int s, struct sockaddr *addr, int *addrlen);
#endif

extern int close(int fd);
extern int listen(int s, int backlog);

#if !defined(__linux__)
extern void bzero(char *b, int length);
extern void bcopy(char *b1, char *b2, int len);

extern int ftime(timeb *tp);

extern int getrlimit(int resource, struct rlimit *rlp);
extern int inet_addr(char *cp);
extern int gethostname(char *name, int namelen);
#else
#include <unistd.h>
#endif
};

#include <new>

#endif
