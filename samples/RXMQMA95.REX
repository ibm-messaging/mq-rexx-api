/* REXX - Put & Get Messages on Queues                                 */
/* Scenario:                                                           */
/*  1. Create 2 Queues to play with                                    */
/*  2. Open both Queues                                                */
/*  3. Create persistent message                                       */
/*  4. Set MsgType = MQMT_REQUEST and request Confirmation of Delivery */
/*  5. Set ReplyToQ to 2nd Queue                                       */
/*  6. Put message to 1st Queue inside Unit-of-Work                    */
/*  7. Check number of messages to both Queues (should be 1:0)         */
/*  8. Try to browse 1st Queue, which fails as no COMMIT is done yet   */
/*  9. Perform COMMIT now                                              */
/* 10. Read message from 1st Queue, response goes to 2nd Queue         */
/* 11. Check number of messages to both Queues (should be 0:1)         */
/* 12. Read response message from 2nd Queue                            */
/* 13. Check that it is MQMT_REPORT message with MQFB_COD feedback     */
/* 14. Back out last changes to close UOW                              */
/* 15. Close and clean-up both Queues                                  */
/* 16. Show how to manipulate RXMQ return & reason codes               */
ARG Parm
PARSE VAR Parm QM trace .

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
say 'Connect to Queue Manager -' QM
say

RXMQTRACE = ''
rcc = RXMQCONN(QM)
 call sayrcc

Q1 = 'RXMQ.QUEUE'
Q2 = 'RXMQ.REPLYTO.QUEUE'

say
say "In order to manipulate 2 Queues, let's create them first"
say
say 'Create' Q1 'and' Q2
say

data.0 = 0
command = 'DEFINE QLOCAL('Q1')'
say 'Issue command <'command'>'
RXMQTRACE = trace
rcc = RXMQC(QM, command, 'data.' )
 call sayrcc

/* Display results */
say
if ( data.0 <> 0 ) then do i=1 to data.0
   say '<'data.i'>'
end
say

data.0 = 0
command = 'DEFINE QLOCAL('Q2')'
say 'Issue command <'command'>'
RXMQTRACE = trace
rcc = RXMQC(QM, command, 'data.' )
 call sayrcc

/* Display results */
say
if ( data.0 <> 0 ) then do i=1 to data.0
   say '<'data.i'>'
end

say
say 'Open Queue' Q1 'for all types of Access, plus Attr manipulation'
say

RXMQTRACE = trace
iod.ver= 3
iod.on = Q1
iod.ot = MQOT_Q
oo = mqoo_output+mqoo_inquire+mqoo_browse+MQOO_INPUT_SHARED
rcc = RXMQOPEN('iod.', oo, 'h1', 'ood.')
 call sayrcc

do w=1 to words(ood.zlist)
  wrd = word(ood.zlist,w)
  say left('OD.'wrd,15) '<'ood.wrd'>'
end

say
say 'Open Queue' Q2 'for Input Access, plus Attr manipulation'
say

RXMQTRACE = trace
oo        = mqoo_inquire+MQOO_INPUT_SHARED
iod.on    = Q2
rcc       = RXMQOPEN(iod., oo , 'h2', 'ood.' )

call sayrcc

do w=1 to words(ood.zlist)
  wrd = word(ood.zlist,w)
  say left('OD.'wrd,15) '<'ood.wrd'>'
end

d.1      = time() '0123456789'
d.0      = LENGTH(d.1)
say
say 'Write a Persistent Message, within UOW, to Queue' Q1
say 'Message contains <'d.1'>'
say 'Set MsgType = MQMT_REQUEST and request Confirmation of Delivery'
say 'Response will be sent to' Q2
say

RXMQTRACE = trace
imd.ENC  = MQENC_NATIVE
imd.PER  = MQPER_PERSISTENT
imd.MSG  = MQMT_REQUEST
imd.FBK  = MQFB_COD
imd.REP  = MQRO_COD_WITH_FULL_DATA
imd.RTOQ = Q2
imd.RTOQM = QM
ipmo.opt = MQPMO_SYNCPOINT
rcc      = RXMQPUT(h1,'d.','imd.','omd.','ipmo.','opmo.')
 call sayrcc

say
say 'Inquire upon the number of Messages now in Queue' Q1
say
RXMQTRACE = trace
atrin = MQIA_CURRENT_Q_DEPTH
atrou = ''
rcc   = RXMQINQ(h1, atrin, 'atrou' )
 call sayrcc
say
say 'Inquired MQIA_CURRENT_Q_DEPTH('atrin') = <'atrou'>'
say

say
say 'Inquire upon the number of Messages now in Queue' Q2
say
RXMQTRACE = trace
atrin = MQIA_CURRENT_Q_DEPTH
atrou = ''
rcc   = RXMQINQ(h2, atrin, 'atrou' )
 call sayrcc
say
say 'Inquired MQIA_CURRENT_Q_DEPTH('atrin') = <'atrou'>'
say
say Q1 'contains just put message;' Q2 'is yet empty'

say
say 'Browse all msgs on queue' Q1 'using the Extension function'
say
RXMQTRACE = trace
do forever
   g.0 = 200
   g.1 = ''
   rcc = RXMQBRWS(h1,'g.')
    call sayrcc
   if ( word(rcc,1) <> 0 ) then leave
   say 'Data received...'
   say '@@@@@@@@@@@@@@@' i 'data <'g.1'> length' g.0
   say
