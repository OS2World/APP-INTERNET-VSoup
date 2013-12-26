//  $Id: mts.hh 1.17 1997/02/06 09:54:36 hardy Exp $
//
//  This progam/module was written by Hardy Griech based on ideas and
//  pieces of code from Chin Huang (cthuang@io.org).  Bug reports should
//  be submitted to rgriech@ibm.net.
//
//  This file is part of soup++ for OS/2.  Soup++ including this file
//  is freeware.  There is no warranty of any kind implied.  The terms
//  of the GNU Gernal Public Licence are valid for this piece of software.
//
//  multithreading save library functions
//


#ifndef __MTS_HH__
#define __MTS_HH__

#include <regexp.h>
#include <stdio.h>

#if defined(OS2)  &&  defined(__MT__)
#include <errno.h>
#define BEGINTHREAD(f,arg)  { while (_beginthread((f),NULL,100000,(arg)) == -1) { \
                                 if (errno != EINVAL) { \
				     perror("beginthread"); \
				     exit( EXIT_FAILURE ); }}}
#else
#define BEGINTHREAD(f,arg)  (f)(arg)
#endif

#include "sema.hh"        // all semaphore classes included !


class TFile {

public:
    enum TMode {mnotopen,mread,mwrite,mreadwrite};
    enum OMode {otext,obinary};

private:
    int handle;
    TMode mode;
    unsigned char *buffer;
    int buffSize;
    int buffEnd;
    int buffNdx;
    int buffEof;

protected:
    const char *fname;
    long filePos;
    long fileLen;
    
public:
    TFile( void );
    virtual ~TFile();

    virtual void close( int killif0=0, int forceKill=0 );
    void remove( void );
    int  open( const char *name, TMode mode, OMode textmode, int create=0 );
    int  write( const void *buff, int bufflen );
    int  putcc( char c );
    int  fputs( const char *s );
    int  printf( const char *fmt, ... ) __attribute__ ((format (printf, 2, 3)));
    int  vprintf( const char *fmt, va_list arg_ptr );
    int  read( void *buff, int bufflen );
    int  getcc( void );
    char *fgets( char *buff, int bufflen, int skipCrLf=0 );
    int  scanf( const char *fmt, void *a1 );
////    int  scanf( const char *fmt, ... ) __attribute__ ((format (scanf, 2, 3)));
    int  flush( void ) { return 1; };
    virtual int  truncate( long length );
    virtual long seek( long offset, int origin );
    virtual long tell( void );

    const char *getName( void ) { return fname; };
    int  isOpen( void ) { return mode != mnotopen; };

private:
    void invalidateBuffer( void ) { buffEof = buffNdx = buffEnd = 0; };
    int  fillBuff( void );

protected:
    virtual int iread( void *buff, int bufflen );
    virtual int iwrite( const void *buff, int bufflen );
};



class TFileTmp: public TFile {

private:
    int memBuffSize;
    unsigned char *memBuff;

public:
    TFileTmp( void );
    ~TFileTmp();

    void close( void );
    int  open( const char *pattern, int buffSize=40000 );
    int  read( void *buff, int bufflen );
    int  truncate( long offset );
    long seek( long offset, int origin );

private:
    virtual int iread( void *buff, int bufflen );
    virtual int iwrite( const void *buff, int bufflen );
};


int hprintfT( int handle, const char *fmt, ... ) __attribute__ ((format (printf, 2, 3)));
int hputsT( const char *s, int handle );
int sprintfT( char *dst, const char *fmt, ... ) __attribute__ ((format (printf, 2, 3)));
int vsprintfT( char *dst, const char *fmt, va_list arg_ptr );
int printfT( const char *fmt, ... ) __attribute__ ((format (printf, 1, 2)));
int vprintfT( const char *fmt, va_list arg_ptr );
int sscanfT( const char *src, const char *fmt, ... ) __attribute__ ((format (scanf, 2, 3)));

size_t fwriteT( const void *buffer, size_t size, size_t count, FILE *out );
FILE *popenT( const char *command, const char *mode );
int pcloseT( FILE *io );
int removeT( const char *fname );
int renameT( const char *oldname, const char *newname );

regexp *regcompT( const char *exp );
int regexecT( const regexp *cexp, const char *target );

const char *xstrdup( const char *src );
void xstrdup( const char **dst, const char *src );


//
//  multithreading save counter
//
class TProtCounter {
private:
    TProtCounter( const TProtCounter &right );   // not allowed !
    operator = ( const TProtCounter &right );    // not allowed !
public:
    TProtCounter( void ) { Cnt = 0; }
    ~TProtCounter() {}
    operator = ( const unsigned long &right ) { Cnt = right;  return *this; }

    TProtCounter & operator += (const unsigned long &offs) { blockThread();  Cnt += offs;  unblockThread();  return *this;  }
    TProtCounter & operator ++(void) { blockThread();  ++Cnt;  unblockThread();  return *this; }
    TProtCounter & operator --(void) { blockThread();  --Cnt;  unblockThread();  return *this; }
    operator unsigned long() { unsigned long res;  blockThread();  res = Cnt;  unblockThread();  return res; }
private:
    unsigned long Cnt;
};


#endif   // __MTS_HH__
