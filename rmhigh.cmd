/*  rmhigh.cmd
 *
 *  removes the highlighting stuff in VSoup.Txt
 *  invoke with 'rmhigh < vsoup.txt > vsoup.doc'
 */

IF chars() \= 0 THEN
    c2 = charin()
ELSE DO
    SAY 'need input file on stdin!'
    EXIT 1
END

DO WHILE chars() \= 0
    c1 = c2
    c2 = charin()
    IF c2 \= X2C('8') THEN
	rc = charout( ,c1 )
    ELSE
	c2 = charin()
END

EXIT 0
