//  $Id: util.hh 1.13 1997/01/20 16:35:01 hardy Exp $
//
//  This progam/module was written by Hardy Griech based on ideas and
//  pieces of code from Chin Huang (cthuang@io.org).  Bug reports should
//  be submitted to rgriech@ibm.net.
//
//  This file is part of soup++ for OS/2.  Soup++ including this file
//  is freeware.  There is no warranty of any kind implied.  The terms
//  of the GNU Gernal Public Licence are valid for this piece of software.
//
//  NNTP client routines
//


#ifndef __UTIL_HH__
#define __UTIL_HH__


#include "mts.hh"


const char *getHeader( TFile &handle, const char *header );
unsigned hashi( const char *src, unsigned tabSize );
int nhandles( int depth=0 );
const char *extractAddress( const char *src );
char *findAddressSep( const char *src );
int isHeader( const char *buf, const char *header );


#endif   // __UTIL_HH__
