//  $Id: smtp.cc 1.22 1997/02/06 09:57:38 hardy Exp $
//
//  This progam/module was written by Hardy Griech based on ideas and
//  pieces of code from Chin Huang (cthuang@io.org).  Bug reports should
//  be submitted to rgriech@ibm.net.
//
//  This file is part of soup++ for OS/2.  Soup++ including this file
//  is freeware.  There is no warranty of any kind implied.  The terms
//  of the GNU Gernal Public Licence are valid for this piece of software.
//
//  Send mail reply packet using SMTP directly
//


#include <ctype.h>
#include <string.h>

#include "global.hh"
#include "mts.hh"
#include "smtp.hh"
#include "socket.hh"
#include "util.hh"



void smtpClose(TSocket &socket)
//
//  close SMTP connection
//
{
#ifdef DEBUG
    printfT( "smtpClose(): QUIT\n" );
#endif
    socket.printf("QUIT\n");
    socket.close();
}   // smtpClose



static int getSmtpReply( TSocket &socket, const char *response)
//
//  get a response from the SMTP server and test it
//  on correct response a '1' is returned    
//
{
    char buf[BUFSIZ];

    do {
	buf[3] = '\0';
	if (socket.gets(buf, BUFSIZ) == NULL) {
	    areas.mailPrintf1( 1, "Expecting SMTP %s reply, got nothing\n",
			       response, buf );
	    return 0;
	}
    } while (buf[3] == '-');		/* wait until not a continuation */

    if (strncmp(buf, response, 3) != 0) {
	areas.mailPrintf1(1, "Expecting SMTP %s reply, got %s\n", response, buf);
    }
    return (buf[0] == *response);	/* only first digit really matters */
}   // getSmtpReply



int smtpConnect ( TSocket &socket )
//
//  Open socket and intialize connection to SMTP server.
//  return value != 0  -->  ok
//
{
    const char *localhost;

    if (socket.open( smtpInfo.host,"smtp","tcp",smtpInfo.port ) < 0)
	return 0;

    if ( !getSmtpReply(socket, "220")) {
	areas.mailPrintf1( 1,"Disconnecting from %s\n", smtpInfo.host);
	smtpClose(socket);
	return 0;
    }

    localhost = socket.getLocalhost();
    socket.printf("HELO %s\n", localhost );
#ifdef DEBUG
    printfT( "localhost: %s\n",localhost );
#endif
    delete localhost;

    if ( !getSmtpReply(socket, "250")) {
	areas.mailPrintf1( 1,"Disconnecting from %s\n", smtpInfo.host);
	smtpClose(socket);
	return 0;
    }
    return 1;
}   // smtpConnect
    


static int sendSmtpRcpt( TSocket &socket, const char *buf )
//
//  Send RCPT command.
//
{
    areas.mailPrintf1(1,"%s: mailing to %s\n", progname, buf);
    socket.printf( "RCPT TO:<%s>\n", buf );
    return getSmtpReply(socket, "250");
}   // sendSmtpRcpt



static int putAddresses( TSocket &socket, char *addresses )
//
//  Send an RCPT command for each address in the address list.
//
{
    const char *srcEnd;
    char *startAddr;
    char *endAddr;
    char saveCh;
    const char *addr;
    
    srcEnd = strchr(addresses, '\0');
    startAddr = addresses;

    while (startAddr < srcEnd) {
	endAddr = findAddressSep(startAddr);
	saveCh = *endAddr;
	*endAddr = '\0';
	addr = extractAddress(startAddr);
	if (addr) {
	    if ( !sendSmtpRcpt(socket, addr)) {
		//// delete addr;
		return 0;
	    }
	    //// delete addr;
	}
	*endAddr = saveCh;
	startAddr = endAddr + 1;
    }
    return 1;
}   // putAddresses



int smtpMail( TSocket &socket, TFile &file, size_t bytes)
//
//  Send message to SMTP server.
//  To all recipients the same message will be sent, i.e. Bcc is not handled
//  in a special way!  sendmail handles it the same way...
//
{
    const char *addr;
    char buf[BUFSIZ];
    const char *from;
    char   *resentTo;
    long   offset;
    size_t count;
    int    sol;                 // start of line
    int    ll;                  // line length

    /* Look for From header and send MAIL command to SMTP server. */
    from = getHeader(file, "From");
    if (from == NULL || (addr = extractAddress(from)) == NULL) {
	areas.mailPrintf1( 1,"%s: no address in From header\n", progname );
	if (from != NULL)
	    delete from;
	return 0;
    }
    areas.mailPrintf1(1,"%s: mailing from %s\n", progname, addr);
    socket.printf("MAIL FROM:<%s>\n", addr);
    //// delete from;
    //// delete addr;
    if ( !getSmtpReply(socket, "250")) {
	return 0;
    }

    offset = file.tell();
    if ((resentTo = (char *)getHeader(file, "Resent-To")) != NULL) {
	//
	//  Send to address on Resent-To header
	//  Continuation is allowed
	//
	/* Send to address on Resent-To header. */
	if ( !putAddresses(socket, resentTo))
	    return 0;
	//// delete resentTo;

	while (file.fgets(buf,sizeof(buf),1) != NULL) {
	    if (buf[0] != ' ' && buf[0] != '\t')
		break;
	    if ( !putAddresses(socket, buf))
		return 0;
	}
    }
    else {
	//
	//  Send to addresses on To, Cc and Bcc headers.
	//
	int more = file.fgets(buf, sizeof(buf), 1) != NULL;
	while (more) {
	    if (buf[0] == '\0')
		break;

	    if (isHeader(buf, "To")  ||  isHeader(buf, "Cc")  ||  isHeader(buf, "Bcc")) {
		//
		//  first skip the To/Cc/Bcc field, then transmit the address
		//
		char *addrs;
		for (addrs = buf;  *addrs != '\0' && !isspace(*addrs);  ++addrs)
		    ;
		if ( !putAddresses(socket, addrs))
		    return 0;

		//
		//  Read next line and check if it is a continuation line.
		//
		while ((more = (file.fgets(buf,sizeof(buf),1) != NULL))) {
		    if (buf[0] != ' ' && buf[0] != '\t')
			break;
		    if ( !putAddresses(socket, buf))
			return 0;
		}
		
		continue;
	    }
	
	    more = file.fgets(buf, sizeof(buf), 1) != NULL;
	}
    }

    /* Send the DATA command and the mail message line by line. */
    socket.printf("DATA\n");
    if ( !getSmtpReply(socket, "354")) {
	return 0;
    }

    file.seek(offset, SEEK_SET);
    count    = bytes;
    sol      = 1;            // start of line
    while (file.fgets(buf, sizeof(buf)) != NULL  &&  count > 0) {
	//
	//  replace trailing "\r\n" with "\n"
	//
	ll  = strlen(buf);
	if (strcmp( buf+ll-2,"\r\n" ) == 0)
	    strcpy( buf+ll-2,"\n" );

	if (sol  &&  buf[0] == '.') {
	    //
	    //  is this a bug or a feature of SMTP?
	    //  the line "..\n" will be treated as EOA.
	    //  If there is a trailing blank, everything seems ok.
	    //
	    if (strcmp(buf,".\n") == 0)
		socket.printf( ".. \n" );
	    else
		socket.printf( ".%s",buf );
	}
	else
	    socket.printf( "%s",buf );
	sol = (buf[strlen(buf)-1] == '\n');
	count -= ll;
    }
    file.seek(offset+bytes, SEEK_SET);

    socket.printf(".\n");
    if ( !getSmtpReply(socket, "250"))
	return 0;

    return 1;
}   // smtpMail