end
say
say 'Message is not yet available on' Q1 'as no COMMIT yet done'
say


say
say 'Synchpoint all accesses to the QM - COMMIT'
say

RXMQTRACE = trace
rcc = RXMQCMIT()
 call sayrcc

say
say 'Suppose a different application now retrieves the message'
say 'Read all msgs from queue' Q1
say

RXMQTRACE = trace
do forever
   g.0      = 200
   g.1      = ''
   igmo.opt = MQGMO_WAIT
   rcc = RXMQGET(h1,'g.','igmd.','ogmd.','igmo.','ogmo.')
    call sayrcc
   if ( word(rcc,1) <> 0 ) then leave
   say 'Data received...'
   say '...............' i 'data <'g.1'> length' g.0
   say 'ogmd.pd' ogmd.pd 'ogmo.rqn<'ogmo.rqn'>'
   say
end

say
say 'Inquire upon the number of Messages now in Queue' Q1
say
RXMQTRACE = trace
atrin = MQIA_CURRENT_Q_DEPTH
atrou = ''
rcc   = RXMQINQ(h1, atrin, 'atrou' )
 call sayrcc
say
say 'Inquired MQIA_CURRENT_Q_DEPTH('atrin') = <'atrou'>'
say

say
say 'Inquire upon the number of Messages now in Queue' Q2
say
RXMQTRACE = trace
atrin = MQIA_CURRENT_Q_DEPTH
atrou = ''
rcc   = RXMQINQ(h2, atrin, 'atrou' )
 call sayrcc
say
say 'Inquired MQIA_CURRENT_Q_DEPTH('atrin') = <'atrou'>'
say
say 'Initial message disappears from' Q1', but response appears on' Q2
say

say
say 'Read response from Queue' Q2
say

RXMQTRACE = trace
do forever
   g.0      = 200
   g.1      = ''
   igmo.opt = MQGMO_WAIT
   rcc = RXMQGET(h2,'g.','igmd.','ogmd.','igmo.','ogmo.')
    call sayrcc
   if ( word(rcc,1) <> 0 ) then leave
   say 'Data received...'
   say '...............' i 'data <'g.1'> length' g.0

   do w=1 to words(ogmd.zlist)
     wrd = word(ogmd.zlist,w)
     say left('MD.'wrd,15) '<'ogmd.wrd'>'
   end

   do w=1 to words(ogmo.zlist)
     wrd = word(ogmo.zlist,w)
     say left('GMO.'wrd,15) '<'ogmo.wrd'>'
   end
end
say 
say 'Note MsgType = 4 = MQMT_REPORT'
say 'Note Feedback = 260 = MQFB_COD'
say

say
say 'Rollback a Unit of Work (empty in this case)'
RXMQTRACE = trace
rcc = RXMQBACK()
 call sayrcc

say
say 'Close Queue' Q1 'and' Q2
say

RXMQTRACE = ''
rcc = RXMQCLOS(h1, mqco_none)
 call sayrcc

RXMQTRACE = ''
rcc = RXMQCLOS(h2, mqco_none)
 call sayrcc

say
say "Clean-up both Queues, they are not needed any more"
say

data.0 = 0
command = 'DELETE QLOCAL('Q1') PURGE'
say 'Issue command <'command'>'
RXMQTRACE = trace
rcc = RXMQC(QM, command, 'data.' )
 call sayrcc

/* Display results */
say
if ( data.0 <> 0 ) then do i=1 to data.0
   say '<'data.i'>'
end
say

data.0 = 0
command = 'DELETE QLOCAL('Q2') PURGE'
say 'Issue command <'command'>'
RXMQTRACE = trace
rcc = RXMQC(QM, command, 'data.' )
 call sayrcc

/* Display results */
say
if ( data.0 <> 0 ) then do i=1 to data.0
   say '<'data.i'>'
end

say
say 'Disconnect from the QM'
say
RXMQTRACE = ''
rcc = RXMQDISC()
 call sayrcc

say
say 'Show the last command used etc.'
say
say 'Last Message <'RXMQ.LASTMSG'>' 
say 'Last call 'RXMQ.LASTOP', which ended with',
    'RC('RXMQ.LASTRC') and MQCC('RXMQ.LASTCC') and MQRC('RXMQ.LASTAC')'

say
say 'Issue a Bad command to show effect of negative RC'
say
rcc = RXMQOPEN()
say 'Last Message <'RXMQ.LASTMSG'>'
say 'Last call 'RXMQ.LASTOP', which ended with',
    'RC('RXMQ.LASTRC') and MQCC('RXMQ.LASTCC') and MQRC('RXMQ.LASTAC')'

say
say 'Remove the Interface functions from the Rexx Workspace ...'
say

RXMQTRACE = ''
rcc = RXMQTERM()
 call sayrcc

say
say '... but leave the MQ_ constants around'
say

say 'MQPER_PERSISTENT' MQPER_PERSISTENT 'RC(2048) is' RXMQ.RCMAP.2048
exit

sayrcc:
say 'RCC =' rcc
 if RXMQ.LASTRC > 0 then do
   interpret 'lcc = RXMQ.CCMAP.'RXMQ.LASTCC
   interpret 'lac = RXMQ.RCMAP.'RXMQ.LASTAC
   say 'LASTCC =' lcc '; LASTAC =' lac
 end
say 'Elapsed time, sec =' time('R')
return
/* End of RXMQMA95 exec */
