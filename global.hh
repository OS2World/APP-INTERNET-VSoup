//  $Id: global.hh 1.16 1997/04/20 19:09:24 hardy Exp $
//
//  This progam/module was written by Hardy Griech based on ideas and
//  pieces of code from Chin Huang (cthuang@io.org).  Bug reports should
//  be submitted to rgriech@swol.de.
//
//  This file is part of VSoup for OS/2.  VSoup including this file
//  is freeware.  There is no warranty of any kind implied.  The terms
//  of the GNU Gernal Public Licence are valid for this piece of software.
//


#ifndef __GLOBAL_HH__
#define __GLOBAL_HH__


#ifdef DEFGLOBALS
#define EXTERN
#define INIT(i) = i
#else
#define EXTERN extern
#define INIT(i)
#endif


#include "areas.hh"
#include "newsrc.hh"


//
//  defines for threads
//
#if defined(OS2)  &&  defined(__MT__)
#define MAXNNTPTHREADS 10                // bei mehr als 15 hat es zu wenig Files !?
#else
#define MAXNNTPTHREADS 1
#endif


#define TIMEOUT        (100L)            // timeout value for connecting, aborting etc.


//
//  global variables
//
#if MAXNNTPTHREADS < 4
#define DEFNNTPTHREADS MAXNNTPTHREADS
#else
#define DEFNNTPTHREADS 4
#endif

EXTERN int maxNntpThreads INIT(DEFNNTPTHREADS);

#ifdef DEFGLOBALS
   TAreasMail areas("AREAS","%07d");
   TNewsrc newsrc;
#else
   extern TAreasMail areas;
   extern TNewsrc newsrc;
#endif

EXTERN int doAbortProgram             INIT(0);


//
//  program options
//
EXTERN char doNewGroups               INIT(0);
EXTERN char doXref                    INIT(1);
EXTERN char *homeDir                  INIT( NULL );
EXTERN char killFile[FILENAME_MAX]    INIT("");
EXTERN char killFileOption            INIT(0);
EXTERN unsigned long maxBytes         INIT(0);
EXTERN char newsrcFile[FILENAME_MAX]  INIT("");
EXTERN char *progname                 INIT(NULL);
EXTERN char readOnly                  INIT(0);
EXTERN char forceMailDelete           INIT(0);


//
//  host/user/passwd storage for the different protocols
//
struct serverInfo {
    const char *host;
    const char *user;
    const char *passwd;
    int port;
};

#ifdef DEFGLOBALS
serverInfo smtpInfo = {NULL,NULL,NULL,-1};
serverInfo pop3Info = {NULL,NULL,NULL,-1};
serverInfo nntpInfo = {NULL,NULL,NULL,-1};
#else
extern serverInfo smtpInfo;
extern serverInfo pop3Info;
extern serverInfo nntpInfo;
#endif


//
//  filenames
//
#ifdef OS2
#define FN_NEWSTIME "newstime"
#else
#define FN_NEWSTIME ".newstime"
#endif
#define FN_REPLIES  "replies"


#endif   // __GLOBAL_HH__
