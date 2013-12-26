//  $Id: mts.cc 1.21 1997/02/12 08:49:45 hardy Exp $
//
//  This progam/module was written by Hardy Griech based on ideas and
//  pieces of code from Chin Huang (cthuang@io.org).  Bug reports should
//  be submitted to rgriech@ibm.net.
//
//  This file is part of soup++ for OS/2.  Soup++ including this file
//  is freeware.  There is no warranty of any kind implied.  The terms
//  of the GNU Gernal Public Licence are valid for this piece of software.
//
//  Problems
//  --------
//  -  the emx09c semaphores are likely to deadlock, because the signal handler
//     is illegal (streams!)...
//  -  sprintf/vsprintf are thread-safe functions and reentrant
//  -  sometimes the functions in this module are calling directly library functions.
//     Those functions are placed here to indicate that they are checked with respect
//     to thread-safety
//  -  all IO should be done without streams, see class TFile!  Although this class
//     seems to be not so nice as e.g. streams, it is doing the job quite well.  Be
//     cautious, if you want to use this class for other projects...
//     E.g. simultaneous access to one TFile is not possible.  If there is a gets()
//     active and the file will be closed at the same time, an access violation
//     will happen (most likely...).  Anyway this type of operation is undefined!
//


#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <io.h>
#include <share.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "mts.hh"
#include "sema.hh"



//--------------------------------------------------------------------------------



TSemaphor sysSema;   // damit werden einige Systemaufrufe serialisiert...



size_t fwriteT( const void *buffer, size_t size, size_t count, FILE *out )
{
    size_t res;

    sysSema.Request();
    res = fwrite( buffer,size,count,out );
    sysSema.Release();
    return res;
}   // fwriteT



FILE *popenT( const char *command, const char *mode )
{
    FILE *res;

#ifdef TRACE_ALL
    printfT( "popenT(%s,%s)\n", command,mode );
#endif

    sysSema.Request();
    res = popen( command,mode );
    sysSema.Release();
    return res;
}   // popenT



int pcloseT( FILE *io )
{
    int res;

#ifdef TRACE_ALL
    printfT( "pcloseT(%p)\n", io );
#endif

    sysSema.Request();
    res = pclose( io );
    sysSema.Release();
    return res;
}   // pcloseT



int removeT( const char *fname )
{
    int res;

#ifdef TRACE_ALL
    printfT( "removeT(%s)\n", fname );
#endif

    sysSema.Request();
    res = remove( fname );
    sysSema.Release();
    return res;
}   // removeT



int renameT( const char *oldname, const char *newname )
{
    int res;

#ifdef TRACE_ALL
    printfT( "renameT(%s,%s)\n",oldname,newname );
#endif

    sysSema.Request();
    res = rename( oldname,newname );
    sysSema.Release();
    return res;
}   // renameT



static char *tempnamT( const char *dir, const char *prefix )
{
    char *res;

#ifdef TRACE_ALL
    printfT( "tempnamT(%s,%s)\n",dir,prefix );
#endif

    sysSema.Request();
    res = tempnam( dir,prefix );
    sysSema.Release();
    return res;
}   // tempNamT



static int openT( const char *name, int oflag )
//
//  open file with mode oflag.
//  - shared write access is denied
//  - if file will be created, it will permit read/write access
//
{
    int res;

#ifdef TRACE_ALL
    printfT( "openT(%s,%d)\n", name,oflag );
#endif

    sysSema.Request();
    res = sopen( name, oflag, SH_DENYWR, S_IREAD | S_IWRITE );
    sysSema.Release();
    return res;
}   // openT



static int closeT( int handle )
{
    int res;

#ifdef TRACE_ALL
    printfT( "closeT(%d)\n", handle );
#endif

    sysSema.Request();
    res = close( handle );
    sysSema.Release();
    return res;
}   // closeT



static int readT( int handle, void *buffer, int len )
{
    int res;

#ifdef TRACE_ALL
    printfT( "readT(%d,%p,%d)\n",handle,buffer,len );
#endif

    sysSema.Request();
    res = read( handle,buffer,len );
    sysSema.Release();
    return res;
}   // readT



static int writeT( int handle, const void *buffer, int len )
{
    int res;

#ifdef TRACE_ALL
    printfT( "writeT(%d,%p,%d)\n",handle,buffer,len );
#endif

    sysSema.Request();
    res = write( handle,buffer,len );
    sysSema.Release();
    return res;
}   // writeT



