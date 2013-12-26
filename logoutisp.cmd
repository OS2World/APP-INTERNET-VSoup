/*
  LogoutIsp  -  rg040297

  $Id: logoutisp.cmd 1.5 1997/02/06 10:15:07 hardy Exp $

  Logoff from your ISP.

  The lines dialer/dialer2/dialerClose/comPort must be configured
*/

call RxFuncAdd 'SysLoadFuncs', 'RexxUtil', 'SysLoadFuncs'
call SysLoadFuncs

dialer            = 'c:\tcpip\bin\ppp.exe'
dialer2           = 'c:\tcpip\bin\dialer.exe'
dialerClose       = '@cmd /c "killname slattach ppp dialer"'
comPort           = 'com3'
startAfterConnect = '@echo connected'

TRACE('')

parse source . . compCmdName
cmdName = filespec('name',compCmdName)
tmpDir  = value('tmp',,'os2environment')

lockFile = ''
IF tmpDir = '' THEN
    SAY 'please define %TMP% for locking' CmdName
ELSE DO
    lockFile = tmpDir || '\logout.sem'
    if stream( lockFile,'c','open write') \= 'READY:' then DO
	SAY CmdName 'already active...'
	EXIT
    END
END

if isActive(dialer)  |  isActive(dialer2) then do
    SAY 'logging off...'
   '' dialerClose

   cnt = 0
   rc = SysSleep( 2 )
   DO FOREVER
       IF \(isActive(dialer)  |  isActive(dialer2)) THEN
	   LEAVE
       call HangupNow
       rc = SysSleep( 2 )
       cnt = cnt + 2
       IF cnt >= 60 THEN DO
	   '@start /min' dialerClose
	   cnt = 0
       END
   END
END
call HangupNow

IF lockFile \= '' THEN DO
    rc = stream( lockFile,'c','close' )
    rc = SysFileDelete( lockFile )
END

EXIT



HangupNow:
'@mode' comPort || ',dtr=on  > nul'
'@mode' comPort || ',dtr=off > nul'
'@mode' comPort || ',dtr=on  > nul'
RETURN



isActive: PROCEDURE
prog = ARG(1)

'@pstat /c | RXQUEUE'

prog = TRANSLATE(prog)
found = 0
DO ii = 1 TO queued()
    PULL line
    IF POS(prog,TRANSLATE(line)) \= 0 THEN do
	found = 1
	LEAVE
    END
END
RETURN found
