/* REXX - Inquire & Set Queue attributes                           */
/* Scenario:                                                       */
/* 1. Open specified Queue                                         */
/* 2. Inquire current setting of MQIA_INHIBIT_GET attribute        */
/* 3. Change this setting to MQQA_GET_INHIBITED                    */
/* 4. Inquire new setting and check that it changed                */
/* 5. Restore initial setting                                      */
ARG Parm
PARSE VAR Parm QM Q1 trace .

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

say
say 'Open Queue' Q1 'for Output and Browse Access, plus Attr manipulation'
say

RXMQTRACE = trace
oo = mqoo_inquire+mqoo_output+mqoo_browse+mqoo_set
rcc = RXMQOPEN(Q1, oo , 'h2', 'ood.' )
 call sayrcc

atrname = MQIA_INHIBIT_GET         /* Queue attribute to manipulate */

RXMQTRACE = trace
atrini = ''
rcc   = RXMQINQ(h2, atrname, 'atrini')
 call sayrcc
say
say 'Inquired MQIA_INHIBIT_GET('atrname') = <'atrini'>'
say

RXMQTRACE = trace
atrnew = MQQA_GET_INHIBITED
say
say 'Setting MQIA_INHIBIT_GET('atrname') = <'atrnew'>'
say
rcc   = RXMQSET(h2, atrname, atrnew)
 call sayrcc

RXMQTRACE = trace
atrchk = ''
rcc = RXMQINQ(h2, atrname, 'atrchk')
 call sayrcc
say
say 'Checking MQIA_INHIBIT_GET('atrname') = <'atrchk'>'
say

RXMQTRACE = trace
say
say 'Resetting to initial MQIA_INHIBIT_GET('atrname') = <'atrini'>'
say
rcc   = RXMQSET(h2, atrname, atrini)
 call sayrcc

RXMQTRACE = trace
atrchk = ''
rcc = RXMQINQ(h2, atrname, 'atrchk')
 call sayrcc
say
say 'Checking MQIA_INHIBIT_GET('atrname') = <'atrchk'>'
say


say
say 'Close Queue' Q1
say

RXMQTRACE = ''
rcc = RXMQCLOS(h2, mqco_none)
 call sayrcc

say
say 'Disconnect from the QM'
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
/* End of RXMQMA95 exec */