static long lseekT( int handle, long offset, int origin )
{
    long res;

#ifdef TRACE_ALL
    printfT( "lseekT(%d,%ld,%d)\n",handle,offset,origin );
#endif

    sysSema.Request();
    res = lseek( handle,offset,origin );
    sysSema.Release();
    return res;
}   // lseekT



static int ftruncateT( int handle, long length )
{
    int res;

#ifdef TRACE_ALL
    printfT( "ftruncateT(%d,%ld)\n",handle,length );
#endif

    sysSema.Request();
    res = ftruncate( handle,length );
    sysSema.Release();
    return res;
}   // ftruncateT



//--------------------------------------------------------------------------------



int hprintfT( int handle, const char *fmt, ... )
{
    va_list ap;
    char buf[BUFSIZ];
    int  buflen;

    sysSema.Request();
    va_start( ap, fmt );
    buflen = vsprintf( buf, fmt, ap );
    va_end( ap );
    if (buflen > 0)
	buflen = write( handle,buf,buflen );
    sysSema.Release();
    return buflen;
}   // hprintfT



int hputsT( const char *s, int handle )
{
    int len, res;

    sysSema.Request();
    len = strlen(s);
    res = write( handle,s,len );
    sysSema.Release();
    return (res == len) ? len : EOF;
}   // hputsT



int sprintfT( char *dst, const char *fmt, ... )
{
    va_list ap;
    int  buflen;

    sysSema.Request();
    *dst = '\0';
    va_start( ap, fmt );
    buflen = vsprintf( dst, fmt, ap );
    va_end( ap );
    sysSema.Release();
    
    return buflen;
}   // sprintfT



int printfT( const char *fmt, ... )
{
    va_list ap;
    char buf[BUFSIZ];
    int  buflen;

    sysSema.Request();
    va_start( ap, fmt );
    buflen = vsprintf( buf, fmt, ap );
    va_end( ap );
    if (buflen > 0)
	write( STDOUT_FILENO, buf,buflen );
    sysSema.Release();
    return buflen;
}   // printfT



int vprintfT( const char *fmt, va_list arg_ptr )
{
    char buf[BUFSIZ];
    int buflen;

    sysSema.Request();
    buflen = vsprintf( buf,fmt,arg_ptr );
    if (buflen > 0)
	write( STDOUT_FILENO, buf,buflen );
    sysSema.Release();
    return buflen;
}   // vprintfT



int vsprintfT( char *dst, const char *fmt, va_list arg_ptr )
{
    int buflen;

    sysSema.Request();
    buflen = vsprintf( dst,fmt,arg_ptr );
    sysSema.Release();
    return buflen;
}   // vsprintfT



int sscanfT( const char *src, const char *fmt, ... )
{
    va_list ap;
    int fields;

    sysSema.Request();
    va_start( ap, fmt );
    fields = vsscanf( src, fmt, ap );
    va_end( ap );
    sysSema.Release();
    return fields;
}   // sscanfT



//--------------------------------------------------------------------------------



TFileTmp::TFileTmp( void )
{
#ifdef TRACE_ALL
    printfT( "TFileTmp::TFileTmp()\n" );
#endif
    memBuff     = NULL;
    memBuffSize = 0;
}   // TFileTmp::TFileTmp



TFileTmp::~TFileTmp()
{
#ifdef TRACE_ALL
    printfT( "TFileTmp::~TFileTmp()\n" );
#endif
    close();
}   // TFileTmp::~TFileTmp



void TFileTmp::close( void )
{
#ifdef TRACE_ALL
    printfT( "TFileTmp::close() %s\n",fname );
#endif
    TFile::close( 0,1 );
    if (memBuff != NULL) {
	memBuffSize = 0;
	delete memBuff;
	memBuff = NULL;
    }
}   // TFileTmp::close



int TFileTmp::open( const char *pattern, int buffSize )
//
//  ein tempor„rer File ist immer bin„r und wird immer erzeugt,
//  und ist read/write!
//
{
    char *tname;

#ifdef TRACE_ALL
    printfT( "TFileTmp::open(%s,%d)\n", pattern,buffSize );
#endif
    for (;;) {
	if ((tname = ::tempnamT(NULL,pattern)) == NULL)
	    return 0;
	filePos = 0;
	fileLen = 0;
	if (TFile::open(tname,TFile::mreadwrite, obinary,1)) {
	    free( tname );
	    break;
	}
#ifdef DEBUG_ALL
	printfT( "temp file %s rejected (already in use)\n",tname );
	perror( tname );
#endif
	free( tname );
    }
    memBuff = new unsigned char[buffSize];
    memBuffSize = buffSize;
    return 1;
}   // TFileTmp::open



