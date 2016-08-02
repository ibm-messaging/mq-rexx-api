/* TESTPUB2 - a Rexx/MQ-pubsub Example */
/* REXX - Publish to a variable topic, no pre-defined objects      */
/* Scenario:                                                       */
/* 1. Open variable topic string as specified                      */
/* 2. Put a retained publication to topic string                   */
/* 3. Now manually run RXMQSUB2 to consume publication             */

ARG Parm
PARSE VAR Parm QM ' ' TS ' ' PB ' ' trace .

say
say 'Publisher to a variable topic, no predefined objects are necessary'
say

                                              /* Set defaults */
if QM == '' then QM = 'WMQW'                  /* QMgr name               */
if TS == '' then TS = 'SWG/RXMQ/VAR/PRICE'    /* Topic string            */
if PB == '' then PB = '$25000'                /* Publication             */
SQ = TN'QUE'                                  /* Subscription queue name */
SN = TN'SUB'                                  /* Subscription name       */

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

say
say 'Open variable topic string' TS
say

iod.ver= MQOD_VERSION_4       /* Descriptor needs to be V4             */
iod.ot = MQOT_TOPIC           /* Object is a topic                     */
iod.os.1 = TS                 /* Topic string                          */
iod.os.0 = LENGTH(TS)         /* Topic string length                   */
iod.ros.0 = 200               /* Resolved topic string max length      */
oo = MQOO_OUTPUT + MQOO_FAIL_IF_QUIESCING

RXMQTRACE = trace
rcc = RXMQOPEN('iod.', oo, 'h1', 'ood.')
 call sayrcc

   do w=1 to words(ood.zlist)
     wrd = word(ood.zlist,w)
     say left('OD.'wrd,15) '<'ood.wrd'>'
   end


say
say 'Put a retained publication to topic string' TS
say

d.1 = PB
d.0 = LENGTH(d.1)

ipmo.opt = MQPMO_FAIL_IF_QUIESCING + MQPMO_RETAIN

RXMQTRACE = trace
rcc      = RXMQPUT(h1,'d.','imd.','omd.','ipmo.','opmo.')
 call sayrcc

   do w=1 to words(omd.zlist)
     wrd = word(omd.zlist,w)
     say left('MD.'wrd,15) '<'omd.wrd'>'
   end
   do w=1 to words(opmo.zlist)
     wrd = word(opmo.zlist,w)
     say left('PMO.'wrd,15) '<'opmo.wrd'>'
   end

say
say 'Published "'PB'" to topic string "'ood.ros.1'"'
say 'Now run RXMQSUB2 sample to consume publication'
say
say 'Stop access to a topic string' TS
say

RXMQTRACE = ''
rcc = RXMQCLOS(h1, mqco_none)
 call sayrcc


say
say 'Disconnect from' QM
say

RXMQTRACE = ''
rcc = RXMQDISC()
 call sayrcc

say
say 'Remove the Interface functions from the Rexx Workspace ...'
say

RXMQTRACE = ''
rcc = RXMQTERM()
 call sayrcc

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
/* End of TESTPUB2 exec */
