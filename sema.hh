//  $Id: sema.hh 1.10 1997/01/20 16:30:10 hardy Exp $
//
//  This progam/module was written by Hardy Griech based on ideas and
//  pieces of code from Chin Huang (cthuang@io.org).  Bug reports should
//  be submitted to rgriech@ibm.net.
//
//  This file is part of soup++ for OS/2.  Soup++ including this file
//  is freeware.  There is no warranty of any kind implied.  The terms
//  of the GNU Gernal Public Licence are valid for this piece of software.
//


#ifndef __SEMA_HH__
#define __SEMA_HH__


#if defined(OS2)  &&  defined(__MT__)

#define INCL_DOSSEMAPHORES
#define INCL_DOSPROCESS
#define INCL_DOSERRORS
#include <os2.h>

class TSemaphor {
private:
    HMTX Sema;
public:
    TSemaphor( void )    { DosCreateMutexSem( (PSZ)NULL, &Sema, 0, FALSE ); }
    ~TSemaphor()         { DosCloseMutexSem( Sema ); }
    int  Request( long timeMs = SEM_INDEFINITE_WAIT ) { return (DosRequestMutexSem(Sema,timeMs) == NO_ERROR); }
    void Release( void ) { DosReleaseMutexSem( Sema ); }
};


class TEvSemaphor {
private:
    HEV Sema;
public:
    TEvSemaphor( void )                            { DosCreateEventSem( (PSZ)NULL, &Sema, 0, FALSE ); }
    ~TEvSemaphor()                                 { DosCloseEventSem( Sema ); }
    void Post( void )                              { DosPostEventSem( Sema ); }
    void Wait( long timeMs = SEM_INDEFINITE_WAIT,
	       int resetSema = 1 )                 { if (DosWaitEventSem(Sema,timeMs) == 0) { if (resetSema) Reset();} }
    void Reset( void )                             { unsigned long ul;  DosResetEventSem( Sema,&ul ); }
};

#define blockThread()    DosEnterCritSec()
#define unblockThread()  DosExitCritSec()

#else

class TSemaphor {
public:
    TSemaphor( void )    {}
    ~TSemaphor()         {}
    int  Request( long timeMs=0 ) { timeMs = 0; return 1; }
    void Release( void ) {}
};


class TEvSemaphor {
public:
    TEvSemaphor( void )      {}
    ~TEvSemaphor()           {}
    void Post( void )        {}
    void Wait( long timeMs=0 ) { timeMs = 0; }
    void Reset( void )       {}
};

#define blockThread()
#define unblockThread()

#endif


#endif  // __SEMA_HH__