int TFileTmp::iread( void *buff, int bufflen )
{
    int cbufflen;
    
#ifdef TRACE_ALL
    printfT( "TFileTmp::read(%p,%d) %s\n",buff,bufflen,fname );
#endif

    //
    //  compare remaining file data to requested size bufflen
    //
    if (bufflen >= 0  &&  filePos + bufflen > fileLen)
	bufflen = fileLen - filePos;
    cbufflen = bufflen;

    //
    //  read from memory part
    //
    if (filePos < memBuffSize  &&  cbufflen > 0) {
	int toRead = cbufflen;
	if (filePos + cbufflen > memBuffSize)
	    toRead = memBuffSize - filePos;
	memcpy( buff, memBuff+filePos, toRead );
	buff = (char *)buff + toRead;
	cbufflen -= toRead;
    }

    //
    //  read from physical file
    //
    if (cbufflen > 0)
	TFile::iread( buff,cbufflen );

    return bufflen;
}   // TFileTmp::iread



int TFileTmp::iwrite( const void *buff, int bufflen )
{
    int cbufflen = bufflen;
    
#ifdef TRACE_ALL
    printfT( "TFileTmp::iwrite(%p,%d) %s\n",buff,bufflen,fname );
#endif
    //
    //  write to memory part
    //
    if (filePos < memBuffSize  &&  cbufflen > 0) {
	int toWrite = cbufflen;
	if (filePos + cbufflen > memBuffSize)
	    toWrite = memBuffSize - filePos;
	memcpy( memBuff+filePos, buff, toWrite );
	buff = (char *)buff + toWrite;
	cbufflen -= toWrite;
    }

    //
    //  write to physical file
    //
    if (cbufflen > 0)
	TFile::iwrite( buff,cbufflen );

    return bufflen;
}   // TFileTmp::iwrite



int TFileTmp::truncate( long offset )
//
//  filePos is undefined after truncate()
//
{
#ifdef TRACE_ALL
    printfT( "TFileTmp::truncate(%ld)\n",offset );
#endif
    if (offset < fileLen)
	fileLen = offset;

    //
    //  set physical file
    //
    if (fileLen <= memBuffSize)
	TFile::truncate( 0 );
    else
	TFile::truncate( fileLen-memBuffSize );
    
    return 0;
}   // TFileTmp::truncate



long TFileTmp::seek( long offset, int origin )
{
#ifdef TRACE_ALL
    printfT( "TFileTmp::seek(%ld,%d) %s\n",offset,origin,fname );
#endif

    //
    //  calculate new file position
    //
    if (origin == SEEK_SET)
	filePos = offset;
    else if (origin == SEEK_CUR)
	filePos += offset;
    else if (origin == SEEK_END)
	filePos = fileLen + offset;
    if (filePos < 0)
	filePos = 0;
    else if (filePos > fileLen)
	filePos = fileLen;

    //
    //  set physical file pointer
    //
    {
	long tPos = filePos;
	
	if (filePos <= memBuffSize)
	    TFile::seek( 0, SEEK_SET );
	else
	    TFile::seek( filePos-memBuffSize, SEEK_SET );
	filePos = tPos;
    }
    
    return filePos;
}   // TFileTmp::seek



//--------------------------------------------------------------------------------



TFile::TFile( void )
{
#ifdef TRACE_ALL
    printfT( "TFile::TFile()\n" );
#endif
    mode  = mnotopen;
    fname = NULL;
    filePos = 0;
    fileLen = 0;
}   // TFile::TFile



TFile::~TFile()
{
#ifdef TRACE_ALL
    printfT( "TFile::~TFile()\n" );
#endif
    close();
}   // TFile::~TFile



void TFile::close( int killif0, int forceKill )
{
#ifdef TRACE_ALL
    printfT( "TFile::close(%d,%d) %s\n",killif0,forceKill,fname );
#endif

    if (mode != mnotopen) {
        int kill = (killif0  &&  (::tell(handle) == 0))  ||  forceKill;
	::closeT( handle );
	if (kill)
	    ::removeT( fname );
    }

    if (fname != NULL) {
	delete fname;
	fname = NULL;
    }
    if (mode == mread  ||  mode == mreadwrite) {
	delete buffer;
	buffer = NULL;
	buffSize = 0;
	invalidateBuffer();
    }
    mode   = mnotopen;
    handle = -1;
}   // TFile::close



