//  $Id: kill.cc 1.19 1997/04/20 19:10:01 hardy Exp $
//
//  This progam/module was written by Hardy Griech based on ideas and
//  pieces of code from Chin Huang (cthuang@io.org).  Bug reports should
//  be submitted to rgriech@swol.de.
//
//  This file is part of VSoup for OS/2.  VSoup including this file
//  is freeware.  There is no warranty of any kind implied.  The terms
//  of the GNU Gernal Public Licence are valid for this piece of software.
//
//  Kill file processing
//


#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <regexp.h>
#include <unistd.h>
#include <sys/nls.h>

#include "kill.hh"
#include "mts.hh"
#include "nntp.hh"



//--------------------------------------------------------------------------------


class MOLines: public MatchObj {    //// warum ist hier public notwendig?
private:
    int greater;
    unsigned long lines;
public:
    MOLines( int agreater, unsigned long alines ) {
#ifdef DEBUG
	printfT("MOLines(%d,%lu)\n",agreater,alines);
#endif
	greater = agreater;  lines = alines;
    }
    ~MOLines() {}
    doesMatch( const char *line ) {
	unsigned long actline;
	return (sscanfT(line,"lines: %lu",&actline) == 1  &&
		(( greater  &&  actline > lines)    ||
		 (!greater  &&  actline < lines)));
    }
};


class MOPatternAll: public MatchObj {
private:
    regexp *re;
public:
    MOPatternAll( const char *exp ) {
#ifdef DEBUG
	printfT( "MOPatternAll(%s)\n",exp );
#endif
	re = regcompT( exp );
    }
    ~MOPatternAll() { delete re; }
    doesMatch( const char *line ) { return regexecT( re,line ); }
};


class MOPattern: public MatchObj {
private:
    char *where;
    int wherelen;
    regexp *re;
public:
    MOPattern( const char *awhere, const char *exp ) {
#ifdef DEBUG
	printfT( "MOPattern(%s,%s)\n",awhere,exp );
#endif
	where = new char [strlen(awhere)+2];
	sprintfT( where,"%s:",awhere );
	wherelen = strlen( where );
	re = regcompT( exp );
    }
    ~MOPattern() { delete where;  delete re; };
    doesMatch( const char *line ) {
	return (strncmp(line,where,wherelen) == 0  &&
		regexecT( re,line+wherelen ));
    }
};


class MOStringAll: public MatchObj {
private:
    const char *s;
public:
    MOStringAll( const char *as ) {
#ifdef DEBUG
	printfT( "MOStringAll(%s)\n",as );
#endif
	s = xstrdup( as );
    }
    ~MOStringAll() { delete s; }
    doesMatch( const char *line ) { return strstr(line,s) != NULL; }
};


class MOString: public MatchObj {
private:
    char *where;
    int wherelen;
    const char *s;
public:
    MOString( const char *awhere, const char *as ) {
#ifdef DEBUG
	printfT( "MOString(%s,%s)\n",awhere,as );
#endif
	where = new char [strlen(awhere+2)];
	sprintfT( where,"%s:",awhere );
	wherelen = strlen(where);
	s = xstrdup( as );
    }
    ~MOString() { delete where;  delete s; }
    doesMatch( const char *line ) {
	return (strncmp(line,where,wherelen) == 0  &&
		strstr(line+wherelen,s) != NULL);
    }
};


//--------------------------------------------------------------------------------



TKillFile::TKillFile( void )
{
    groupKillList = actGroupList = NULL;
    actGroupName = xstrdup("");
    killThreshold = 0;
}   // TKillFile::TKillFile



void TKillFile::killGroup( Group *gp )
{
    Exp *ep1, *ep2;
    
    if (gp == NULL)
	return;

    ep1 = gp->expList;
    while (ep1 != NULL) {
	if (ep1->macho != NULL) {
	    delete ep1->macho;
	}
	ep2 = ep1->next;
	delete ep1;
	ep1 = ep2;
    }
    delete gp->grpPat;
    delete gp;
}   // TKillFile::killGroup



TKillFile::~TKillFile()
{
    Group *gp1, *gp2;

    gp1 = groupKillList;
    while (gp1 != NULL) {
	gp2 = gp1->next;
	killGroup( gp1 );
	gp1 = gp2;
    }

    gp1 = actGroupList;
    while (gp1 != NULL) {
	gp2 = gp1->next;
	delete gp1;
	gp1 = gp2;
    }
    delete actGroupName;
}   // TKillFile::~TKillFile



