/* REXX - Issue QMGR commands and interpret results                */
/* Scenario:                                                       */
/* 1. Issue command DISPLAY QMGR ALL & display results             */
/* 2. Pull out the name of the Command Queue                       */
/* 3. For z/OS only format and redisplay the results               */
/* 4. Issue command to display the attributes of the Command Queue */
/* 5. For z/OS only format and redisplay the results               */
/* 6. Request to display all queues, but do not show them          */
/* 7. Select only Persistent Transmission Queues from the output   */
/* 8. If no Transmission Queues are found, show all Queues         */
ARG Parm 
PARSE VAR Parm QMGR trace .

/* If not on TSO, add INIT REXX function */
env = address()
if env <> 'TSO' then
  rcc = RxFuncAdd('RXMQINIT','RXMQN','RXMQINIT')

/* Reset elapsed time clock */

t=time('R')

say
say 'Initialize the interface'
say

RXMQTRACE = ''
rcc= RXMQINIT()
 call sayrcc

say
say 'Display Queue Manager' QMGR 'details'
say

data.0 = 0
command = 'DISPLAY QMGR ALL'
RXMQTRACE = trace
rcc = RXMQC(QMGR, command, 'data.' )
 call sayrcc

/* Display QMGR data */
say
say 'Messages received' data.0
if ( data.0 <> 0 ) then do i=1 to data.0
   say i '<'data.i'>'
   if QU == 'QU' then parse var data.i . 'COMMANDQ(' QU ')' .
end

call cleaned

say
say 'Display the attributes of Command Queue' QU
say

data.0 = 0
command = 'DISPLAY QUEUE('QU') ALL '
RXMQTRACE = trace
rcc = RXMQC(QMGR, command, 'data.' )
 call sayrcc

/* Display the data */
say
say 'Messages received' data.0
say 'Start of native data...'
if ( data.0 <> 0 ) then do i=1 to data.0
   say i '<'data.i'>'
end
say 'End of native data '

call cleaned

say
say 'Request to display all queues, but do not show them'
say 'And select only Persistent Transmission Queues from the output'
say

data.0 = 0
allq.0 = 0
j = 0
RXMQTRACE = trace
rcc = RXMQC(QMGR, 'DISPLAY QUEUE(*) TYPE(QLOCAL) DEFPSIST USAGE', 'data.' )
 call sayrcc

if ( data.0 <> 0 ) then do i=1 to data.0
   parse var data.i . 'QUEUE(' allq.i ')' .
   allq.0 = allq.0 + 1
   parse var data.i . 'DEFPSIST(' dp ')' .
   parse var data.i . 'USAGE(' us ')' .
   if ( ( dp = 'YES') & ( us = 'XMITQ' ) ) then do
      j = j + 1
      gotq.j = allq.i
   end
   gotq.0 = j
end

say
if ( gotq.0 <> 0 ) then do
   say 'Selected Queues are'
   do i=1 to gotq.0
      say gotq.i
   end
say 'End of Selected Queues'
end
else do
   say 'No Transmission Queues found, all Queues are listed'
   do i=1 to allq.0
      say allq.i
   end
end
say

say 'Remove the Interface'
say
RXMQTRACE = ''
rcc = RXMQTERM()
 call sayrcc

/* End of Exec */
exit 0

cleaned:
if env == 'TSO' then do
   say
   say 'Now convert all the unprintables to blanks and remove those extra blanks'
   say
   say 'Start of cleaned data:'
   if ( data.0 <> 0 ) then do i=1 to data.0
      data.i = translate(data.i, ' ', xrange('00'x, '3f'x) )
      datastart = 1
      do forever
         line = ''
         startpos = pos("(",data.i,datastart)
         if startpos == 0 then leave
         do j=startpos-1 by -1 to datastart while(substr(data.i,j,1) <> ' ')
         end
         line  = line || substr(data.i,j+1,startpos-j)
         do j=startpos+1 by 1 while(substr(data.i,j,1) == ' ')
         end
         datastart = j
         endpos = pos(")",data.i,datastart)
         do j=endpos-1 by -1 to datastart while(substr(data.i,j,1) == ' ')
         end
         line = line || substr(data.i,datastart,j-datastart+1) || ')'
         datastart = endpos + 1
         say line
      end
   end
   say 'End of cleaned data'
end
return


sayrcc:
say 'RCC =' rcc
 if RXMQ.LASTRC > 0 then do
   interpret 'lcc = RXMQ.CCMAP.'RXMQ.LASTCC
   interpret 'lac = RXMQ.RCMAP.'RXMQ.LASTAC
   say 'LASTCC =' lcc '; LASTAC =' lac
 end
say 'Elapsed time, sec =' time('R')
return