void TFile::remove( void )
{
    close( 0,1 );
}   // TFile::remove



int TFile::open( const char *name, TMode mode, OMode textmode, int create )
//
//  in case of mwrite, the file pointer is positioned at end of file
//  returns 0, if not successful
//
{
    int flag;

#ifdef TRACE_ALL
    printfT( "TFile::open(%s,%d,%d,%d)\n",name,mode,textmode,create );
#endif

    if (TFile::mode != mnotopen)
	close(0);

    if (textmode == otext)
	flag = O_TEXT;
    else
	flag = O_BINARY;

    if (mode == mread)
	flag |= O_RDONLY;
    else if (mode == mwrite)
	flag |= O_WRONLY;
    else if (mode == mreadwrite)
	flag |= O_RDWR;
    else {
	errno = EPERM;
	return 0;
    }
    
    if (create)
	flag |= O_CREAT | O_TRUNC;

    handle = ::openT( name, flag );
    if (handle < 0)
	return 0;

    seek( 0,SEEK_END );           // set fileLen
    fileLen = filePos;
    if (mode != mwrite)
	seek( 0,SEEK_SET );       // sets also filePos
    
    TFile::mode = mode;
    xstrdup( &fname, name );
    buffSize = 0;
    buffer = NULL;
    if (mode == mread  ||  mode == mreadwrite) {
	buffSize = 4096;
	buffer = new unsigned char[buffSize+10];
    }
    invalidateBuffer();
    return 1;
}   // TFile::open



int TFile::truncate( long length )
{
    int res = ::ftruncateT( handle, length );
    seek( 0, SEEK_END );
    return res;
}   // TFile::truncate



long TFile::seek( long offset, int origin )
{
    long pos;

#ifdef TRACE_ALL
    printfT( "TFile::seek(%ld,%d)\n",offset,origin );
#endif
    invalidateBuffer();
    pos = ::lseekT( handle,offset,origin );
    if (pos >= 0)
	filePos = pos;
    return pos;
}   // TFile::seek



long TFile::tell( void )
{
    return filePos;
}   // TFile::tell



int TFile::write( const void *buff, int bufflen )
{
    int r;

    if (bufflen <= 0)
	return 0;

    //
    //  set the physical filePos correctly (fillBuff() problem)
    //
    if (buffNdx < buffEnd)
	seek( filePos,SEEK_SET );   // also invalidates Buffer
    
    r = iwrite( buff,bufflen );
    if (r > 0) {
	filePos += r;
	if (filePos > fileLen)
	    fileLen = filePos;
    }
    return r;
}   // TFile::write



int TFile::iwrite( const void *buff, int bufflen )
//
//  must not change filePos!
//
{
    return ::writeT( handle,buff,bufflen );
}   // TFile::iwrite



int TFile::putcc( char c )
{
    return (write(&c,1) == 1) ? 1 : EOF;
}   // TFile::putc



int TFile::fputs( const char *s )
{
    int len, res;

    len = strlen(s);
    res = write( s,len );
    return (res == len) ? len : EOF;
}   // TFile::fputs



int TFile::printf( const char *fmt, ... )
{
    va_list ap;
    char buf[BUFSIZ];
    int  buflen;

    va_start( ap, fmt );
    buflen = ::vsprintfT( buf, fmt, ap );
    va_end( ap );
    return write( buf,buflen );
}   // TFile::printf



int TFile::vprintf( const char *fmt, va_list arg_ptr )
{
    char buf[BUFSIZ];
    int buflen;

    buflen = ::vsprintfT( buf,fmt,arg_ptr );
    return write( buf,buflen );
}   // TFile::vprintf



int TFile::read( void *buff, int bufflen )
{
    int r;

    invalidateBuffer();
    r = iread( buff,bufflen );
    if (r > 0)
	filePos += r;

    assert( filePos <= fileLen );
    return r;
}   // TFile::read



int TFile::iread( void *buff, int bufflen )
//
//  must not change filePos!
//
{
    return ::readT( handle,buff,bufflen );
}   // TFile::iread



