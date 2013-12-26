/**/

if lines() = 0 then do
   say 'Modify os2emx.h / os2.h for multiple inclusion  -  rg160996'
   say ''
   say 'usage:  modifyemxh < input > output'
   say 'can be applied to os2.h & os2emx.h'
   exit( 3 )
end

trace('')

level       = 0
labelCnt    = 0
labelPrefix = '__HG'
globalIf    = 0
globalLab   = labelPrefix || 'global' || TIME('S')
localIf     = 0
do while lines() \= 0
   orgtext = linein()
   text = strip(orgtext)
IF 0 THEN DO 
   IF POS('RexxStart',text) \= 0 THEN DO  /****/
       SAY 'RexxStart'
       TRACE('i')
   END
end

   select

      when substr(text,1,3) = '#if' then do
         SELECT

            when level = 0 then
               rc = lineout( , '//' orgtext )    /* #if for multiple inclusion of header */

            when level = 1 then do
               if pos('INCL_',text) \= 0 then do
                  if globalIf then do
                     rc = lineout( , '#endif /*' globalLab '*/' )
                     rc = lineout( , '' )
                     globalIf = 0
                  end
                  curLabel = labelPrefix || labelCnt
                  labelCnt = labelCnt + 1
                  rc = lineout( , '' )
                  rc = lineout( , '#if !defined (' || curLabel || ')' )
                  rc = lineout( , orgtext )
                  rc = lineout( , '#define' curLabel )
                  localIf = 1
               END
	       ELSE DO
		   IF globalIf THEN DO
		       rc = lineout( , '#endif /*' globalLab '*/' )
		       rc = lineout( , '' )
		       globalIf = 0
		   END
		   rc = lineout( , '' )
		   rc = lineout( , orgtext );
		   localIf = 0
	       END
	    END

            otherwise
               rc = lineout( , orgtext )
         end
         level = level + 1
      end

      when substr(text,1,6) = '#endif' then do
         level=level - 1

         select
            when level = 0 then
               rc = lineout( , '//' text )

            when level = 1 then do
               rc = lineout( , orgtext )
               if localIf then
                  rc = lineout( , '#endif /*' curLabel '*/' )
            end

            when level < 0 then do
               say 'illegal nesting of #if/#endif in input file'
               exit( 3 )
            end

            otherwise
               rc = lineout( , orgtext )
         end
      END

      WHEN (level = 1)  &  (SUBSTR(text,1,7) = '#pragma') THEN DO
	  IF globalIf THEN DO
            rc = lineout( , '#endif /*' globalLab '*/' )
            rc = lineout( , '' )
            globalIf = 0
         end
         rc = lineout( , orgtext )
      END

      when (level = 1)  &  \globalIf  &  (orgtext \= '') then do
	  rc = lineout( , '' )
	  rc = lineout( , '#if !defined (' || globalLab || ')' )
	  rc = lineout( , orgtext )
	  globalIf = 1
      end

      otherwise
         rc = lineout( , orgtext )

   end
end
rc = lineout( , '#define' globalLab )

exit( 0 )