void TKillFile::stripBlanks( char *line )
{
    char *p1, *p2;
    int  len;

    p1 = line;
    while (*p1 == ' '  ||  *p1 == '\t')
	++p1;
    p2 = line + strlen(line) - 1;
    while (p2 >= p1  &&  (*p2 == ' '  ||  *p2 == '\t'))
	--p2;
    len = p2-p1+1;
    if (len > 0) {
	memmove( line,p1,len );
	line[len] = '\0';
    }
    else
	line[0] = '\0';
}   // TKillFile::stripBlanks



int TKillFile::readLine( char *line, int n, TFile &inf, int &lineNum )
//
//  fetch the next line from file
//  blanks are stripped
//  blank lines & lines with '#' in the beginning are skipped
//  on EOF NULL is returned
//    
{
    for (;;) {
	*line = '\0';
	if (inf.fgets(line,n,1) == NULL)
	    return 0;
	++lineNum;
	stripBlanks( line );
	if (line[0] != '\0'  &&  line[0] != '#')
	    break;
    }
    return 1;
}   // TKillFile::readLine



int TKillFile::readFile( const char *killFile )
//
//  Read kill file and compile regular expressions.
//  Return:  -1 -> file not found, 0 -> syntax error, 1 -> ok
//  Nicht so hanz das optimale:  besser w„re es eine Zustandsmaschine
//  zusammenzubasteln...
//
{
    char buf[1000], name[1000], tmp[1000];
    char searchIn[1000], searchFor[1000];
    TFile inf;
    Group *pGroup, *pLastGroup;
    Exp *pLastExp;
    char ok;
    int lineNum;

    groupKillList = NULL;

    if ( !inf.open(killFile,TFile::mread,TFile::otext))
	return -1;

    sema.Request();

    pLastGroup = NULL;
    ok = 1;

    //
    //  read newsgroup name
    //
    killThreshold = 0;
    lineNum = 0;
    while (ok  &&  readLine(buf,sizeof(buf),inf,lineNum)) {
#ifdef DEBUG_ALL
	printfT( "line: '%s'\n",buf );
#endif
	//
	//  check for "killthreshold"-line
	//
	{
	    long tmp;
	    
	    if (sscanfT(buf,"killthreshold %ld",&tmp) == 1) {
		killThreshold = tmp;
		continue;
	    }
	}
	
	//
	//  check for "quit"-line
	//
	if (strcmp(buf,"quit") == 0)
	    break;
	
	//
	//  check for: <killgroup> "{"
	//
	if (sscanfT(buf,"%s%s",name,tmp) == 1) {
	    if ( !readLine(tmp,sizeof(tmp),inf,lineNum)) {
		ok = 0;
		break;
	    }
	}
	if (tmp[0] != '{' || tmp[1] != '\0') {
	    ok = 0;
	    break;
	}

	//
	//  create Group-List entry and append it to groupKillList
	//
	_nls_strlwr( (unsigned char *)name );
	if (stricmp(name, "all") == 0)
	    strcpy( name,".*" );               // use 'special' pattern which matches all group names
	pGroup = new Group;
	pGroup->grpPat = regcompT( name );
	pGroup->expList = NULL;
	pGroup->next = NULL;

	if (pLastGroup == NULL)
	    groupKillList = pGroup;
	else
	    pLastGroup->next = pGroup;
	pLastGroup = pGroup;

	//
	//  Read kill expressions until closing brace.
	//
	pLastExp = NULL;
	while (readLine(buf,sizeof(buf),inf,lineNum)) {
	    long points;
	    unsigned long lineno;
	    MatchObj *macho = NULL;
	    Exp *pExp;

	    _nls_strlwr( (unsigned char *)buf );
#ifdef DEBUG
	    printfT( "TKillfile::readFile(): %s\n",buf );
#endif
	    if (buf[0] == '}'  &&  buf[1] == '\0')
		break;
	    
	    *searchIn = *searchFor = '\0';
	    if (sscanfT(buf,"%ld pattern header%*[:] %[^\n]", &points,searchFor) == 2) {
		macho = new MOPatternAll( searchFor );
	    }
	    else if (sscanfT(buf,"%ld pattern %[^ \t:]%*[:] %[^\n]", &points, searchIn, searchFor) == 3) {
		macho = new MOPattern( searchIn,searchFor );
	    }
	    else if (sscanfT(buf,"%ld header%*[:] %[^\n]", &points,searchFor) == 2) {
		macho = new MOStringAll( searchFor );
	    }
	    else if (sscanfT(buf,"%ld lines%*[:] %[<>] %lu", &points,searchFor,&lineno) == 3) {
		if (searchFor[1] != '\0') {
		    ok = 0;
		    break;
		}
		macho = new MOLines( *searchFor == '>', lineno );
	    }
	    else if (sscanfT(buf,"%ld %[^ \t:]%*[:] %[^\n]", &points,searchIn,searchFor) == 3) {
		macho = new MOString( searchIn,searchFor );
	    }
	    else {
		ok = 0;
		break;
	    }
	    
	    assert( macho != NULL );
	    
	    //
	    //  create entry and append it to expression list
	    //
	    pExp = new Exp;
	    pExp->points = points;
	    pExp->macho  = macho;
	    pExp->next   = NULL;
	    if (pLastExp == NULL)
		pGroup->expList = pExp;
	    else
		pLastExp->next = pExp;
	    pLastExp = pExp;
	}
    }
    sema.Release();

    inf.close();

    if ( !ok)
	hprintfT( STDERR_FILENO, "error in score file %s,\n\tsection %s, line %d\n",
		  killFile,name,lineNum);
    return ok;
}   // TKillFile::readFile