int TFile::fillBuff( void )
//
//  must not change filePos!
//
{
    int blk;
    int toRead;

#ifdef TRACE_ALL
    printfT( "TFile::fillBuff()\n" );
#endif

    if (buffEof)
	return buffNdx < buffEnd;

    if (buffEnd-buffNdx >= buffSize/2)
	return 1;

#ifdef TRACE_ALL
    printfT( "TFile::fillBuff() -- 2\n" );
#endif

    memmove( buffer, buffer+buffNdx, buffEnd-buffNdx );
    buffEnd = buffEnd - buffNdx;
    buffNdx = 0;
    toRead = (buffSize-buffEnd) & 0xfc00;
    blk = iread( buffer+buffEnd, toRead );
    if (blk > 0)
	buffEnd += blk;
    else
	buffEof = 1;            // -> nothing left to read!
    buffer[buffEnd] = '\0';

#ifdef DEBUG_ALL
    printfT( "TFile::fillBuff() = %d,%d,%d\n",blk,buffNdx,buffEnd );
#endif
    return buffEnd > 0;
}   // TFile::fillBuff



int TFile::getcc( void )
{
    if (buffNdx >= buffEnd) {
	if ( !fillBuff())
	    return -1;
    }
    ++filePos;
    return buffer[buffNdx++];
}   // TFile::getcc



char *TFile::fgets( char *buff, int bufflen, int skipCrLf )
//
//  '\0' will always be skipped!
//  If SkipCrLf, then also \r & \n are skipped
//
{
    char *p;
    int  n;
    int  r;

#ifdef TRACE_ALL
    printfT( "TFile::fgets(.,%d) %s\n",bufflen,fname );
#endif

    p = buff;
    n = 0;
    for (;;) {
	if (n >= bufflen-2) {     // if Buffer exhausted return (there is no \n at eob)
	    if (skipCrLf) {
		do
		    r = getcc();
		while (r > 0  &&  r != '\n');
	    }
	    break;
	}

	r = getcc();
	if (r < 0) {              // EOF!
	    if (n == 0) {
		*p = '\0';
		return( NULL );
	    }
	    else
		break;
	}
	else if (r != 0) {
	    if ( ! (skipCrLf  &&  (r == '\r'  ||  r == '\n'))) {
		*(p++) = (unsigned char)r;
		++n;
	    }
	    if (r == '\n')
		break;
	}
    }
    *p = '\0';
    return buff;
}   // TFile::fgets



int TFile::scanf( const char *fmt, void *a1 )
//
//  Scan one argument via sscanf.  Extension of this function to any number
//  of arguments does not work, because "%n" must be added (and an argument...)
//  Perhaps one argument is also ok, because what happened if one wants to
//  scan two arguments, but the second failed (what will "%n" display?)
//  -  sollen besondere Zeichen gescannt werden, so sollte das immer mit "%[...]"
//     gemacht werden, da ein einzelnes Zeichen, daá nicht paát die Zuweisung "%n"
//     verhindert!
//  trick:  the internal read buffer is always kept at least half full,
//          therefor scanf can work just like sscanf!
//
{
    int  fields;
    int  cnt;
    char fmt2[200];

#ifdef TRACE_ALL
    printfT( "TFile::scanf1(%s,...) %d,%d\n",fmt,buffNdx,buffEnd );
#endif

    if ( !fillBuff())
	return EOF;

    strcpy( fmt2,fmt );
    strcat( fmt2,"%n" );

    cnt = 0;
    fields = ::sscanfT( (char *)buffer+buffNdx, fmt2, a1,&cnt );
#ifdef DEBUG_ALL
    printfT( "TFile::scanf1()='%.20s' %d,%d\n",buffer+buffNdx,cnt,fields );
#endif
    if (cnt == 0)
	fields = 0;
    else {
	buffNdx += cnt;
	filePos += cnt;
	assert( filePos <= fileLen );
    }
    return fields;
}   // TFile::scanf



//--------------------------------------------------------------------------------



static TSemaphor regSema;



regexp *regcompT( const char *exp )
{
    regexp *res;

    regSema.Request();
    res = regcomp( exp );
    regSema.Release();
    return res;
}   // regcompT



int regexecT( const regexp *cexp, const char *target )
{
    int res;

    regSema.Request();
    res = regexec( cexp,target );
    regSema.Release();
    return res;
}   // regexecT



//--------------------------------------------------------------------------------



const char *xstrdup( const char *src )
{
    int len;
    char *dst;

    len = strlen(src)+1;
    dst = new char [len];
    if (dst != NULL)
	memcpy( dst,src,len );
    else {
	hprintfT( STDERR_FILENO,"\nxstrdup failed\nprogram aborted\n" );
	exit( 3 );
    }
    return dst;
}   // xstrdup



void xstrdup( const char **dst, const char *src )
{
    if (*dst != NULL)
	delete *dst;

    if (src != NULL)
	*dst = xstrdup( src );
    else
	*dst = NULL;
}   // xstrdup
