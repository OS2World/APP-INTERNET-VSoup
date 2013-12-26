//  $Id: util.cc 1.13 1997/01/20 16:34:50 hardy Exp $
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


#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/nls.h>

#include "mts.hh"
#include "util.hh"



unsigned hashi( const char *src, unsigned tabSize )
{
    unsigned long res = 0;
    const char *p;
    const unsigned long maxbit  = 0x40000000;   // needs 32bits
    const unsigned long maxmask = maxbit - 1;

    for (p = src;  *p != '\0';  ++p) {
	res = (res << 1) ^ _nls_tolower(*p);
	if (res >= maxbit)
	    res = (res & maxmask) ^ 0x00000001;
    }
    return res % tabSize;
}   // hashi



int nhandles( int depth )
//
//  directly taken from EMs emx/test/nfiles.c
//  depth is required as a break condition
//
{
    int h, i;

    assert( _heapchk() == _HEAPOK );
#ifdef TRACE_ALL
    printfT( "nhandles(%d)\n",depth );
#endif

    h = open("nul", O_RDONLY);
    if (h < 0)
	return 0;
    i = (depth > 1) ? nhandles(depth-1) + 1 : 1;
    close (h);
    return i;
}   // nhandles



int isHeader( const char *buf, const char *header )
//
//  according to RFC1036:
//  Each header line consist of a keyword, a colon, a blank, and 
//  some additional information.
//
{
    size_t len = strlen(header);
    return strnicmp(buf, header, len) == 0  &&  buf[len] == ':'  &&
	(buf[len+1] == ' ' || buf[len+1] == '\t');
}   // isHeader



const char *getHeader( TFile &handle, const char *headerField )
//
//  get the value for the message header field (NULL, if not found)
//  the memory for the result is allocated from the heap!
//  handle is the file that contains header/message info
//  on return the file is positioned to its original position
//
{
    char buf[BUFSIZ];
    char *result;
    long offset;
    int headerFieldLen, n;

    /* Remember file position */
    offset = handle.tell();

    headerFieldLen = strlen(headerField);

    //
    //  Look through header
    //
    while (handle.fgets(buf,sizeof(buf),1)) {
	//
	//  end of header ?
	//
	if (buf[0] == '\0')
	    break;

	//
	//  is there a match with headerField
	//
	if (isHeader(buf,headerField)) {
	    //
	    //  yes -> allocate memory for result and copy info of headerField
	    //         to result
	    //
	    n = strlen(buf + headerFieldLen + 2);
	    result = (char *)xstrdup(buf+headerFieldLen+2);
	    handle.seek(offset, SEEK_SET);          //  reposition file
	    return result;
	}
    }       

    //
    //  Reposition file
    //
    handle.seek(offset, SEEK_SET);
    return NULL;
}   // getHeader



const char *extractAddress( const char *src )
//
//  Extract mail address from the string.
//  Return a pointer to buffer on the heap containing the address or
//  NULL on an error.
//
{
    char buf[BUFSIZ];
    char ch, *put;
    const char *get;
    char gotAddress;

    gotAddress = 0;
    put = buf;
    if ((get = strchr(src, '<')) != 0) {
	char ch = *++get;
	while (ch != '>' && ch != '\0') {
	    *put++ = ch;
	    ch = *++get;
	}
	gotAddress = 1;
    } else {
	get = src;
	ch = *get++;

	/* Skip leading whitespace. */
	while (ch != '\0' && isspace(ch))
	    ch = *get++;

	while (ch != '\0') {
	    if (isspace(ch)) {
		ch = *get++;

	    } else if (ch == '(') {
		/* Skip comment. */
		int nest = 1;
		while (nest > 0 && ch != '\0') {
		    ch = *get++;

		    if (ch == '(')
			++nest;
		    else if (ch == ')')
			--nest;
		}

		if (ch == ')') {
		    ch = *get++;
		}

	    } else if (ch == '"') {
		/* Copy quoted string. */
		do {
		    *put++ = ch;
		    ch = *get++;
		} while (ch != '"' && ch != '\0');

		if (ch == '"') {
		    *put++ = ch;
		    ch = *get++;
		}

	    } else {
		/* Copy address. */
		while (ch != '\0' && ch != '(' && !isspace(ch)) {
		    *put++ = ch;
		    ch = *get++;
		}
		gotAddress = 1;
	    }
	}
    }

    if (gotAddress) {
	*put = '\0';
	return xstrdup( buf );
    } else {
	return NULL;
    }
}   // extractAddress



char *findAddressSep( const char *src )
//
//  Search for ',' separating addresses.
//
{
    char ch, matchCh;

    ch = *src; 
    while (ch != '\0' && ch != ',') {
        if (ch == '"') {
            matchCh = '"';
        } else if (ch == '(') {
            matchCh = ')';
        } else if (ch == '<') {
            matchCh = '>';
        } else {
            matchCh = '\0';
        }

        if (matchCh) {
            do {
                ch = *(++src);
            } while (ch != '\0' && ch != matchCh);

            if (ch == '\0')
                break;
        }
        ch = *(++src);
    }

    return (char *)src;
}   // findAdressSep