TKillFile::Group *TKillFile::buildActGroupList( const char *groupName )
//
//  return group kill for *groupName
//
{
    Group *p;
    Group **pp;
    char *name;

#ifdef TRACE_ALL
    printfT( "TKillFile::buildActGroupList(%s)\n",groupName );
#endif

    name = (char *)xstrdup( groupName );
    _nls_strlwr( (unsigned char *)name );

    if (stricmp(name,actGroupName) != 0) {
	pp = &actGroupList;
	for (p = groupKillList; p != NULL; p = p->next) {
	    //
	    //  is groupname matched by a killgroup regexp?
	    //
	    if (regexecT(p->grpPat,name)) {
		//
		//  does the killgroup regexp match the complete groupname?
		//
		if (name              == p->grpPat->startp[0]  &&
		    name+strlen(name) == p->grpPat->endp[0]) {
#ifdef DEBUG_ALL
		    printfT( "regexec: %p,%ld %p %p\n", name,strlen(name), p->grpPat->startp[0], p->grpPat->endp[0] );
#endif
		    if (*pp == NULL) {
			*pp = new Group;
			(*pp)->next = NULL;
		    }
		    (*pp)->expList = p->expList;
		    pp = &((*pp)->next);
		}
	    }
	}
	if (*pp != NULL)
	    (*pp)->expList = NULL;
	xstrdup( &actGroupName, name );
    }

    delete name;
    return actGroupList;
}   // TKillFile::buildActGroupList



long TKillFile::matchLine( const char *agroupName, const char *aline )
//
//  Check if line matches score criteria (line must be already in lower case!)
//  Return the sum of the matching scores.
//
{
    Group *pGroup;
    Exp   *pExp;
    char groupName[NNTP_STRLEN];
    char line[NNTP_STRLEN];
    long points;

    if (agroupName == NULL  ||  aline == NULL)
	return killThreshold;

    strcpy( groupName,agroupName );  _nls_strlwr( (unsigned char *)groupName );
    strcpy( line, aline );           _nls_strlwr( (unsigned char *)line );

    sema.Request();

    buildActGroupList( groupName );
    points = 0;
    for (pGroup = actGroupList;
	 pGroup != NULL  &&  pGroup->expList != NULL;
	 pGroup = pGroup->next) {
	for (pExp = pGroup->expList; pExp != NULL; pExp = pExp->next) {
	    if (pExp->macho->doesMatch(line))
		points += pExp->points;
	}
    }

    sema.Release();

    return points;
}   // TKillFile::matchLine



int TKillFile::doKillQ( const char *groupName )
{
    Group *p;
    
    sema.Request();
    p = buildActGroupList(groupName);
    sema.Release();
    return p != NULL  &&  p->expList != NULL;    // minimum one match
}   // doKillQ
