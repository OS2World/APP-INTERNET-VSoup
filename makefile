# $Id: makefile 1.25 1997/04/20 19:29:13 hardy Exp $

O = .o

CFLAGS = -DOS2 -DOS2EMX_PLAIN_CHAR -Wall -Zcrtdll -Zmtd -O1 -DNDEBUG
# -Zmtd
# -DWININITIALIZE
# -mepilogue -mprobe
# -DDEBUG -DTRACE_ALL -DHANDLEERR
# -DDEBUG_ALL -DTRACE_ALL -DTRACE -DDEBUG
# -DTRACE -DDEBUG
# -O99 -DNDEBUG

LDFLAGS = -s
LIBS    = -lregexp -lsocket -lstdcpp

.cc.o:
	$(CC) $(CFLAGS) -c $<

CC      = gcc
OBJECTS = areas$(O) kill$(O) main$(O) mts$(O) news$(O) newsrc$(O) nntpcl$(O) pop3$(O) \
                reply$(O) smtp$(O) socket$(O) util$(O)
PROGRAM = vsoup.exe
VERSION = vsoup128
MISC    = d:/b/32/yarnio.cmd d:/b/32/yarnio.set d:/b/32/modifyemxh.cmd \
          d:/b/32/loginisp.cmd d:/b/32/logoutisp.cmd \
          file_id.diz copying readme.1st rmhigh.cmd

$(PROGRAM): $(OBJECTS)
	$(CC) $(LDFLAGS) $(CFLAGS) -o $(PROGRAM) $(OBJECTS) $(LIBS)
#	copy $(PROGRAM) f:\yarn\bin

areas$(O):    areas.hh mts.hh

kill$(O):     kill.hh mts.hh nntp.hh

main$(O):     areas.hh global.hh mts.hh news.hh pop3.hh reply.hh socket.hh util.hh

mts$(O):      mts.hh sema.hh

news$(O):     areas.hh global.hh kill.hh mts.hh news.hh newsrc.hh nntp.hh nntpcl.hh \
              socket.hh

newsrc$(O):   mts.hh newsrc.hh util.hh

nntpcl$(O):   mts.hh nntp.hh nntpcl.hh socket.hh

pop3$(O):     areas.hh global.hh mts.hh pop3.hh socket.hh util.hh

reply$(O):    global.hh mts.hh nntpcl.hh reply.hh smtp.hh socket.hh util.hh

smtp$(O):     global.hh mts.hh smtp.hh socket.hh util.hh

socket$(O):   mts.hh socket.hh sema.hh

util$(O):     mts.hh util.hh sema.hh

test$(O):     global.hh socket.hh sema.hh util.hh



doc:    vsoup.inf

vsoup.inf: vsoup.src
	emxdoc -t -b1 -c -o vsoup.txt vsoup.src
	emxdoc -i -c -o vsoup.ipf vsoup.src
	ipfc /inf vsoup.ipf
	del vsoup.ipf

convsoup.exe: convsoup.cc
	$(CC) $(LDFLAGS)  $(CFLAGS) -o convsoup.exe convsoup.cc

ownsoup.exe:  ownsoup.cc
	$(CC) $(LDFLAGS)  $(CFLAGS) -o ownsoup.exe ownsoup.cc -lregexp

qsoup.exe:    qsoup.cc
	$(CC) $(LDFLAGS) $(CFLAGS) -o qsoup.exe qsoup.cc

zip:    $(PROGRAM) convsoup.exe ownsoup.exe qsoup.exe doc
	emxbind -s $(PROGRAM)
	-del $(VERSION).zip  > nul 2>&1
	zip -9j $(VERSION) *.cc *.hh *.ico makefile vsoup.src vsoup.txt vsoup.inf $(MISC) $(PROGRAM) convsoup.exe ownsoup.exe qsoup.exe
	uuencode $(VERSION).zip $(VERSION).uue

clean:
	-del *.o         > nul 2>&1
	-del *.obj       > nul 2>&1
	-del *.exe       > nul 2>&1
	-del *~          > nul 2>&1
	-del *.ipf       > nul 2>&1
