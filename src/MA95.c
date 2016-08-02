// MA95.C : WebSphere MQ Support Pac MA95 - REXX support for z/OS, Windows, AIX and Linux
//
//   (C) Copyright IBM Corporation 1997, 2012
//
// This is a nearly full implementation of the MQ API with these
//      omissions :
//
//      INQ and SET only take a single attribute.
//
//      MQCONNX not implemented
//
//      MQBEGIN not implemented
//
// The MQ API is extended in several ways:
//
//
//      * The QM Handle is not externally referenced, as this is held
//            within this program
//
//      * Structures that get updated are managed via seperate
//            input and output areas, so avoiding the need call
//            to do all that tedious rebuilding for each
//
//      * Output areas contain .ZLIST which contains a list of all the
//            components within the output stem (without the dot)
//
//      * A Browse function is provided which just returns the
//            message data
//
//      * A Header Extraction function will interprete Dead Letter
//           and Transmission Headers from suitable messages
//
//      * Expansion of Events into components (like the Header Extraction)
//
//      * Interpretation of Trigger Messages and Trigger Data
//
//      * Function to only set up MQ Constants
//
//
//   In order to use this Rexx/MQ Interface, initialization function
//        must be called before usage.
//
//   For Windows, AIX and Linux this is done by a RxFuncAdd('RXMQINIT','RXMQx','RXMQINIT')
//        call to let Rexx know about the DLL, and then a
//        rcc = RXMQINIT() to call the initialization function.
//        As part of this initialization function, all the other
//        RXMQ functions are registered with Rexx.
//
//   There is no equivalent termination function, as the MQ routines
//        attach to process termination to stop access at this
//        time, and a Termination operation would interfere
//        with this processing. However, a function called
//        RXMQTERM will deregister the interface functions WITHOUT
//        doing end-of-process actions.
//
//   All the RXMQ functions return a standard Rexx Return string.
//        This is structured so that the numeric Return Code
//        (which may be negative) is obtained by a word(RCC,1) call.
//
//   The Return Code for an operation can be negative to
//        show that this DLL has detected the error, otherwise
//        it will be the MQ Completion Code
//
//        The Return String is in text format as follows:
//
//            Word 1 : Return Code
//            Word 2 : MQ Completion Code (or 0 if MQ not done)
//            Word 2 : MQ Reason     Code (or 0 if MQ not done)
//            Word 4 : RXMQ... function being run
//            Word > : OK or an helpful error message
//
//   In addition, the current (ie: the settings last set) values are
//      available in these variables:
//
//   Windows, AIX, Linux:
//          RXMQN.LASTRC    -> current operation Return Code
//          RXMQN.LASTCC    -> current operation MQ Completion Code
//          RXMQN.LASTAC    -> current operation MQ Reason     Code
//          RXMQN.LASTOP    -> current operation RXMQ function
//          RXMQN.LASTMSG   -> current operation Return String
//          or
//          RXMQT.LASTRC    -> current operation Return Code
//          RXMQT.LASTCC    -> current operation MQ Completion Code
//          RXMQT.LASTAC    -> current operation MQ Reason     Code
//          RXMQT.LASTOP    -> current operation RXMQ function
//          RXMQT.LASTMSG   -> current operation Return String
//   MVS:
//          RXMQV.LASTRC    -> current operation Return Code
//          RXMQV.LASTCC    -> current operation MQ Completion Code
//          RXMQV.LASTAC    -> current operation MQ Reason     Code
//          RXMQV.LASTOP    -> current operation RXMQ function
//          RXMQV.LASTMSG   -> current operation Return String
//   Both:
//          RXMQ.LASTRC     -> current operation Return Code
//          RXMQ.LASTCC     -> current operation MQ Completion Code
//          RXMQ.LASTAC     -> current operation MQ Reason     Code
//          RXMQ.LASTOP     -> current operation RXMQ function
//          RXMQ.LASTMSG    -> current operation Return String
//
//   As a nice little bonus, there are also defined lots of variables
//        called RXMQ.RCMAP.nn , where nn is a MQAC number, whose
//        value is the name of the Reason Code (MQRC_ERROR_THING).
//        These variables can then be used in the Rexx Exec to get
//        the name from the Reason code via a
//                 interpret 'name = RXMQ.RCMAP.'word(rcc,1)
//        statement.
//
//        A TERM function leaves the MQ Rexx Variables known; if removed via
//        a DROP call, they may be restored via RXMQCONS (registering if
//        apt via RxFuncAdd('RXMQCONS',RXQMx','RXMQCONS') if needed).
//
//
//   Tracing MAY be enabled (if this module has been compiled with
//        the relevant setting of the TRACE macro) by setting
//        values into a Rexx Variable call RXMQTRACE. The settings are:
//
//                              CONN  -> mqconn
//                              DISC  -> mqdisc
//                              OPEN  -> mqopen
//                              CLOSE -> mqclose
//                              GET   -> mqget
//                              PUT   -> mqput
//                              PUT1  -> mqput1
//                              INQ   -> mqinq
//                              SET   -> mqset
//                              CMIT  -> mqcmit
//                              BACK  -> mqback
//                              SUB   -> mqsub
//
//                              BRO   -> Browse extension
//                              HXT   -> Header extraction extension
//                              EVENT -> Event determination extension
//                              TM    -> Trigger message extension
//                              COM   -> Command interface
//                              MQV   -> Debug a RXMQV
//
//                              INIT  -> initialization processing
//                              TERM  -> Deregistration processing
//
//                              *     -> Trace everything!!!
//
//   Support  : This program is not supported in any way by IBM.
//              It is distributed only to show techniques for
//              Message Queueing (if so distributed).
//              It may be freely modified and adapted by your
//              installation.
//
//
//

//
// Standard Header file includes
//
 #include <string.h>
 #include <stdlib.h>
 #include <stdio.h>
 #include <ctype.h>
#define __USE_MINGW_ANSI_STDIO 1
 #include <inttypes.h>
 #include <errno.h>

// Required for Win MINGW only
#define _int64 __int64

// For MVS FTYPE is required before rexx.h
#define FTYPE extern int APIENTRY

//
// Rexx Header file includes
//
#define INCL_RXSHV
#define INCL_RXFUNC
#include <rexx.h>

//
// WebSphere MQ Header file includes
//

#include <cmqc.h>
#include <cmqcfc.h>

#ifdef __MVS__
//
// REXX Functions for MVS
// And redefine entry points so that externalized entries are in assembler stub
//
  #define RXMQINIT CPPMINIT
  #define RXMQTERM CPPMTERM
  #define RXMQCONS CPPMCONS
  #define RXMQCONN CPPMCONN
  #define RXMQOPEN CPPMOPEN
  #define RXMQCLOS CPPMCLOS
  #define RXMQDISC CPPMDISC
  #define RXMQCMIT CPPMCMIT
  #define RXMQBACK CPPMBACK
  #define RXMQPUT  CPPMPUT
  #define RXMQPUT1 CPPMPUT1
  #define RXMQGET  CPPMGET
  #define RXMQINQ  CPPMINQ
  #define RXMQSET  CPPMSET
  #define RXMQSUB  CPPMSUB
  #define RXMQBRWS CPPMBRWS
  #define RXMQHXT  CPPMHXT
  #define RXMQEVNT CPPMEVNT
  #define RXMQTM   CPPMTM
  #define RXMQC    CPPMC
  #define RXMQV    CPPMV
  #define RXMQVC   CPPMVC
//
// Function to print to stdout (REXX terminal or standard output)
//
#define PRINTF(s) RXSAY(s, strlen(s));
#else
#define PRINTF(s) printf(s); \
                  fflush(NULL) ;  // Required to flush buffer
#endif

#define RXMQPARM ( PSZ       afuncname /*Rexx Func name       */\
                 , MQLONG    aargc     /*     Number of parms */\
                 , PRXSTRING aargv     /*     Parms           */\
                 , PSZ       aqname    /*     Cur Queue       */\
                 , PRXSTRING aretstr   /*     Return String   */\
                 )

//
// Global definitions
//
//  MINQS     -> Start scan of Queues allowed to process (not 0!)
//  MAXQS     -> maximum number of Queues allowed to process
//
 #define MINQS 1
 #define MAXQS 100
 #define MAXCOMMLEN 5000

 #define RXMQANCHOR "RXMQANCHOR"
 #define RXMQeyecatcher "RXMQ"
 typedef struct _RXMQCB {
     MQCHAR4    StrucId                      ; // Structure identifier
     MQULONG    tracebits                    ; // Variable to contain current trace status
     char       QMname[MQ_Q_MGR_NAME_LENGTH] ; // QM name
     MQHCONN    QMh                          ; // Connection handle
     MQHOBJ     Qh[MAXQS]                    ; // Queue handle
 } RXMQCB;

//
// Trace/Return variables
//
#ifdef __MVS__
  #define PREFIX   "RXMQV."
  #define TRACEVAR "RXMQVTRACE"
  #define RCMAP    "RXMQV.RCMAP.%lu"
#endif

#ifdef _RXMQN
  #define PREFIX   "RXMQN."
  #define TRACEVAR "RXMQNTRACE"
  #define RCMAP    "RXMQN.RCMAP.%lu"
  #define THISDLL  "RXMQN"
 //
 // This is a table of functions to be registered with Rexx
 //
  static PSZ funcs[] = {"RXMQV"       ,
                            "RXMQCONN"    ,  "RXMQNCONN"   ,
                            "RXMQDISC"    ,  "RXMQNDISC"   ,
                            "RXMQOPEN"    ,  "RXMQNOPEN"   ,
                            "RXMQCLOS"    ,  "RXMQNCLOSE"  ,
                            "RXMQGET"     ,  "RXMQNGET"    ,
                            "RXMQPUT"     ,  "RXMQNPUT"    ,
                            "RXMQINQ"     ,  "RXMQNINQ"    ,
                            "RXMQSET"     ,  "RXMQNSET"    ,
                            "RXMQSUB"     ,  "RXMQNSUB"    ,
                            "RXMQCMIT"    ,  "RXMQNCMIT"   ,
                            "RXMQBACK"    ,  "RXMQNBACK"   ,
                            "RXMQBRWS"    ,  "RXMQNBROWSE" ,
                            "RXMQHXT"     ,  "RXMQNHXT"    ,
                            "RXMQEVNT"    ,  "RXMQNEVENT"  ,
                            "RXMQTM"      ,  "RXMQNTM"     ,
                            "RXMQPUT1"    ,  "RXMQNPUT1"   ,
                            "RXMQC"       ,  "RXMQNC"      ,
                            "RXMQCONS"    ,  "RXMQNCONS"   ,
                            "RXMQTERM"    ,  "RXMQNTERM"
              } ;
 static MQULONG numfuncs = sizeof(funcs)/sizeof(char *) ;
#endif

#ifdef _RXMQT
  #define PREFIX   "RXMQT."
  #define RCMAP    "RXMQT.RCMAP.%lu"
  #define TRACEVAR "RXMQTTRACE"
  #define THISDLL  "RXMQT"
//
// This is a table of functions to be registered with Rexx
//
 static PSZ funcs[] = {"RXMQV"       ,
                           "RXMQCONN"    ,  "RXMQTCONN"   ,
                           "RXMQDISC"    ,  "RXMQTDISC"   ,
                           "RXMQOPEN"    ,  "RXMQTOPEN"   ,
                           "RXMQCLOS"    ,  "RXMQTCLOSE"  ,
                           "RXMQGET"     ,  "RXMQTGET"    ,
                           "RXMQPUT"     ,  "RXMQTPUT"    ,
                           "RXMQINQ"     ,  "RXMQTINQ"    ,
                           "RXMQSET"     ,  "RXMQTSET"    ,
                           "RXMQSUB"     ,  "RXMQTSUB"    ,
                           "RXMQCMIT"    ,  "RXMQTCMIT"   ,
                           "RXMQBACK"    ,  "RXMQTBACK"   ,
                           "RXMQBRWS"    ,  "RXMQTBROWSE" ,
                           "RXMQHXT"     ,  "RXMQTHXT"    ,
                           "RXMQEVNT"    ,  "RXMQTEVENT"  ,
                           "RXMQTM"      ,  "RXMQTTM"     ,
                           "RXMQPUT1"    ,  "RXMQTPUT1"   ,
                           "RXMQC"       ,  "RXMQTC"      ,
                           "RXMQCONS"    ,  "RXMQTCONS"   ,
                           "RXMQTERM"    ,  "RXMQTTERM"
                          } ;
 static MQULONG numfuncs = sizeof(funcs)/sizeof(char *) ;
#endif

//
// Define type for output messages struct
//
 typedef struct _RETMSG
                {
                 int    retcode  ;  // Internal (minus) retcode
                 char * retmsgc  ;  // Text part of message
                } RETMSG, *PRETMSG         ;

//
// Define global structure defaults
//
 MQOD    od_default    = {MQOD_DEFAULT}   ;
 MQSD    sd_default    = {MQSD_DEFAULT}   ;
 MQMD2   md_default    = {MQMD2_DEFAULT}  ;
 MQPMO   pmo_default   = {MQPMO_DEFAULT}  ;
 MQGMO   gmo_default   = {MQGMO_DEFAULT}  ;

//
//  Global debug variable for controlling current trace status
//

  #define ZERO  0x00000000
  #define CONN  0x80000000
  #define DISC  0x40000000
  #define OPEN  0x20000000
  #define CLOSE 0x10000000
  #define GET   0x08000000
  #define PUT   0x04000000
  #define PUT1  0x02000000
  #define INQ   0x01000000
  #define SET   0x00800000
  #define CMIT  0x00400000
  #define BACK  0x00200000
  #define BRO   0x00100000
  #define HXT   0x00080000
  #define EVENT 0x00040000
  #define TM    0x00020000
  #define MQV   0x00010000
  #define COM   0x00008000
  #define SUB   0x00004000
  #define INIT  0x00000020
  #define TERM  0x00000010
  #define ALL   0xFFFFFFFF

//
//  The TRACE macro is used to enable tracing. This may be perminently
//            enabled/disabled, or dynamically under the control
//            of the RXMQTRACE Rexx variable.
//
//
//  TRACE settings :
//
//  TRACE  -> if ...    for dynamic tracing based on RXMQTRACE (TRACX for values)
//  TRACE  -> printf    for static trace everything debugging
//  TRACE  -> ;         for stop all tracing
//
//  TRACES -> printf    for static trace everything
//  TRACES -> ;         for stop all tracing
//
//  DUMPCB -> if ...    for dynamic tracing based on RXMQTRACE (control blocks)
//  DUMPCB -> printf    for static trace everything debugging
//  DUMPCB -> ;         for stop all tracing
//
//
//  The setting are thus:
//
// #define TRACE(p1,p2) if ( p1 ) printf p2
// #define TRACE(p1,p2) printf p2
// #define TRACE(p1,p2)
//
// #define TRACX(p1,p2) if ( p1 ) DumpDump p2
// #define TRACX(p1,p2) DumpDump p2
// #define TRACX(p1,p2)
//
// #define TRACES(p1) printf p1
// #define TRACES(p1)
//
// #define DUMPCB(p1,p2) if ( p1 ) DumpControlBlock( p2 )
// #define DUMPCB(p1,p2) DumpControlBlock( p2 )
// #define DUMPCB(p1,p2)

#define TRACE(p1,p2) if ( p1 ) printf p2   ;
#define TRACX(p1,p2) if ( p1 ) DumpDump p2 ;
#define TRACES(p1)
#define DUMPCB(p1,p2) if ( p1 ) DumpControlBlock( p2 ) ;

//
// Set of functions to support trace
//
//
// Print a string in hexadecimal form:
// <01234567 89ABCDEF ...>
//
 void DumpHex (MQBYTE * string, MQULONG size)
 {
  MQULONG i                      ;
  printf( "<%.2X", string[0] )   ; // print 1st char
  for(i=1; i < size; i++)
    {
     if (i%4 == 0) printf(" ")   ; // word separator
     printf("%.2X",string[i])    ; // only print hex
    }
  printf(">")                    ;
 }

//
// Print a string in character form (if they are printable):
//  *abcdefghABCDEFGH...*
//
 void DumpChars (MQBYTE * string, MQULONG size)
 {
  MQULONG i                      ;
  printf(" *")                   ;
  for(i=0; i < size; i++) printf("%c", ( isprint(string[i]) ? string[i] : '.' ) )  ;
  printf("* ")                   ;
 }

//
// Print a string as hex and char:
// <01234567 89ABCDEF ...> *abcdefghABCDEFGH...*
//
 void DumpDump (MQBYTE * string, MQULONG size)
 {
  MQULONG i                            ;
  if (size > 32 ) printf( "\n" )       ;
  for(i=0; i < size; i+=32)
  {
   DumpHex  ( &string[i], (size-i > 32) ? 32 : size-i ) ;
   DumpChars( &string[i], (size-i > 32) ? 32 : size-i ) ;
   if (size > 32 ) printf( "\n" )      ;
  }
 }

//
// Set of functions to format internal & MQ control blocks.
//
  void DumpChar (char * name, char byte)
  {
   char      format[100] ; // format variable

   strcpy( format, name )                          ;
   strcat( format,"%c <%X>\n" )                    ; // use exact size and left justify
   printf( format, byte, byte )                    ;
  }

 void DumpString (char * name, char * string, MQULONG size)
 {
  MQULONG   i           ; // counter
  char      format[100] ; // format variable

  strcpy( format, name)                           ;
  strcat( format,"%-*.*s<%.2X")                   ; // use exact size and left justify
  printf( format, size, size, string, string[0] ) ;
  for(i=1; i < size; i++)
    {
     if (i%4 == 0) printf(" ")        ; // word separator
     printf("%.2X" ,string[i] )       ; // only print hex
    }
  printf( ">\n")                      ;
 }

 void DumpBytes (char * name, MQBYTE * string, MQULONG size)
 {
  printf( name )            ; // variable name
  DumpHex  ( string, size ) ;
  DumpChars( string, size ) ;
  if (strlen(name) != 0) printf( "\n") ; // only when full line
 }

 void DumpLongHex (char * name, MQLONG value)
 {
  char      format[100] ; // format variable

  strcpy( format, name)                     ;
  strcat( format,"<%0*"PRIX32">\n")         ; // use exact size and prefix with zeros
  printf( format, 2*sizeof(MQLONG), value ) ;
 }

 void DumpLoLoHex (char * name, MQINT64 value)
 {
  char      format[100] ; // format variable

  strcpy( format, name)                            ;
  strcat( format,"%"PRIX64" <%0*.*llX>\n")              ; // peculiarity here, but works
  printf( format, value, 2*sizeof(MQINT64), value );
 }

 void DumpLongDec (char * name, MQLONG value)
 {
  char      format[100] ; // format variable

  strcpy( format, name)                                       ;
  strcat( format,"%"PRId32" <%0*lX>\n")                       ; // use exact size and prefix with zeros
  printf( format, value, (int32_t)(2*sizeof(MQLONG)), value ) ;
 }

 void DumpPointer (char * name, MQPTR value)
 {
  char      format[100] ; // format variable

  strcpy( format, name)                           ;
  strcat( format,"%p <%*p>\n")                    ; // use exact size
  printf( format, value, 2*sizeof(MQPTR), value ) ;
 }

 void DumpControlBlock (void * cbptr)
 {
  int       i       ; // counter
  RXMQCB  * rxmqcbp ; // pointer to RXMQCB
  MQOD    * odp     ; // pointer to MQOD
  MQMD2   * mdp     ; // pointer to MQMD
  MQGMO   * gmop    ; // pointer to MQGMO
  MQPMO   * pmop    ; // pointer to MQPMO
  MQSD    * sdp     ; // pointer to MQSD

  if ( !memcmp(cbptr, RXMQeyecatcher, sizeof(MQCHAR4)))        // Format RXMQCB
   {
    rxmqcbp = (RXMQCB *) cbptr ;
    printf( "\n")  ;
    DumpString  ( "RXMQCB StrucId           :", rxmqcbp->StrucId,   sizeof(MQCHAR4)  ) ;
    DumpLongHex ( "       tracebits         :", rxmqcbp->tracebits                   ) ;
    DumpString  ( "       QMname            :", rxmqcbp->QMname ,   sizeof(MQCHAR48) ) ;
    DumpLongHex ( "       QMh               :", rxmqcbp->QMh                         ) ;

    for(i=0; i < MAXQS; i++)
      if (rxmqcbp->Qh[i])
        printf("       Qh[%.2d]            :<%0*"PRIX32">\n",
               i, (int)(2*sizeof(MQHOBJ)), (uint32_t)rxmqcbp->Qh[i] ) ;
    printf( "\n") ;
   }

  if ( !memcmp(cbptr, MQOD_STRUC_ID, sizeof(MQCHAR4)))        // Format MQOD
    {
     odp = (MQOD *) cbptr;
     printf( "\n") ;
     DumpString  ( "MQOD StrucId             :", odp->StrucId, sizeof(MQCHAR4) )              ;
     DumpLongDec ( "     Version             :", odp->Version )                               ;
     DumpLongDec ( "     ObjectType          :", odp->ObjectType )                            ;
     DumpString  ( "     ObjectName          :", odp->ObjectName, sizeof(MQCHAR48) )          ;
     DumpString  ( "     ObjectQMgrName      :", odp->ObjectQMgrName, sizeof(MQCHAR48) )      ;
     DumpString  ( "     DynamicQName        :", odp->DynamicQName, sizeof(MQCHAR48) )        ;
     DumpString  ( "     AlternateUserId     :", odp->AlternateUserId, sizeof(MQCHAR12) )     ;
     DumpLongDec ( "     RecsPresent         :", odp->RecsPresent )                           ;
     DumpLongDec ( "     KnownDestCount      :", odp->KnownDestCount )                        ;
     DumpLongDec ( "     UnknownDestCount    :", odp->UnknownDestCount )                      ;
     DumpLongDec ( "     InvalidDestCount    :", odp->InvalidDestCount )                      ;
     DumpLongDec ( "     ObjectRecOffset     :", odp->ObjectRecOffset )                       ;
     DumpLongDec ( "     ResponseRecOffset   :", odp->ResponseRecOffset )                     ;
     DumpPointer ( "     ObjectRecPtr        :", odp->ObjectRecPtr )                          ;
     DumpPointer ( "     ResponseRecPtr      :", odp->ResponseRecPtr )                        ;
     DumpBytes   ( "     AlternateSecurityId :", odp->AlternateSecurityId, sizeof(MQBYTE40) ) ;
     DumpString  ( "     ResolvedQName       :", odp->ResolvedQName, sizeof(MQCHAR48) )       ;
     DumpString  ( "     ResolvedQMgrName    :", odp->ResolvedQMgrName, sizeof(MQCHAR48) )    ;
     DumpPointer ( "     ObjectString.Ptr    :", odp->ObjectString.VSPtr )                    ;
     DumpLongDec ( "     ObjectString.Off    :", odp->ObjectString.VSOffset )                 ;
     DumpLongDec ( "     ObjectString.Size   :", odp->ObjectString.VSBufSize )                ;
     DumpLongDec ( "     ObjectString.Len    :", odp->ObjectString.VSLength )                 ;
     DumpLongDec ( "     ObjectString.CCSID  :", odp->ObjectString.VSCCSID )                  ;
     DumpPointer ( "     SelectionString.Ptr :", odp->SelectionString.VSPtr )                 ;
     DumpLongDec ( "     SelectionString.Off :", odp->SelectionString.VSOffset )              ;
     DumpLongDec ( "     SelectionString.Size:", odp->SelectionString.VSBufSize )             ;
     DumpLongDec ( "     SelectionString.Len :", odp->SelectionString.VSLength )              ;
     DumpLongDec ( "     SelectionString.CCSI:", odp->SelectionString.VSCCSID )               ;
     DumpPointer ( "     ResObjectString.Ptr :", odp->ResObjectString.VSPtr )                 ;
     DumpLongDec ( "     ResObjectString.Off :", odp->ResObjectString.VSOffset )              ;
     DumpLongDec ( "     ResObjectString.Size:", odp->ResObjectString.VSBufSize )             ;
     DumpLongDec ( "     ResObjectString.Len :", odp->ResObjectString.VSLength )              ;
     DumpLongDec ( "     ResObjectString.CCSI:", odp->ResObjectString.VSCCSID )               ;
     DumpLongDec ( "     ResolvedType        :", odp->ResolvedType )                          ;
     printf( "\n") ;
    }

  if ( !memcmp(cbptr, MQMD_STRUC_ID, sizeof(MQCHAR4)))        // Format MQMD
    {
     mdp = (MQMD2 *) cbptr;
     printf( "\n") ;
     DumpString  ( "MQMD StrucId             :", mdp->StrucId, sizeof(MQCHAR4)  )          ;
     DumpLongDec ( "     Version             :", mdp->Version )                            ;
     DumpLongDec ( "     Report              :", mdp->Report )                             ;
     DumpLongDec ( "     MsgType             :", mdp->MsgType )                            ;
     DumpLongDec ( "     Expiry              :", mdp->Expiry )                             ;
     DumpLongDec ( "     Feedback            :", mdp->Feedback )                           ;
     DumpLongDec ( "     Encoding            :", mdp->Encoding )                           ;
     DumpLongDec ( "     CodedCharSetId      :", mdp->CodedCharSetId )                     ;
     DumpString  ( "     Format              :", mdp->Format, sizeof(MQCHAR8) )            ;
     DumpLongDec ( "     Priority            :", mdp->Priority )                           ;
     DumpLongDec ( "     Persistence         :", mdp->Persistence )                        ;
     DumpBytes   ( "     MsgId               :", mdp->MsgId, sizeof(MQBYTE24) )            ;
     DumpBytes   ( "     CorrelId            :", mdp->CorrelId, sizeof(MQBYTE24) )         ;
     DumpLongDec ( "     BackoutCount        :", mdp->BackoutCount )                       ;
     DumpString  ( "     ReplyToQ            :", mdp->ReplyToQ, sizeof(MQCHAR48) )         ;
     DumpString  ( "     ReplyToQMgr         :", mdp->ReplyToQMgr, sizeof(MQCHAR48) )      ;
     DumpString  ( "     UserIdentifier      :", mdp->UserIdentifier, sizeof(MQCHAR12) )   ;
     DumpBytes   ( "     AccountingToken     :", mdp->AccountingToken, sizeof(MQBYTE32) )  ;
     DumpString  ( "     ApplIdentityData    :", mdp->ApplIdentityData, sizeof(MQCHAR32) ) ;
     DumpLongDec ( "     PutApplType         :", mdp->PutApplType )                        ;
     DumpString  ( "     PutApplName         :", mdp->PutApplName, sizeof(MQCHAR28) )      ;
     DumpString  ( "     PutDate             :", mdp->PutDate, sizeof(MQCHAR8) )           ;
     DumpString  ( "     PutTime             :", mdp->PutTime, sizeof(MQCHAR8) )           ;
     DumpString  ( "     ApplOriginData      :", mdp->ApplOriginData, sizeof(MQCHAR4) )    ;
     DumpBytes   ( "     GroupId             :", mdp->GroupId, sizeof(MQBYTE24) )          ;
     DumpLongDec ( "     MsgSeqNumber        :", mdp->MsgSeqNumber )                       ;
     DumpLongDec ( "     Offset              :", mdp->Offset )                             ;
     DumpLongDec ( "     MsgFlags            :", mdp->MsgFlags )                           ;
     DumpLongDec ( "     OriginalLength      :", mdp->OriginalLength )                     ;
     printf( "\n") ;
    }

  if ( !memcmp(cbptr, MQGMO_STRUC_ID, sizeof(MQCHAR4)))        // Format MQGMO
    {
     gmop = (MQGMO *) cbptr;
     printf( "\n") ;
     DumpString  ( "MQGMO StrucId            :", gmop->StrucId, sizeof(MQCHAR4)  )       ;
     DumpLongDec ( "      Version            :", gmop->Version )                         ;
     DumpLongDec ( "      Options            :", gmop->Options )                         ;
     DumpLongDec ( "      WaitInterval       :", gmop->WaitInterval )                    ;
#ifdef __MVS__
     DumpPointer ( "      Signal1            :", gmop->Signal1 )                         ;
#else
     DumpLongDec ( "      Signal1            :", gmop->Signal1 )                         ;
#endif
     DumpLongDec ( "      Signal2            :", gmop->Signal2 )                         ;
     DumpString  ( "      ResolvedQName      :", gmop->ResolvedQName, sizeof(MQCHAR48) ) ;
     DumpLongDec ( "      MatchOptions       :", gmop->MatchOptions )                    ;
     DumpChar    ( "      GroupStatus        :", gmop->GroupStatus )                     ;
     DumpChar    ( "      SegmentStatus      :", gmop->SegmentStatus )                   ;
     DumpChar    ( "      Segmentation       :", gmop->Segmentation )                    ;
     DumpChar    ( "      Reserved1          :", gmop->Reserved1 )                       ;
     DumpBytes   ( "      MsgToken           :", gmop->MsgToken, sizeof(MQBYTE16) )      ;
     DumpLongDec ( "      ReturnedLength     :", gmop->ReturnedLength )                  ;
     DumpLongDec ( "      Reserved2          :", gmop->Reserved2 )                       ;
     DumpLoLoHex ( "      MsgHandle          :", gmop->MsgHandle )                       ;
     printf( "\n") ;
    }

  if ( !memcmp(cbptr, MQPMO_STRUC_ID, sizeof(MQCHAR4)))        // Format MQPMO
    {
     pmop = (MQPMO *) cbptr;
     printf( "\n") ;
     DumpString  ( "MQPMO StrucId            :", pmop->StrucId, sizeof(MQCHAR4)  )          ;
     DumpLongDec ( "      Version            :", pmop->Version )                            ;
     DumpLongDec ( "      Options            :", pmop->Options )                            ;
     DumpLongDec ( "      Timeout            :", pmop->Timeout )                            ;
     DumpLongHex ( "      Context            :", pmop->Context )                            ;
     DumpLongDec ( "      KnownDestCount     :", pmop->KnownDestCount )                     ;
     DumpLongDec ( "      UnknownDestCount   :", pmop->UnknownDestCount )                   ;
     DumpLongDec ( "      InvalidDestCount   :", pmop->InvalidDestCount )                   ;
     DumpString  ( "      ResolvedQName      :", pmop->ResolvedQName, sizeof(MQCHAR48) )    ;
     DumpString  ( "      ResolvedQMgrName   :", pmop->ResolvedQMgrName, sizeof(MQCHAR48) ) ;
     DumpLongDec ( "      RecsPresent        :", pmop->RecsPresent )                        ;
     DumpLongDec ( "      PutMsgRecFields    :", pmop->PutMsgRecFields )                    ;
     DumpLongDec ( "      PutMsgRecOffset    :", pmop->PutMsgRecOffset )                    ;
     DumpLongDec ( "      ResponseRecOffset  :", pmop->ResponseRecOffset )                  ;
     DumpPointer ( "      ResponseRecPtr     :", pmop->ResponseRecPtr )                     ;
     DumpLoLoHex ( "      OriginalMsgHandle  :", pmop->OriginalMsgHandle )                  ;
     DumpLoLoHex ( "      NewMsgHandle       :", pmop->NewMsgHandle )                       ;
     DumpLongDec ( "      Action             :", pmop->Action )                             ;
     DumpLongDec ( "      PubLevel           :", pmop->PubLevel )                           ;
     printf( "\n") ;
    }

  if ( !memcmp(cbptr, MQSD_STRUC_ID, sizeof(MQCHAR4)))        // Format MQSD
    {
     sdp = (MQSD *) cbptr;
     printf( "\n") ;
     DumpString  ( "MQSD StrucId             :", sdp->StrucId, sizeof(MQCHAR4)  )             ;
     DumpLongDec ( "     Version             :", sdp->Version )                               ;
     DumpLongDec ( "     Options             :", sdp->Options )                               ;
     DumpString  ( "     ObjectName          :", sdp->ObjectName, sizeof(MQCHAR48) )          ;
     DumpString  ( "     AlternateUserId     :", sdp->AlternateUserId, sizeof(MQCHAR12) )     ;
     DumpBytes   ( "     AlternateSecurityId :", sdp->AlternateSecurityId, sizeof(MQBYTE40) ) ;
     DumpLongDec ( "     SubExpiry           :", sdp->SubExpiry )                             ;
     DumpPointer ( "     ObjectString.Ptr    :", sdp->ObjectString.VSPtr )                    ;
     DumpLongDec ( "     ObjectString.Off    :", sdp->ObjectString.VSOffset )                 ;
     DumpLongDec ( "     ObjectString.Size   :", sdp->ObjectString.VSBufSize )                ;
     DumpLongDec ( "     ObjectString.Len    :", sdp->ObjectString.VSLength )                 ;
     DumpLongDec ( "     ObjectString.CCSID  :", sdp->ObjectString.VSCCSID )                  ;
     DumpPointer ( "     SubName.Ptr         :", sdp->SubName.VSPtr )                         ;
     DumpLongDec ( "     SubName.Off         :", sdp->SubName.VSOffset )                      ;
     DumpLongDec ( "     SubName.Size        :", sdp->SubName.VSBufSize )                     ;
     DumpLongDec ( "     SubName.Len         :", sdp->SubName.VSLength )                      ;
     DumpLongDec ( "     SubName.CCSID       :", sdp->SubName.VSCCSID )                       ;
     DumpPointer ( "     SubUserData.Ptr     :", sdp->SubUserData.VSPtr )                     ;
     DumpLongDec ( "     SubUserData.Off     :", sdp->SubUserData.VSOffset )                  ;
     DumpLongDec ( "     SubUserData.Size    :", sdp->SubUserData.VSBufSize )                 ;
     DumpLongDec ( "     SubUserData.Len     :", sdp->SubUserData.VSLength )                  ;
     DumpLongDec ( "     SubUserData.CCSID   :", sdp->SubUserData.VSCCSID )                   ;
     DumpBytes   ( "     SubCorrelId         :", sdp->SubCorrelId, sizeof(MQBYTE24) )         ;
     DumpLongDec ( "     PubPriority         :", sdp->PubPriority )                           ;
     DumpBytes   ( "     PubAccountingToken  :", sdp->PubAccountingToken, sizeof(MQBYTE32) )  ;
     DumpString  ( "     PubApplIdentityData :", sdp->PubApplIdentityData, sizeof(MQCHAR32) ) ;
     DumpPointer ( "     SelectionString.Ptr :", sdp->SelectionString.VSPtr )                 ;
     DumpLongDec ( "     SelectionString.Off :", sdp->SelectionString.VSOffset )              ;
     DumpLongDec ( "     SelectionString.Size:", sdp->SelectionString.VSBufSize )             ;
     DumpLongDec ( "     SelectionString.Len :", sdp->SelectionString.VSLength )              ;
     DumpLongDec ( "     SelectionString.CCSI:", sdp->SelectionString.VSCCSID )               ;
     DumpLongDec ( "     SubLevel            :", sdp->SubLevel )                              ;
     DumpPointer ( "     ResObjectString.Ptr :", sdp->ResObjectString.VSPtr )                 ;
     DumpLongDec ( "     ResObjectString.Off :", sdp->ResObjectString.VSOffset )              ;
     DumpLongDec ( "     ResObjectString.Size:", sdp->ResObjectString.VSBufSize )             ;
     DumpLongDec ( "     ResObjectString.Len :", sdp->ResObjectString.VSLength )              ;
     DumpLongDec ( "     ResObjectString.CCSI:", sdp->ResObjectString.VSCCSID )               ;
     printf( "\n") ;
    }
 }


//
// Internal Functions
//

//
// Functions that Fetch and Set REXX Variables in desired format
//
//
// Fetch integer value from REXX function parameter
//
void parm_to_ulong ( RXSTRING   parm    // parameter REXX string
                   , MQLONG *   number  // received value
                   )
{
  MQULONG i  ;
 *number = 0 ;
 for(i=0; i<parm.strlength; i++)
  {
   if((parm.strptr[i] < '0') || (parm.strptr[i] > '9')) return;
   else *number = (*number)*10 + ((parm.strptr[i]) & 0x0f);
  }
}

//
// Fetch MQPTR value from REXX variable (no conversion required!)
//
void var_to_ptr ( MQULONG    traceid      // trace id of caller
                , char     * name         // variable name
                , MQPTR    * anchorptr    // received value
                )
{
 SHVBLOCK                sv1              ;  // REXX var interface CB
 int                     sv1rc            ;  // REXX var interface RC
 MQPTR                   tempptr = 0      ;  // Receive value here

 sv1.shvnext     = 0                      ; // Fetch only one variable
 sv1.shvcode     = RXSHV_SYFET            ; // Fetch operation
 sv1.shvret      = 0                      ; // Zero out RC

 MAKERXSTRING(sv1.shvname,name,strlen(name))  ; // Construct REXX variable name structure
 sv1.shvnamelen  = sv1.shvname.strlength      ; // REXX variable name length

 sv1.shvvalue.strptr    = (char *) &tempptr        ; // Set pointer to value buffer for REXX
 sv1.shvvalue.strlength = sizeof(MQPTR)            ; // Set max accepted value length
 sv1.shvvaluelen        = sv1.shvvalue.strlength   ; // Actual length will be here

 sv1rc           = RexxVariablePool(&sv1)          ; // Call REXX variable interface

 TRACES(("RXfetch rc = %d, %s ->%p<-%"PRIu32"/%u\n",
         sv1rc,name,tempptr,(uint32_t)sv1.shvvaluelen,(uint32_t)sizeof(tempptr))
       ) ;

 if (    (sv1rc                       == RXSHV_OK)
      && (tempptr                     != NULL    ) )
          *anchorptr = (void **) tempptr     ; // pointer value

 return;
} // End of var_to_ptr

//
// Fetch MQLONG value from REXX stem variable
//
void stem_to_long ( MQULONG    traceid      // trace id of caller
                  , RXSTRING   stem         // stem variable name high
                  , char       name[]       // stem variable name low
                  , MQLONG   * number       // received value
                  )
{
 char                    varnamc[100] ;  // Char version of variable name
 char                    varvalc[100] ;  // Char version of variable value
 SHVBLOCK                sv1          ;  // REXX var interface CB
 int                     sv1rc        ;  // REXX var interface RC

 memset(&varvalc,0,sizeof(varvalc))   ; // Clear REXX variable value buffer
 sv1.shvnext     = 0                  ; // Fetch only one variable
 sv1.shvcode     = RXSHV_SYFET        ; // Fetch operation
 sv1.shvret      = 0                  ; // Zero out RC

 sprintf(varnamc,"%.*s%s",(int)stem.strlength,stem.strptr,name) ; // Construct REXX variable name
 MAKERXSTRING(sv1.shvname,varnamc,strlen(varnamc))              ; // Construct REXX variable name structure
 sv1.shvnamelen  = sv1.shvname.strlength                        ; // REXX variable name length

 sv1.shvvalue.strptr    = varvalc                   ; // Set pointer to value buffer for REXX
 sv1.shvvalue.strlength = sizeof(varvalc)           ; // Set max accepted value length
 sv1.shvvaluelen        = sv1.shvvalue.strlength    ; // Actual length will be here

 sv1rc           = RexxVariablePool(&sv1)           ; // Call REXX variable interface

 TRACE(traceid, ("RXfetch rc = %d, %s ->%s<-%"PRIu32"/%"PRIu32"\n",
                 sv1rc,varnamc,varvalc,(uint32_t)sv1.shvvaluelen,(uint32_t)sizeof(varvalc))
      ) ;

 if (    (sv1rc                       == RXSHV_OK)
      && (strlen(varvalc)             != 0       ) )
          sscanf(varvalc,"%"SCNd32,(int32_t*)number) ; // int32_t value

 return ;
} // End of stem_to_long

//
// Fetch MQINT64 value from REXX stem variable
//
void stem_to_int64 ( MQULONG    traceid      // trace id of caller
                   , RXSTRING   stem         // stem variable name high
                   , char       name[]       // stem variable name low
                   , MQINT64 *  number       // received value
                   )
{
 char                    varnamc[100] ;  // Char version of variable name
 char                    varvalc[100] ;  // Char version of variable value
 SHVBLOCK                sv1          ;  // REXX var interface CB
 int                     sv1rc        ;  // REXX var interface RC

 memset(&varvalc,0,sizeof(varvalc))   ; // Clear REXX variable value buffer
 sv1.shvnext     = 0                  ; // Fetch only one variable
 sv1.shvcode     = RXSHV_SYFET        ; // Fetch operation
 sv1.shvret      = 0                  ; // Zero out RC

 sprintf(varnamc,"%.*s%s",(int)stem.strlength,stem.strptr,name)    ; // Construct REXX variable name
 MAKERXSTRING(sv1.shvname,varnamc,strlen(varnamc))                 ; // Construct REXX variable name structure
 sv1.shvnamelen  = sv1.shvname.strlength                           ; // REXX variable name length

 sv1.shvvalue.strptr    = varvalc                   ; // Set pointer to value buffer for REXX
 sv1.shvvalue.strlength = sizeof(varvalc)           ; // Set max accepted value length
 sv1.shvvaluelen        = sv1.shvvalue.strlength    ; // Actual length will be here

 sv1rc           = RexxVariablePool(&sv1)           ; // Call REXX variable interface

 TRACE(traceid, ("RXfetch rc = %d, %s ->%s<-%"PRIu32"/%"PRIu32"\n",
                 sv1rc,varnamc,varvalc,(uint32_t)sv1.shvvaluelen,(uint32_t)sizeof(varvalc))
                ) ;

 if (    (sv1rc               == RXSHV_OK)
      && (strlen(varvalc)     != 0       ) )
//        sscanf(varvalc,"%llu",number)                 ; // long int value
          sscanf(varvalc,"%"SCNu64"",(uint64_t*)number) ; // workaround for UNIX/Linux
 return ;
} // End of stem_to_int64

//
// Fetch single MQCHAR value from REXX variable
//
void stem_to_char ( MQULONG    traceid      // trace id of caller
                  , RXSTRING   stem         // stem variable name high
                  , char       name[]       // stem variable name low
                  , MQCHAR *   letter       // received value
                  )
{
 char                    varnamc[100] ;  // Char version of variable name
 char                    varvalc[100] ;  // Char version of variable value
 SHVBLOCK                sv1          ;  // REXX var interface CB
 int                     sv1rc        ;  // REXX var interface RC

 memset(&varvalc,0,sizeof(varvalc))   ; // Clear REXX variable value buffer
 sv1.shvnext     = 0                  ; // Fetch only one variable
 sv1.shvcode     = RXSHV_SYFET        ; // Fetch operation
 sv1.shvret      = 0                  ; // Zero out RC

 sprintf(varnamc,"%.*s%s",(int)stem.strlength,stem.strptr,name) ; // Construct REXX variable name
 MAKERXSTRING(sv1.shvname,varnamc,strlen(varnamc))        ; // Construct REXX variable name structure
 sv1.shvnamelen  = sv1.shvname.strlength                  ; // REXX variable name length

 sv1.shvvalue.strptr    = varvalc                         ; // Set pointer to value buffer for REXX
 sv1.shvvalue.strlength = sizeof(varvalc)                 ; // Set max accepted value length
 sv1.shvvaluelen        = sv1.shvvalue.strlength          ; // Actual length will be here

 sv1rc           = RexxVariablePool(&sv1)                 ; // Call REXX variable interface

 TRACE(traceid, ("RXfetch rc = %d, %s ->%s<-%"PRIu32"/%"PRIu32"\n",
                 sv1rc,varnamc,varvalc,(uint32_t)sv1.shvvaluelen,(uint32_t)sizeof(varvalc))
      ) ;

 if (    (sv1rc                          == RXSHV_OK)
         && (strlen(varvalc)             != 0       ))
         *letter = varvalc[0]                         ; // char value

 return ;
} // End of stem_to_char


//
// Fetch string MQCHAR value from REXX variable
//
void stem_to_string ( MQULONG    traceid      // trace id of caller
                    , RXSTRING   stem         // stem variable name high
                    , char       name[]       // stem variable name low
                    , MQCHAR     string[]     // received value
                    , int        size         // max size of value
                    )
{
 char                    varnamc[100] ;  // Char version of variable name
 char                    varvalc[100] ;  // Char version of variable value
 SHVBLOCK                sv1          ;  // REXX var interface CB
 int                     sv1rc        ;  // REXX var interface RC

 if ( size > (int) sizeof(varvalc) )   // should never happen
   {
    TRACE(traceid, ("Size = %d more than buffer length %"PRIu32" !\n",
                    size,(uint32_t)sizeof(varvalc))
         ) ;
    return ;
   }
 memset(&varvalc,0,sizeof(varvalc))       ; // To ensure value is 0-terminated

 sv1.shvnext     = 0                      ; // Fetch only one variable
 sv1.shvcode     = RXSHV_SYFET            ; // Fetch operation
 sv1.shvret      = 0                      ; // Zero out RC

 sprintf(varnamc,"%.*s%s",(int)stem.strlength,stem.strptr,name) ; // Construct REXX variable name
 MAKERXSTRING(sv1.shvname,varnamc,strlen(varnamc))              ; // Construct REXX variable name structure
 sv1.shvnamelen  = sv1.shvname.strlength                        ; // REXX variable name length

 sv1.shvvalue.strptr    = varvalc                ; // Set pointer to value buffer for REXX
 sv1.shvvalue.strlength = size                   ; // Set max accepted value length
 sv1.shvvaluelen        = sv1.shvvalue.strlength ; // Actual length will be here

 sv1rc           = RexxVariablePool(&sv1)        ; // Call REXX variable interface

 TRACE(traceid, ("RXfetch rc = %d, %s ->%s<-%"PRIu32"/%d\n",
                  sv1rc,varnamc,varvalc,(uint32_t)sv1.shvvaluelen,size)
      ) ;

 if (sv1rc == RXSHV_OK) strcpy(string, varvalc)  ; // Return value, only if OK!

 return ;
} // End of stem_to_string

//
// Fetch MQBYTE value from REXX variable
// Use this only for MQBYTExx variables
//
void stem_to_bytes  ( MQULONG    traceid      // trace id of caller
                    , RXSTRING   stem         // stem variable name high
                    , char       name[]   // stem variable name low
                    , MQBYTE     string[] // received value
                    , int        size         // max size of value
                    )
{
 char                    varnamc[100] ;  // Char version of variable name
 MQBYTE                  varvalc[100] ;  // Char version of variable value
 SHVBLOCK                sv1          ;  // REXX var interface CB
 int                     sv1rc        ;  // REXX var interface RC

 if ( size > (int) sizeof(varvalc) )   // should never happen
   {
   TRACE(traceid, ("Size = %d more than buffer length %"PRIu32" !\n",
                   size,(uint32_t)sizeof(varvalc))
        ) ;
    return ;
   }
 memset(&varvalc,0,sizeof(varvalc))   ; // For readability only

 sv1.shvnext     = 0                  ; // Fetch only one variable
 sv1.shvcode     = RXSHV_SYFET        ; // Fetch operation
 sv1.shvret      = 0                  ; // Zero out RC

 sprintf(varnamc,"%.*s%s",(int)stem.strlength,stem.strptr,name) ; // Construct REXX variable name
 MAKERXSTRING(sv1.shvname,varnamc,strlen(varnamc))              ; // Construct REXX variable name structure
 sv1.shvnamelen  = sv1.shvname.strlength                        ; // REXX variable name length

 sv1.shvvalue.strptr    = (char *) varvalc       ; // Set pointer to value buffer for REXX
 sv1.shvvalue.strlength = size                   ; // Set max accepted value length
 sv1.shvvaluelen        = sv1.shvvalue.strlength ; // Actual length will be here

 sv1rc           = RexxVariablePool(&sv1)        ; // Call REXX variable interface

 TRACE(traceid, ("RXfetch rc = %d, %s ->",sv1rc,varnamc) )               ;
 TRACX(traceid, (varvalc,sv1.shvvaluelen) )                              ;
 TRACE(traceid, ("<-%"PRIu32"/%d\n",(uint32_t)sv1.shvvaluelen,size) )    ;

 if (sv1rc == RXSHV_OK) memcpy(string, &varvalc, sv1.shvvalue.strlength) ;
                                                   // Return value, only if OK!
 return ;
} // End of stem_to_bytes

//
// Fetch long MQBYTE value from REXX variable
// Buffer is provided by the caller
//
MQULONG stem_to_data ( MQULONG    traceid      // trace id of caller
                     , RXSTRING   stem         // stem variable name high
                     , char       name[]       // stem variable name low
                     , MQBYTE     string[]     // received value
                     , int        size         // max size of value
                     )
{
 char                    varnamc[100] ;  // Char version of variable name
 SHVBLOCK                sv1              ;  // REXX var interface CB
 int                     sv1rc            ;  // REXX var interface RC

 sv1.shvnext     = 0                      ; // Fetch only one variable
 sv1.shvcode     = RXSHV_SYFET            ; // Fetch operation
 sv1.shvret      = 0                      ; // Zero out RC

 sprintf(varnamc,"%.*s%s",(int)stem.strlength,stem.strptr,name) ; // Construct REXX variable name
 MAKERXSTRING(sv1.shvname,varnamc,strlen(varnamc))              ; // Construct REXX variable name structure
 sv1.shvnamelen  = sv1.shvname.strlength                        ; // REXX variable name length

 sv1.shvvalue.strptr    = (char *) string        ; // Set pointer to value buffer for REXX
 sv1.shvvalue.strlength = size                   ; // Set max accepted value length
 sv1.shvvaluelen        = sv1.shvvalue.strlength ; // Actual length will be here

 sv1rc           = RexxVariablePool(&sv1)        ; // Call REXX variable interface

 TRACE(traceid, ("RXfetch rc = %d, %s ->",sv1rc,varnamc) )    ;
 TRACX(traceid, (string,sv1.shvvaluelen) )                     ;
 TRACE(traceid, ("<-%"PRIu32"/%d\n",(uint32_t)sv1.shvvaluelen,size) ) ;

 if (sv1rc == RXSHV_OK) return (sv1.shvvalue.strlength)    ;
 else                   return 0                           ;
} // End of stem_to_data

//
// Fetch MQCHARV value from REXX variable
//
void stem_to_strinv ( MQULONG    traceid     // trace id of caller
                    , RXSTRING   stem        // stem variable name high
                    , char       name[]  // stem variable name low
                    , MQCHARV *  string      // received value
                    )
{
 char                    varnamc[100] ;  // Char version of variable name
 RXSTRING                varname          ;  // REXX variable name

 sprintf(varnamc,"%.*s%s",(int)stem.strlength,stem.strptr,name) ; // Construct REXX variable name
 MAKERXSTRING(varname,varnamc,strlen(varnamc))                  ; // Construct REXX variable name structure

 stem_to_long  (traceid, varname, ".0" ,    &string->VSBufSize)    ; // Assume it is buffer length
 stem_to_long  (traceid, varname, ".CCSI" , &string->VSCCSID)      ; // CCSID may be required

 //
 // Allocate data buffer to store stem.name.1 variable data.
 // Buffer will be freed during call to stem_from_strinv
 //
 if ( string->VSBufSize != 0 )
   {
    TRACE(traceid, ("Doing malloc for %"PRId32" bytes",(int32_t)string->VSBufSize))  ;
    string->VSPtr = malloc(string->VSBufSize)                         ;
    if ( string->VSPtr == NULL )
      {
       TRACE(traceid, ("malloc rc %d\n",errno) )                      ;
       string->VSBufSize = 0                                          ;
      }
    else string->VSLength = stem_to_data(traceid, varname, ".1",
                                   (MQBYTE *)string->VSPtr, string->VSBufSize) ;
   }

 return ;
} // End of stem_to_strinv

//
// Set REXX variable to MQPTR value (no conversion required!)
//
void var_from_ptr ( MQULONG    traceid      // trace id of caller
                   , char    * name         // variable name
                   , MQPTR     anchor       // value to set
                   )
{
 SHVBLOCK                sv1              ;  // REXX var interface CB
 int                     sv1rc            ;  // REXX var interface RC

 sv1.shvnext     = 0                      ; // Set only one variable
 sv1.shvcode     = RXSHV_SYSET            ; // Set operation
 sv1.shvret      = 0                      ; // Zero out RC

 MAKERXSTRING(sv1.shvname,name,strlen(name))        ; // Construct REXX variable name structure
 sv1.shvnamelen  = sv1.shvname.strlength            ; // REXX variable name length

 MAKERXSTRING(sv1.shvvalue,(char*)&anchor,sizeof(anchor))  ; // Construct REXX variable value structure
 sv1.shvvaluelen = sv1.shvvalue.strlength           ; // Set actual value length for REXX

 sv1rc           = RexxVariablePool(&sv1)           ; // Call REXX variable interface

 TRACES(("RXset rc = %d, %s ->%p<-%ld/%"PRId32"\n",
         sv1rc,name,anchor,sv1.shvvaluelen,(int32_t)sizeof(anchor))
      ) ;

 return ;
} // End of var_from_ptr

//
// Set REXX variable to MQLONG value
//
void stem_from_long ( MQULONG    traceid  // trace id of caller
                    , char       zlist[]  // .ZLIST string for accumulation
                    , RXSTRING   stem     // stem variable name high
                    , char       name[]   // stem variable name low
                    , int32_t    number   // value to set
                    )
{
 char                    varnamc[100] ; // Char version of variable name
 char                    varvalc[100] ; // Char version of variable value
 SHVBLOCK                sv1          ; // REXX var interface CB
 int                     sv1rc        ; // REXX var interface RC

 sv1.shvnext     = 0                  ; // Set only one variable
 sv1.shvcode     = RXSHV_SYSET        ; // Set operation
 sv1.shvret      = 0                  ; // Zero out RC

 sprintf(varnamc,"%.*s%s",(int)stem.strlength,stem.strptr,name) ; // Construct REXX variable name
 MAKERXSTRING(sv1.shvname,varnamc,strlen(varnamc))              ; // Construct REXX variable name structure
 sv1.shvnamelen  = sv1.shvname.strlength                        ; // REXX variable name length

 sprintf(varvalc,"%"PRId32,number)                   ; // Convert long int to char
 MAKERXSTRING(sv1.shvvalue,varvalc,strlen(varvalc))  ; // Construct REXX variable value structure
 sv1.shvvaluelen = sv1.shvvalue.strlength            ; // Set actual value length for REXX

 sv1rc           = RexxVariablePool(&sv1)            ; // Call REXX variable interface

 TRACE(traceid, ("RXset rc = %d, %s ->%s<-%"PRIu32"/%"PRIu32"\n",
                  sv1rc,varnamc,varvalc,(uint32_t)sv1.shvvaluelen,(uint32_t)strlen(varvalc))
      ) ;

 if (   ( (sv1rc == RXSHV_OK) || (sv1rc == RXSHV_NEWV) )
     && (zlist != NULL))
   {
     strcat(zlist," ")        ;
     strcat(zlist,name)       ;
   }
 return ;
} // End of stem_from_long

//
// Set REXX variable to MQINT64 value
//
void stem_from_int64 ( MQULONG    traceid  // trace id of caller
                     , char       zlist[]  // .ZLIST string for accumulation
                     , RXSTRING   stem     // stem variable name high
                     , char       name[]   // stem variable name low
                     , MQINT64    number   // value to set
                     )
{
 char                    varnamc[100] ;  // Char version of variable name
 char                    varvalc[100] ;  // Char version of variable value
 SHVBLOCK                sv1          ;  // REXX var interface CB
 int                     sv1rc        ;  // REXX var interface RC

 sv1.shvnext     = 0                  ; // Set only one variable
 sv1.shvcode     = RXSHV_SYSET        ; // Set operation
 sv1.shvret      = 0                  ; // Zero out RC

 sprintf(varnamc,"%.*s%s",(int)stem.strlength,stem.strptr,name) ; // Construct REXX variable name
 MAKERXSTRING(sv1.shvname,varnamc,strlen(varnamc))              ; // Construct REXX variable name structure
 sv1.shvnamelen  = sv1.shvname.strlength                        ; // REXX variable name length

// sprintf((char *)varvalc,"%lld",number)              ; // Convert MQINT64 to char
 sprintf((char *)varvalc,"%"PRIu64,(uint64_t)number) ; // Convert MQINT64 to char (workaround for ..IX)
 MAKERXSTRING(sv1.shvvalue,varvalc,strlen(varvalc))  ; // Construct REXX variable value structure
 sv1.shvvaluelen = sv1.shvvalue.strlength            ; // Set actual value length for REXX

 sv1rc           = RexxVariablePool(&sv1)            ; // Call REXX variable interface

 TRACE(traceid, ("RXset rc = %d, %s ->%s<-%"PRIu32"/%"PRIu32"\n",
                 sv1rc,varnamc,varvalc,(uint32_t)sv1.shvvaluelen,(uint32_t)strlen(varvalc))
      ) ;

 if (   ((sv1rc == RXSHV_OK) || (sv1rc == RXSHV_NEWV))
     && (zlist != NULL))
 {
     strcat(zlist," ")        ;
     strcat(zlist,name)       ;
 }
 return ;
} // End of stem_from_int64

//
// Set REXX variable to single MQCHAR value
//
void stem_from_char ( MQULONG    traceid  // trace id of caller
                    , char       zlist[]  // .ZLIST string for accumulation
                    , RXSTRING   stem     // stem variable name high
                    , char       name[]   // stem variable name low
                    , MQCHAR     letter   // value to set
                    )
{
 char                    varnamc[100] ;  // Char version of variable name
 SHVBLOCK                sv1          ;  // REXX var interface CB
 int                     sv1rc        ;  // REXX var interface RC

 sv1.shvnext     = 0           ; // Set only one variable
 sv1.shvcode     = RXSHV_SYSET ; // Set operation
 sv1.shvret      = 0           ; // Zero out RC

 sprintf(varnamc,"%.*s%s",(int)stem.strlength,stem.strptr,name) ; // Construct REXX variable name
 MAKERXSTRING(sv1.shvname,varnamc,strlen(varnamc))              ; // Construct REXX variable name structure
 sv1.shvnamelen  = sv1.shvname.strlength                        ; // REXX variable name length

 MAKERXSTRING(sv1.shvvalue,&letter,sizeof(MQCHAR))  ; // Construct REXX variable value structure
 sv1.shvvaluelen = sv1.shvvalue.strlength           ; // Set actual value length for REXX

 sv1rc           = RexxVariablePool(&sv1)           ; // Call REXX variable interface

 TRACE(traceid, ("RXset rc = %d, %s ->%c<-%"PRIu32"/%"PRIu32"\n",
                 sv1rc,varnamc,letter,(uint32_t)sv1.shvvaluelen,(uint32_t)sizeof(MQCHAR))
      ) ;

 if (   ((sv1rc == RXSHV_OK) || (sv1rc == RXSHV_NEWV))
     && (zlist != NULL))
 {
     strcat(zlist," ")        ;
     strcat(zlist,name)       ;
 }

 return ;
} // End of stem_from_char

//
// Set REXX variable to MQCHAR string value
//
void stem_from_string ( MQULONG    traceid  // trace id of caller
                      , char       zlist[]  // .ZLIST string for accumulation
                      , RXSTRING   stem     // stem variable name high
                      , char       name[]   // stem variable name low
                      , MQCHAR     string[] // value to set
                      , int        size     // full size of value
                      )
{
 char                    varnamc[100] ;  // Char version of variable name
 SHVBLOCK                sv1          ;  // REXX var interface CB
 int                     sv1rc        ;  // REXX var interface RC

 sv1.shvnext     = 0                  ; // Set only one variable
 sv1.shvcode     = RXSHV_SYSET        ; // Set operation
 sv1.shvret      = 0                  ; // Zero out RC

 sprintf(varnamc,"%.*s%s",(int)stem.strlength,stem.strptr,name) ; // Construct REXX variable name
 MAKERXSTRING(sv1.shvname,varnamc,strlen(varnamc))              ; // Construct REXX variable name structure
 sv1.shvnamelen  = sv1.shvname.strlength                        ; // REXX variable name length

 if ( memchr(string,0,size) != NULL ) sv1.shvvaluelen = strlen(string) ; // If null-terminated
 else sv1.shvvaluelen = size                                           ; // else take full string
 MAKERXSTRING(sv1.shvvalue,string,sv1.shvvaluelen)   ; // Construct REXX variable value structure

 sv1rc           = RexxVariablePool(&sv1)            ; // Call REXX variable interface

 TRACE(traceid, ("RXset rc = %d, %s ->%.*s<-%"PRIu32"/%d\n",
                 sv1rc,varnamc,size,string,(uint32_t)sv1.shvvaluelen,size)
      ) ;

 if (   ((sv1rc == RXSHV_OK) || (sv1rc == RXSHV_NEWV))
     && (zlist != NULL))
 {
     strcat(zlist," ")        ;
     strcat(zlist,name)       ;
 }

 return ;
} // End of stem_from_string

//
// Set REXX variable to MQBYTE string value
//
void stem_from_bytes  ( MQULONG    traceid   // trace id of caller
                      , char       zlist[]   // .ZLIST string for accumulation
                      , RXSTRING   stem      // stem variable name high
                      , char     * name      // stem variable name low
                      , MQBYTE   * string    // value to set
                      , int        size      // full size of value
                      )
{
 char                    varnamc[100]     ;  // Char version of variable name
 SHVBLOCK                sv1              ;  // REXX var interface CB
 int                     sv1rc            ;  // REXX var interface RC

 sv1.shvnext     = 0                      ; // Set only one variable
 sv1.shvcode     = RXSHV_SYSET            ; // Set operation
 sv1.shvret      = 0                      ; // Zero out RC

 sprintf(varnamc,"%.*s%s",(int)stem.strlength,stem.strptr,name) ; // Construct REXX variable name
 MAKERXSTRING(sv1.shvname,varnamc,strlen(varnamc))              ; // Construct REXX variable name structure
 sv1.shvnamelen  = sv1.shvname.strlength                        ; // REXX variable name length

 MAKERXSTRING(sv1.shvvalue,(char*)string,size)                  ; // Construct REXX variable value structure
 sv1.shvvaluelen = sv1.shvvalue.strlength                       ; // Set actual value length for REXX

 sv1rc           = RexxVariablePool(&sv1)                       ; // Call REXX variable interface

 TRACE(traceid, ("RXset rc = %d, %s ->",sv1rc,varnamc) )       ;
 TRACX(traceid, (string,sv1.shvvaluelen) )                      ;
 TRACE(traceid, ("<-%"PRIu32"/%d\n",(uint32_t)sv1.shvvaluelen,size) ) ;

 if (   ((sv1rc == RXSHV_OK) || (sv1rc == RXSHV_NEWV))
     && (zlist != NULL))
 {
     strcat(zlist," ")                      ;
     strcat(zlist,(const char *)name)       ;
 }

 return ;
} // End of stem_from_bytes

//
// Set REXX variable to MQCHARV value
//
void stem_from_strinv ( MQULONG    traceid      // trace id of caller
                      , char       zlist[]      // .ZLIST string for accumulation
                      , RXSTRING   stem         // stem variable name high
                      , char       name[]       // stem variable name low
                      , MQCHARV    string       // value to set
                      )
{
 char                   varnamc[100] ;      // Char version of variable name

 stem_from_long  (traceid, zlist, stem,
                  strcat(strcpy(varnamc,name),".0"),
                  string.VSLength)                          ;
 stem_from_long  (traceid, zlist, stem,
                  strcat(strcpy(varnamc,name),".CCSI"),
                  string.VSCCSID)                           ;
 //
 // Free data buffer for stem.name.1 variable data, if allocated.
 //
 if ( string.VSPtr != 0 )
    {
     stem_from_bytes (traceid, zlist, stem,
                      strcat(strcpy(varnamc,name),".1"),
                      (MQBYTE *)string.VSPtr, string.VSLength)  ;
     TRACE(traceid, ("Freemaining VS area\n"))                  ;
     free(string.VSPtr)                                         ;
    }

 return ;
} // End of stem_from_strinv


// set_entry is called at the beginning of each external function execution
//
// Tracing is controlled by the TRACE macro.
//
//         If the dynamic usage of TRACE is set, then the
//            set_entry routine determines whether or not data
//            is printed (based on the RXMQTRACE Rexx var).
//

//
// set_entry will obtain the trace setting currently in effect,
//             and set tracebits variable for later checking in DEBUG macros
//
//         Known settings are :
//
//                              CONN  -> mqconn
//                              DISC  -> mqdisc
//                              OPEN  -> mqopen
//                              CLOSE -> mqclose
//                              GET   -> mqget
//                              PUT   -> mqput
//                              PUT1  -> mqput1
//                              INQ   -> mqinq
//                              SET   -> mqset
//                              CMIT  -> mqcmit
//                              BACK  -> mqback
//                              SUB   -> mqsub
//
//                              BRO   -> Browse extension
//                              HXT   -> Header extraction extension
//                              EVENT -> Event determination extension
//                              TM    -> Trigger message extension
//                              COM   -> Command interface
//                              MQV   -> Debug a RXMQV
//
//                              INIT  -> initialization processing
//                              TERM  -> Deregistration processing
//
//                              *     -> Trace everything!!!
//
//
//  These settings are extracted from the RXMQTRACE Rexx Variable
//    (so RXMQTRACE = 'INQ OPEN' will trace Open and Inq operations)
//
//
//

MQLONG  set_envir ( char     * func        // Current function executed
                  , MQULONG  * traceidptr  // Trace bitmask
                  , RXMQCB  ** anchorptr   // pointer to RXMQCB pointer
                  )
{
  RXSTRING              varname      ;  // Variable name
  char                  varvalc[100] ;  // Char version of variable
  MQLONG                rc = 0       ;

// First of all let's try to access our anchor control block for the current thread.
// It may or may not exist when calling RXMQ function.
// If exists, use it; otherwise create it.
// This control block is a RXMQCB structure, which address is assigned to REXX RXMQANCHOR variable.

 var_to_ptr (ZERO, RXMQANCHOR , (void **) anchorptr)  ;

 if ( (*anchorptr) == NULL    )
   {
    TRACES(("Doing malloc for RXMQCB for %u bytes\n",(uint32_t)sizeof(RXMQCB))) ;
    *anchorptr = (RXMQCB *) malloc(sizeof(RXMQCB))                    ;

    if ( *anchorptr == NULL )
      {
       TRACES(("malloc rc = %d\n",errno)) ;
       rc = -77                           ;
      }
    else
      {
       var_from_ptr  (ZERO, RXMQANCHOR , *anchorptr)        ;
       memset (*anchorptr, 0, sizeof(RXMQCB))                  ;
       memcpy ((*anchorptr)->StrucId, RXMQeyecatcher, sizeof(MQCHAR4)) ;
      }
   }

 if (rc == 0 )
   {
    (*anchorptr)->tracebits = ZERO                                 ;

    memset(&varvalc,0,sizeof(varvalc))                             ; // Clear REXX variable
    MAKERXSTRING(varname,TRACEVAR,strlen(TRACEVAR))                ; // Make name like RXMQNTRACE
    stem_to_string(ZERO, varname, "", varvalc, sizeof(varvalc))    ; // Get old-style trace variable
    strcat(varvalc," ")                                            ; // Add a blank to tail

    if ( strlen(varvalc) > 1)
      {
       if ( strstr(varvalc,"* "    ) != NULL) (*anchorptr)->tracebits |= ALL   ;
       if ( strstr(varvalc,"CONN " ) != NULL) (*anchorptr)->tracebits |= CONN  ;
       if ( strstr(varvalc,"DISC " ) != NULL) (*anchorptr)->tracebits |= DISC  ;
       if ( strstr(varvalc,"OPEN " ) != NULL) (*anchorptr)->tracebits |= OPEN  ;
       if ( strstr(varvalc,"CLOSE ") != NULL) (*anchorptr)->tracebits |= CLOSE ;
       if ( strstr(varvalc,"GET "  ) != NULL) (*anchorptr)->tracebits |= GET   ;
       if ( strstr(varvalc,"PUT "  ) != NULL) (*anchorptr)->tracebits |= PUT   ;
       if ( strstr(varvalc,"PUT1 " ) != NULL) (*anchorptr)->tracebits |= PUT1  ;
       if ( strstr(varvalc,"INQ "  ) != NULL) (*anchorptr)->tracebits |= INQ   ;
       if ( strstr(varvalc,"SET "  ) != NULL) (*anchorptr)->tracebits |= SET   ;
       if ( strstr(varvalc,"CMIT " ) != NULL) (*anchorptr)->tracebits |= CMIT  ;
       if ( strstr(varvalc,"BACK " ) != NULL) (*anchorptr)->tracebits |= BACK  ;
       if ( strstr(varvalc,"SUB "  ) != NULL) (*anchorptr)->tracebits |= SUB   ;
       if ( strstr(varvalc,"BRO "  ) != NULL) (*anchorptr)->tracebits |= BRO   ;
       if ( strstr(varvalc,"HXT "  ) != NULL) (*anchorptr)->tracebits |= HXT   ;
       if ( strstr(varvalc,"EVENT ") != NULL) (*anchorptr)->tracebits |= EVENT ;
       if ( strstr(varvalc,"TM "   ) != NULL) (*anchorptr)->tracebits |= TM    ;
       if ( strstr(varvalc,"COM "  ) != NULL) (*anchorptr)->tracebits |= COM   ;
       if ( strstr(varvalc,"MQV "  ) != NULL) (*anchorptr)->tracebits |= MQV   ;
       if ( strstr(varvalc,"INIT " ) != NULL) (*anchorptr)->tracebits |= INIT  ;
       if ( strstr(varvalc,"TERM " ) != NULL) (*anchorptr)->tracebits |= TERM  ;
      }

    memset(&varvalc,0,sizeof(varvalc))                             ; // Clear REXX variable
    MAKERXSTRING(varname,"RXMQTRACE",strlen("RXMQTRACE"))          ; // Make name like RXMQTRACE
    stem_to_string(ZERO, varname, "", varvalc, sizeof(varvalc))    ; // Get new-style trace variable
    strcat(varvalc," ")                                            ; // Add a blank to tail

    if ( strlen(varvalc) > 1)
      {
       if ( strstr(varvalc,"* "    ) != NULL) (*anchorptr)->tracebits |= ALL   ;
       if ( strstr(varvalc,"CONN " ) != NULL) (*anchorptr)->tracebits |= CONN  ;
       if ( strstr(varvalc,"DISC " ) != NULL) (*anchorptr)->tracebits |= DISC  ;
       if ( strstr(varvalc,"OPEN " ) != NULL) (*anchorptr)->tracebits |= OPEN  ;
       if ( strstr(varvalc,"CLOSE ") != NULL) (*anchorptr)->tracebits |= CLOSE ;
       if ( strstr(varvalc,"GET "  ) != NULL) (*anchorptr)->tracebits |= GET   ;
       if ( strstr(varvalc,"PUT "  ) != NULL) (*anchorptr)->tracebits |= PUT   ;
       if ( strstr(varvalc,"PUT1 " ) != NULL) (*anchorptr)->tracebits |= PUT1  ;
       if ( strstr(varvalc,"INQ "  ) != NULL) (*anchorptr)->tracebits |= INQ   ;
       if ( strstr(varvalc,"SET "  ) != NULL) (*anchorptr)->tracebits |= SET   ;
       if ( strstr(varvalc,"SUB "  ) != NULL) (*anchorptr)->tracebits |= SUB   ;
       if ( strstr(varvalc,"CMIT " ) != NULL) (*anchorptr)->tracebits |= CMIT  ;
       if ( strstr(varvalc,"BACK " ) != NULL) (*anchorptr)->tracebits |= BACK  ;
       if ( strstr(varvalc,"SUB "  ) != NULL) (*anchorptr)->tracebits |= SUB   ;
       if ( strstr(varvalc,"BRO "  ) != NULL) (*anchorptr)->tracebits |= BRO   ;
       if ( strstr(varvalc,"HXT "  ) != NULL) (*anchorptr)->tracebits |= HXT   ;
       if ( strstr(varvalc,"EVENT ") != NULL) (*anchorptr)->tracebits |= EVENT ;
       if ( strstr(varvalc,"TM "   ) != NULL) (*anchorptr)->tracebits |= TM    ;
       if ( strstr(varvalc,"COM "  ) != NULL) (*anchorptr)->tracebits |= COM   ;
       if ( strstr(varvalc,"MQV "  ) != NULL) (*anchorptr)->tracebits |= MQV   ;
       if ( strstr(varvalc,"INIT " ) != NULL) (*anchorptr)->tracebits |= INIT  ;
       if ( strstr(varvalc,"TERM " ) != NULL) (*anchorptr)->tracebits |= TERM  ;
      }
   }

 if (rc == 0 )
   {
    *traceidptr &= (*anchorptr)->tracebits          ;
    TRACE(*traceidptr, ("Function is <%s>\n",func)) ;
    TRACE(*traceidptr, ("traceid = %"PRIX32"\n",(uint32_t)*traceidptr));
    DUMPCB(*traceidptr, *anchorptr)                 ;
   }

 return rc;
} // End of set_envir function


//
// Return processing functions
//
//      set_return   : sets up the return variables
//
//          LASTRC    -> current operation Return Code
//          LASTCC    -> current operation MQ Completion Code
//          LASTAC    -> current operation MQ Reason     Code
//          LASTOP    -> current operation RXMQ Function
//          LASTMSG   -> current operation text message
//
//          The Return Code for the operation can be negative to
//              show that this interface has detected the error,
//              otherwise it will be the MQ Completion Code
//
//          The Message is in text format as follows:
//
//            Arg 1 : Return Code
//            Arg 2 : MQ Completion Code (or 0 if MQ not done)
//            Arg 2 : MQ Reason     Code (or 0 if MQ not done)
//            Arg 4 : RXMQ... function being run
//            Arg 5 : OK or an helpful error message
//            Arg 6 : Trace id of caller
//
//
void set_return ( const MQLONG   rc       //Function return Code
                , const MQLONG   cc       //MQ Completion Code
                , const MQLONG   ac       //MQ Reason Code
                , char         * op       //Function name
                , PRETMSG        pRetMsg  //Function message table
                , PRXSTRING      aretstr  //REXX Return String
                , MQULONG        traceid  //trace id of caller
                , char         * moremsg  //additional message
                )
{
 int                     i                ;
 RXSTRING                varname_new      ;  // Variable name
 RXSTRING                varname_old      ;  // Variable name

 TRACE(traceid,("Entering set_return\n")) ;

 MAKERXSTRING(varname_new, "RXMQ.", sizeof("RXMQ.")-1)  ;
 MAKERXSTRING(varname_old, PREFIX,  sizeof(PREFIX)-1)   ;

 stem_from_long  (traceid, NULL, varname_new, "LASTRC", rc);
 stem_from_long  (traceid, NULL, varname_old, "LASTRC", rc);

 stem_from_long  (traceid, NULL, varname_new, "LASTCC", cc);
 stem_from_long  (traceid, NULL, varname_old, "LASTCC", cc);

 stem_from_long  (traceid, NULL, varname_new, "LASTAC", ac);
 stem_from_long  (traceid, NULL, varname_old, "LASTAC", ac);

 stem_from_string(traceid, NULL, varname_new, "LASTOP", op, strlen(op)) ;
 stem_from_string(traceid, NULL, varname_old, "LASTOP", op, strlen(op)) ;

 if (rc < 0)
   {
    for (i = 0; ; i++)
      {
       if (pRetMsg[i].retcode == rc ) break ;
       if (pRetMsg[i].retcode == -99) break ;
      }
    sprintf(aretstr->strptr,  "%"PRId32" %"PRId32" %"PRId32" %-s %s",
        (uint32_t)rc, (uint32_t)cc, (uint32_t)ac, op, pRetMsg[i].retmsgc);
   }
 else sprintf(aretstr->strptr,"%"PRId32" %"PRIu32" %"PRIu32" %-s %s %s",
              (uint32_t)rc, (uint32_t)cc, (uint32_t)ac, op,
              ( (cc == MQCC_OK      ) ? "OK"      :
                (cc == MQCC_WARNING ) ? "WARNING" :
                (cc == MQCC_FAILED  ) ? "FAILED"  :
                                        "UNKNOWN" ),
              moremsg        ) ;

 aretstr->strlength  = strlen(aretstr->strptr)     ;
 stem_from_string(traceid, NULL, varname_new, "LASTMSG",
                  aretstr->strptr, aretstr->strlength);
 stem_from_string(traceid, NULL, varname_old, "LASTMSG",
                  aretstr->strptr, aretstr->strlength);

 TRACE(traceid,("Leaving set_return\n"))  ;
 TRACE(traceid,("Leaving %s\n",op))       ; // Leaving function
 fflush(NULL)                             ; // Required to flush trace buffer

 return ;
} // End of set_return function

//
// Functions that manipulate MQ objects and Rexx Variables
//
//           The make_??_from_stem  functions build the ??
//                                  MQ object by trying to reference
//                                  a stem variable (whose name ending
//                                  with a dot) whose name is supplied.
//                                  If the .xx part is present, then
//                                  the contents are copied into the
//                                  MQ Object.
//
//
//            The make_stem_from_?? functions work the other way around.
//                                  they are supplied with the name of
//                                  a stem variable (including the dot)
//                                  and build a Rexx representatation of
//                                  the MQ object by setting xx.yy
//                                  variables, where the yy is a
//                                  component of the MQ object. In addition,
//                                  (.)ZLIST is set to a list of the components
//                                  in the stem (without the leading dot).
//
//
//   The names of the .extensions are described in the functions.
//
//   Note that the make_stem_from_?? functions will ALWAYS build
//        all of the .extensions, whereas the make_??_from_stem
//        functions can cope with the omission of extensions.
//
//
// make_od_from_stem will return an Object Descriptor from the
//                   contents of a Stem Variable:
//
//                     .VER  -> Version
//                     .OT   -> ObjectType
//                     .ON   -> ObjectName
//                     .OQM  -> ObjectQMgrName
//                     .DQN  -> DynamicQueue
//                     .AUID -> AlternateUserid
//                     .RP   -> RecsPresent
//                     .KDC  -> KnownDestCount (output only)
//                     .UDC  -> UnknownDestCount (output only)
//                     .IDC  -> InvalidDestCount (output only)
//                     .ORO  -> ObjectRecOffset (not implemented)
//                     .RRO  -> ResponseRecOffset (not implemented)
//                     .ORP  -> ObjectRecPtr (not implemented)
//                     .RRP  -> ResponseRecPtr (not implemented)
//                     .ASID -> AlternateSecurityId (Windows only)
//                     .RQN  -> ResolvedQName (output only)
//                     .RQMN -> ResolvedQMgrName (output only)
//                     .OS   -> ObjectString
//                     .SS   -> SelectionString
//                     .ROS  -> ResObjectString
//                     .RT   -> ResolvedType
//

void make_od_from_stem ( MQULONG    traceid      // trace id of caller
                       , MQOD     * od           // target object descriptor
                       , RXSTRING   stem         // name of stem variable
                       )
{
 TRACE(traceid, ("Entering make_od_from_stem\n") ) ;

 memcpy(od, &od_default, sizeof(MQOD))         ;

//If the given variable is not a stem. variable, then take the
//   quicker option of assuming it's a Queue to open

 if ( stem.strptr[stem.strlength-1] != '.' )
   {
     memcpy(&od->ObjectName,
            stem.strptr,
            strlen(stem.strptr) ) ;
     DUMPCB(traceid,  od )            ;
     TRACE(traceid, ("Leaving make_od_from_stem\n") ) ;
     return                       ;
    }

 //The given variable is a stem. variable, so get its contents

 // Version 1 of MQOD
 stem_to_long  (traceid, stem, "VER" , &od->Version)                               ;
 stem_to_long  (traceid, stem, "OT"  , &od->ObjectType)                            ;
 stem_to_string(traceid, stem, "ON"  ,  od->ObjectName,          sizeof(MQCHAR48)) ;
 stem_to_string(traceid, stem, "OQM" ,  od->ObjectQMgrName,      sizeof(MQCHAR48)) ;
 stem_to_string(traceid, stem, "DQN" ,  od->DynamicQName,        sizeof(MQCHAR48)) ;
 stem_to_string(traceid, stem, "AUID",  od->AlternateUserId,     sizeof(MQCHAR12)) ;
 // Version 2 of MQOD
 stem_to_long  (traceid, stem, "RP"  , &od->RecsPresent)                           ;
 stem_to_long  (traceid, stem, "KDC" , &od->KnownDestCount)                        ;
 stem_to_long  (traceid, stem, "UDC" , &od->UnknownDestCount)                      ;
 stem_to_long  (traceid, stem, "IDC" , &od->InvalidDestCount)                      ;
 // Version 3 of MQOD
 stem_to_bytes (traceid, stem, "ASID",  od->AlternateSecurityId, sizeof(MQBYTE40)) ;
 stem_to_string(traceid, stem, "RQN" ,  od->ResolvedQName,       sizeof(MQCHAR48)) ;
 stem_to_string(traceid, stem, "RQMN",  od->ResolvedQMgrName,    sizeof(MQCHAR48)) ;
 // Version 4 of MQOD
 stem_to_strinv(traceid, stem, "OS"  , &od->ObjectString)                          ;
 stem_to_strinv(traceid, stem, "SS"  , &od->SelectionString)                       ;
 stem_to_strinv(traceid, stem, "ROS" , &od->ResObjectString)                       ;
 stem_to_long  (traceid, stem, "RT"  , &od->ResolvedType)                          ;

 DUMPCB(traceid, od )                             ;
 TRACE(traceid, ("Leaving make_od_from_stem\n") ) ;

 return ;
} // End of make_od_from_stem function

//
// make_stem_from_od will return an Object Descriptor as the
//                   contents of a Stem Variable:
//
//                     .VER  -> Version
//                     .OT   -> ObjectType
//                     .ON   -> ObjectName
//                     .OQM  -> ObjectQMgrName
//                     .DQN  -> DynamicQueue
//                     .AUID -> AlternateUserId
//                     .RP   -> RecsPresent
//                     .KDC  -> KnownDestCount
//                     .UDC  -> UnknownDestCount
//                     .IDC  -> InvalidDestCount
//                     .ORO  -> ObjectRecOffset (not implemented)
//                     .RRO  -> ResponseRecOffset (not implemented)
//                     .ORP  -> ObjectRecPtr (not implemented)
//                     .RRP  -> ResponseRecPtr (not implemented)
//                     .ASID -> AlternateSecurityId (Windows only)
//                     .RQN  -> ResolvedQName
//                     .RQMN -> ResolvedQMgrName
//                     .OS   -> ObjectString
//                     .SS   -> SelectionString
//                     .ROS  -> ResObjectString
//                     .RT   -> ResolvedType
//
//                     .ZLIST -> 'VER OT ON OQM DQN AUID
//                                RP
//                                KDC UDC IDC
//                                ASID
//                                RQN RQMN
//                                OS. SS. ROS. RT
//

void make_stem_from_od ( MQULONG    traceid      // trace id of caller
                       , MQOD     * od           // source object descriptor
                       , RXSTRING   stem         // name of stem variable
                       )
{
 char                        zlist[200]   ;  // Char version of .ZLIST
 zlist[0] = '\0'                          ;

 TRACE(traceid, ("Entering make_stem_from_od\n") ) ;
 DUMPCB(traceid, od )                              ;


 // Version 1 of MQOD
 stem_from_long  (traceid, zlist, stem, "VER" , od->Version)                               ;
 stem_from_long  (traceid, zlist, stem, "OT"  , od->ObjectType)                            ;
 stem_from_string(traceid, zlist, stem, "ON"  , od->ObjectName,          sizeof(MQCHAR48)) ;
 stem_from_string(traceid, zlist, stem, "OQM" , od->ObjectQMgrName,      sizeof(MQCHAR48)) ;
 stem_from_string(traceid, zlist, stem, "DQN" , od->DynamicQName,        sizeof(MQCHAR48)) ;
 stem_from_string(traceid, zlist, stem, "AUID", od->AlternateUserId,     sizeof(MQCHAR12)) ;
 // Version 2 of MQOD
 stem_from_long  (traceid, zlist, stem, "RP"  , od->RecsPresent)                           ;
 stem_from_long  (traceid, zlist, stem, "KDC" , od->KnownDestCount)                        ;
 stem_from_long  (traceid, zlist, stem, "UDC" , od->UnknownDestCount)                      ;
 stem_from_long  (traceid, zlist, stem, "IDC" , od->InvalidDestCount)                      ;
 // Version 3 of MQOD
 stem_from_bytes (traceid, zlist, stem, "ASID", od->AlternateSecurityId, sizeof(MQBYTE40)) ;
 stem_from_string(traceid, zlist, stem, "RQN" , od->ResolvedQName,       sizeof(MQCHAR48)) ;
 stem_from_string(traceid, zlist, stem, "RQMN", od->ResolvedQMgrName,    sizeof(MQCHAR48)) ;
 // Version 4 of MQOD
 stem_from_strinv(traceid, zlist, stem, "OS"  , od->ObjectString)                          ;
 stem_from_strinv(traceid, zlist, stem, "SS"  , od->SelectionString)                       ;
 stem_from_strinv(traceid, zlist, stem, "ROS" , od->ResObjectString)                       ;
 stem_from_long  (traceid, zlist, stem, "RT"  , od->ResolvedType)                          ;

 stem_from_string(traceid, zlist, stem, "ZLIST", zlist, strlen(zlist))                         ;

 TRACE(traceid, ("Leaving make_stem_from_od\n") ) ;

 return ;
} // End of make_stem_from_od function

//
// make_po_from_stem will return a Put Message Options Desc from the
//                   contents of a Stem Variable:
//
//                     .VER  -> Version
//                     .OPT  -> Options
//                     .TIME -> Timeout
//                     .CON  -> Context
//                     .KDC  -> KnownDestCount (output only)
//                     .UDC  -> UnKnownDestCount (output only)
//                     .IDC  -> InvalidDestCount (output only)
//                     .RQN  -> ResolvedQName (output only)
//                     .RQMN -> ResolvedQMgrName (output only)
//                     .RP   -> RecsPresent
//                     .PMRF -> PutMsgRecFields (not implemented)
//                     .PMRO -> PutMsgRecOffset (not implemented)
//                     .RRO  -> ResponseRecOffset (not implemented)
//                     .PMRP -> PutMsgRecPtr (not implemented)
//                     .RRP  -> ResponseRecPtr (not implemented)
//                     .OMH  -> OriginalMsgHandle
//                     .NMH  -> NewMsgHandle
//                     .ACT  -> Action
//                     .PL   -> PubLevel
//

void make_po_from_stem ( MQULONG    traceid      // trace id of caller
                       , MQPMO    * pmo          // target PMO
                       , RXSTRING   stem         // name of stem variable
                       )
{
 TRACE(traceid, ("Entering make_po_from_stem\n") ) ;

 memcpy(pmo, &pmo_default, sizeof(MQPMO))      ;

 // Version 1 of MQPMO
 stem_to_long  (traceid, stem, "VER" , &pmo->Version)                            ;
 stem_to_long  (traceid, stem, "OPT" , &pmo->Options)                            ;
 stem_to_long  (traceid, stem, "TIME", &pmo->Timeout)                            ;
 stem_to_long  (traceid, stem, "CON" , &pmo->Context)                            ;
 stem_to_long  (traceid, stem, "KDC" , &pmo->KnownDestCount)                     ;
 stem_to_long  (traceid, stem, "UDC" , &pmo->UnknownDestCount)                   ;
 stem_to_long  (traceid, stem, "IDC" , &pmo->InvalidDestCount)                   ;
 stem_to_string(traceid, stem, "RQN" ,  pmo->ResolvedQName,    sizeof(MQCHAR48)) ;
 stem_to_string(traceid, stem, "RQMN",  pmo->ResolvedQMgrName, sizeof(MQCHAR48)) ;
 // Version 2 of MQPMO
 stem_to_long  (traceid, stem, "RP"  , &pmo->RecsPresent)                        ;
 // Version 3 of MQPMO
 stem_to_int64 (traceid, stem, "OMH" , &pmo->OriginalMsgHandle)                  ;
 stem_to_int64 (traceid, stem, "NMH" , &pmo->NewMsgHandle)                       ;
 stem_to_long  (traceid, stem, "ACT" , &pmo->Action)                             ;
 stem_to_long  (traceid, stem, "PL"  , &pmo->PubLevel)                           ;

 DUMPCB(traceid, pmo ) ;
 TRACE(traceid, ("Leaving make_po_from_stem\n") ) ;

 return ;
} // End of make_po_from_stem function

//
// make_stem_from_po will return a Put Message Options Desc as the
//                   contents of a Stem Variable:
//
//                     .VER  -> Version
//                     .OPT  -> Options
//                     .TIME -> Timeout
//                     .CON  -> Context
//                     .KDC  -> KnownDestCount
//                     .UDC  -> UnKnownDestCount
//                     .IDC  -> InvalidDestCount
//                     .RQN  -> ResolvedQName
//                     .RQMN -> ResolvedQMgrName
//                     .RP   -> RecsPresent
//                     .PMRF -> PutMsgRecFields (not implemented)
//                     .PMRO -> PutMsgRecOffset (not implemented)
//                     .RRO  -> ResponseRecOffset (not implemented)
//                     .PMRP -> PutMsgRecPtr (not implemented)
//                     .RRP  -> ResponseRecPtr (not implemented)
//                     .OMH  -> OriginalMsgHandle
//                     .NMH  -> NewMsgHandle
//                     .ACT  -> Action
//                     .PL   -> PubLevel
//
//                     .ZLIST -> 'VER OPT CON CON TIME KDC UDC IDC RQN RQMN
//                                RP OMH NMH ACT PL'
//

void make_stem_from_po ( MQULONG    traceid      // trace id of caller
                       , MQPMO    * pmo          // source PMO
                       , RXSTRING   stem         // name of stem variable
                       )
{
 char                    zlist[200]   ;  // Char version of .ZLIST
 zlist[0] = '\0'                      ;

 TRACE(traceid, ("Entering make_stem_from_po\n") ) ;
 DUMPCB(traceid,  pmo )                            ;

 // Version 1 of MQPMO
 stem_from_long  (traceid, zlist, stem, "VER" , pmo->Version)                               ;
 stem_from_long  (traceid, zlist, stem, "OPT" , pmo->Options)                               ;
 stem_from_long  (traceid, zlist, stem, "TIME", pmo->Timeout)                               ;
 stem_from_long  (traceid, zlist, stem, "CON" , pmo->Context)                               ;
 stem_from_long  (traceid, zlist, stem, "KDC" , pmo->KnownDestCount)                        ;
 stem_from_long  (traceid, zlist, stem, "UDC" , pmo->UnknownDestCount)                      ;
 stem_from_long  (traceid, zlist, stem, "IDC" , pmo->InvalidDestCount)                      ;
 stem_from_string(traceid, zlist, stem, "RQN" , pmo->ResolvedQName,       sizeof(MQCHAR48)) ;
 stem_from_string(traceid, zlist, stem, "RQMN", pmo->ResolvedQMgrName,    sizeof(MQCHAR48)) ;
 // Version 2 of MQPMO
 stem_from_long  (traceid, zlist, stem, "RP"  , pmo->RecsPresent)                           ;
 // Version 3 of MQPMO
 stem_from_int64 (traceid, zlist, stem, "OMH" , pmo->OriginalMsgHandle)                     ;
 stem_from_int64 (traceid, zlist, stem, "NMH" , pmo->NewMsgHandle)                          ;
 stem_from_long  (traceid, zlist, stem, "ACT" , pmo->Action)                                ;
 stem_from_long  (traceid, zlist, stem, "PL"  , pmo->PubLevel)                              ;

 stem_from_string(traceid, zlist, stem, "ZLIST", zlist, strlen(zlist))                      ;

 TRACE(traceid,  ("Leaving make_stem_from_po\n") ) ;

 return ;
} // End of make_stem_from_po function

//
// make_go_from_stem will return a Get Message Options Desc from the
//                   contents of a Stem Variable:
//
//                     .VER   -> Version
//                     .OPT   -> Options
//                     .WAIT  -> WaitInterval
//                     .RQN   -> ResolvedQueueName (output only)
//                     .MOPT  -> MatchOptions
//                     .GS    -> GroupStatus (output only)
//                     .SS    -> SegmentStatus (output only)
//                     .SEG   -> Segmentation (output only)
//                     .MT    -> MsgToken
//                     .RL    -> ReturnedLength (output only)
//                     .MH    -> MsgHandle
//

void make_go_from_stem ( MQULONG    traceid      // trace id of caller
                       , MQGMO    * gmo          // target GMO
                       , RXSTRING   stem         // name of stem variable
                       )
{
 TRACE(traceid, ("Entering make_go_from_stem\n") ) ;

 memcpy(gmo, &gmo_default, sizeof(MQGMO))      ;

 // Version 1 of MQGMO
 stem_to_long  (traceid, stem, "VER" , &gmo->Version)                         ;
 stem_to_long  (traceid, stem, "OPT" , &gmo->Options)                         ;
 stem_to_long  (traceid, stem, "WAIT", &gmo->WaitInterval)                    ;
 stem_to_string(traceid, stem, "RQN" ,  gmo->ResolvedQName, sizeof(MQCHAR48)) ;
 // Version 2 of MQGMO
 stem_to_long  (traceid, stem, "MOPT", &gmo->MatchOptions)                    ;
 stem_to_char  (traceid, stem, "GS"  , &gmo->GroupStatus)                     ;
 stem_to_char  (traceid, stem, "SS"  , &gmo->SegmentStatus)                   ;
 stem_to_char  (traceid, stem, "SEG" , &gmo->Segmentation)                    ;
 // Version 3 of MQGMO
 stem_to_bytes (traceid, stem, "MT"  ,  gmo->MsgToken,      sizeof(MQBYTE16)) ;
 stem_to_long  (traceid, stem, "RL"  , &gmo->ReturnedLength)                  ;
 // Version 4 of MQGMO
 stem_to_int64 (traceid, stem, "MH"  , &gmo->MsgHandle)                       ;

 DUMPCB(traceid,  gmo ) ;
 TRACE(traceid, ("Leaving make_go_from_stem\n") ) ;

 return ;
} // End of make_go_from_stem function

//
// make_stem_from_go will return a Get Message Options Desc as the
//                   contents of a Stem Variable:
//
//                     .VER   -> Version
//                     .OPT   -> Options
//                     .WAIT  -> WaitInterval
//                     .RQN   -> ResolvedQueueName
//                     .MOPT  -> MatchOptions
//                     .GS    -> GroupStatus
//                     .SS    -> SegmentStatus
//                     .SEG   -> Segmentation
//                     .MT    -> sgToken
//                     .RL    -> ReturnedLength
//                     .MH    -> MsgHandle
//                     .ZLIST -> 'VER OPT WAIT RQN
//                                MOPT GS SS SEG
//                                MT RL MH'
//

void make_stem_from_go ( MQULONG    traceid      // trace id of caller
                       , MQGMO    * gmo          // source GMO
                       , RXSTRING   stem         // name of stem variable
                       )
{
 char                    zlist[200]   ;  // Char version of .ZLIST
 zlist[0] = '\0'                      ;

 TRACE(traceid, ("Entering make_stem_from_go\n") ) ;
 DUMPCB(traceid,  gmo )                            ;

// Version 1 of MQGMO
 stem_from_long  (traceid, zlist, stem, "VER" , gmo->Version)                         ;
 stem_from_long  (traceid, zlist, stem, "OPT" , gmo->Options)                         ;
 stem_from_long  (traceid, zlist, stem, "WAIT", gmo->WaitInterval)                    ;
 stem_from_string(traceid, zlist, stem, "RQN" , gmo->ResolvedQName, sizeof(MQCHAR48)) ;
// Version 2 of MQGMO
 stem_from_long  (traceid, zlist, stem, "MOPT", gmo->MatchOptions)                    ;
 stem_from_char  (traceid, zlist, stem, "GS"  , gmo->GroupStatus)                     ;
 stem_from_char  (traceid, zlist, stem, "SS"  , gmo->SegmentStatus)                   ;
 stem_from_char  (traceid, zlist, stem, "SEG" , gmo->Segmentation)                    ;
// Version 3 of MQGMO
 stem_from_bytes (traceid, zlist, stem, "MT"  , gmo->MsgToken,      sizeof(MQBYTE16)) ;
 stem_from_long  (traceid, zlist, stem, "RL"  , gmo->ReturnedLength)                  ;
// Version 4 of MQGMO
 stem_from_int64 (traceid, zlist, stem, "MH"  , gmo->MsgHandle)                       ;

 stem_from_string(traceid, zlist, stem, "ZLIST", zlist, strlen(zlist))                ;

 TRACE(traceid, ("Leaving make_stem_from_go\n") ) ;

 return ;
} // End of make_stem_from_go function

//
// make_md_from_stem will return a Put Message Options Desc from the
//                   contents of a Stem Variable:
//
//                     .VER   -> Version
//                     .REP   -> Report
//                     .MSG   -> MsgType
//                     .EXP   -> Expiry
//                     .FBK   -> Feedback
//                     .ENC   -> Encoding
//                     .CCSI  -> CodedCharSetId
//                     .FORM  -> Format
//                     .PRI   -> Priority
//                     .PER   -> Persistence
//                     .MSGID -> MsgId
//                     .CID   -> CorrelId
//                     .BC    -> BackoutCount
//                     .RTOQ  -> ReplyToQ
//                     .RTOQM -> ReplyToQMgr
//                     .UID   -> UserIdentifier
//                     .AT    -> AccountingToken
//                     .AID   -> ApplyIdentityData
//                     .PAT   -> PutApplType
//                     .PAN   -> PutApplName
//                     .PD    -> PutDate
//                     .PT    -> PutTime
//                     .AOD   -> ApplOriginData
//                     .GID   -> GroupId
//                     .MSN   -> MsgSeqNumber
//                     .OFF   -> Offset
//                     .MF    -> MsgFlags
//                     .OL    -> OriginalLength
//
//

void make_md_from_stem ( MQULONG    traceid      // trace id of caller
                       , MQMD2    * md           // target message descriptor
                       , RXSTRING   stem         // name of stem variable
                       )
{
 TRACE(traceid, ("Entering make_md_from_stem\n") ) ;

 memcpy(md, &md_default, sizeof(MQMD2))        ;
 md->Version = MQMD_VERSION_1                  ;

 // Version 1 of MQMD
 stem_to_long  (traceid, stem, "VER"  , &md->Version)                           ;
 stem_to_long  (traceid, stem, "REP"  , &md->Report)                            ;
 stem_to_long  (traceid, stem, "MSG"  , &md->MsgType)                           ;
 stem_to_long  (traceid, stem, "EXP"  , &md->Expiry)                            ;
 stem_to_long  (traceid, stem, "FBK"  , &md->Feedback)                          ;
 stem_to_long  (traceid, stem, "ENC"  , &md->Encoding)                          ;
 stem_to_long  (traceid, stem, "CCSI" , &md->CodedCharSetId)                    ;
 stem_to_string(traceid, stem, "FORM" ,  md->Format,           sizeof(MQCHAR8)) ;
 stem_to_long  (traceid, stem, "PRI"  , &md->Priority)                          ;
 stem_to_long  (traceid, stem, "PER"  , &md->Persistence)                       ;
 stem_to_bytes (traceid, stem, "MSGID",  md->MsgId,            sizeof(MQBYTE24));
 stem_to_bytes (traceid, stem, "CID"  ,  md->CorrelId,         sizeof(MQBYTE24));
 stem_to_long  (traceid, stem, "BC"   , &md->BackoutCount)                      ;
 stem_to_string(traceid, stem, "RTOQ" ,  md->ReplyToQ,         sizeof(MQCHAR48));
 stem_to_string(traceid, stem, "RTOQM",  md->ReplyToQMgr,      sizeof(MQCHAR48));
 stem_to_string(traceid, stem, "UID"  ,  md->UserIdentifier,   sizeof(MQCHAR12));
 stem_to_bytes (traceid, stem, "AT"   ,  md->AccountingToken,  sizeof(MQBYTE32));
 stem_to_string(traceid, stem, "AID"  ,  md->ApplIdentityData, sizeof(MQCHAR32));
 stem_to_long  (traceid, stem, "PAT"  , &md->PutApplType)                       ;
 stem_to_string(traceid, stem, "PAN"  ,  md->PutApplName,      sizeof(MQCHAR28));
 stem_to_string(traceid, stem, "PD"   ,  md->PutDate,          sizeof(MQCHAR8)) ;
 stem_to_string(traceid, stem, "PT"   ,  md->PutTime,          sizeof(MQCHAR8)) ;
 stem_to_string(traceid, stem, "AOD"  ,  md->ApplOriginData,   sizeof(MQCHAR4)) ;
 // Version 2 of MQMD
 stem_to_bytes (traceid, stem, "GID"  ,  md->GroupId,          sizeof(MQBYTE24));
 stem_to_long  (traceid, stem, "MSN"  , &md->MsgSeqNumber)                      ;
 stem_to_long  (traceid, stem, "OFF"  , &md->Offset)                            ;
 stem_to_long  (traceid, stem, "MF"   , &md->MsgFlags)                          ;
 stem_to_long  (traceid, stem, "OL"   , &md->OriginalLength)                    ;

 DUMPCB(traceid,  md ) ;
 TRACE(traceid, ("Leaving make_md_from_stem\n") ) ;

 return ;
} // End of make_md_from_stem function

//
// make_stem_from_md will return a Message Descriptor as the
//                   contents of a Stem Variable:
//
//                     .VER   -> Version
//                     .REP   -> Report
//                     .MSG   -> MsgType
//                     .EXP   -> Expiry
//                     .FBK   -> Feedback
//                     .ENC   -> Encoding
//                     .CCSI  -> CodedCharSetId
//                     .FORM  -> Format
//                     .PRI   -> Priority
//                     .PER   -> Persistence
//                     .MSGID -> MsgId
//                     .CID   -> CorrelId
//                     .BC    -> BackoutCount
//                     .RTOQ  -> ReplyToQ
//                     .RTOQM -> ReplyToQMgr
//                     .UID   -> UserIdentifier
//                     .AT    -> AccountingToken
//                     .AID   -> ApplyIdentityData
//                     .PAT   -> PutApplType
//                     .PAN   -> PutApplName
//                     .PD    -> PutDate
//                     .PT    -> PutTime
//                     .AOD   -> ApplOriginData
//                     .GID   -> GroupId
//                     .MSN   -> MsgSeqNumber
//                     .OFF   -> Offset
//                     .MF    -> MsgFlags
//                     .OL    -> OriginalLength
//
//
//                     .ZLIST -> 'VER REP MSG EXP FBK ENC CCSI FORM PRI PER MSGID CID BC
//                                RTOQ RTOQM UID AT AID PAT PAN PD PT AOD
//                                GID MSN OFF MF OL
//
//

void make_stem_from_md ( MQULONG    traceid      // trace id of caller
                       , MQMD2    * md           // source message descriptor
                       , RXSTRING   stem         // name of stem variable
                       )
{
 char                         zlist[200]   ;  // Char version of .ZLIST
 zlist[0] = '\0'                           ;

 TRACE(traceid, ("Entering make_stem_from_md\n") ) ;
 DUMPCB(traceid,  md )                             ;

// Version 1 of MQMD
 stem_from_long  (traceid, zlist, stem, "VER"  , md->Version)                               ;
 stem_from_long  (traceid, zlist, stem, "REP"  , md->Report)                                ;
 stem_from_long  (traceid, zlist, stem, "MSG"  , md->MsgType)                               ;
 stem_from_long  (traceid, zlist, stem, "EXP"  , md->Expiry)                                ;
 stem_from_long  (traceid, zlist, stem, "FBK"  , md->Feedback)                              ;
 stem_from_long  (traceid, zlist, stem, "ENC"  , md->Encoding)                              ;
 stem_from_long  (traceid, zlist, stem, "CCSI" , md->CodedCharSetId)                        ;
 stem_from_string(traceid, zlist, stem, "FORM" , md->Format,              sizeof(MQCHAR8))  ;
 stem_from_long  (traceid, zlist, stem, "PRI"  , md->Priority)                              ;
 stem_from_long  (traceid, zlist, stem, "PER"  , md->Persistence)                           ;
 stem_from_bytes (traceid, zlist, stem, "MSGID", md->MsgId,               sizeof(MQBYTE24)) ;
 stem_from_bytes (traceid, zlist, stem, "CID"  , md->CorrelId,            sizeof(MQBYTE24)) ;
 stem_from_long  (traceid, zlist, stem, "BC"   , md->BackoutCount)                          ;
 stem_from_string(traceid, zlist, stem, "RTOQ" , md->ReplyToQ,            sizeof(MQCHAR48)) ;
 stem_from_string(traceid, zlist, stem, "RTOQM", md->ReplyToQMgr,         sizeof(MQCHAR48)) ;
 stem_from_string(traceid, zlist, stem, "UID"  , md->UserIdentifier,      sizeof(MQCHAR12)) ;
 stem_from_bytes (traceid, zlist, stem, "AT"   , md->AccountingToken,     sizeof(MQBYTE32)) ;
 stem_from_string(traceid, zlist, stem, "AID"  , md->ApplIdentityData,    sizeof(MQCHAR32)) ;
 stem_from_long  (traceid, zlist, stem, "PAT"  , md->PutApplType)                           ;
 stem_from_string(traceid, zlist, stem, "PAN"  , md->PutApplName,         sizeof(MQCHAR28)) ;
 stem_from_string(traceid, zlist, stem, "PD"   , md->PutDate,             sizeof(MQCHAR8))  ;
 stem_from_string(traceid, zlist, stem, "PT"   , md->PutTime,             sizeof(MQCHAR8))  ;
 stem_from_string(traceid, zlist, stem, "AOD"  , md->ApplOriginData,      sizeof(MQCHAR4))  ;
// Version 2 of MQMD
 stem_from_bytes (traceid, zlist, stem, "GID"  , md->GroupId,             sizeof(MQBYTE24)) ;
 stem_from_long  (traceid, zlist, stem, "MSN"  , md->MsgSeqNumber)                          ;
 stem_from_long  (traceid, zlist, stem, "OFF"  , md->Offset)                                ;
 stem_from_long  (traceid, zlist, stem, "MF"   , md->MsgFlags)                              ;
 stem_from_long  (traceid, zlist, stem, "OL"   , md->OriginalLength)                        ;

 stem_from_string(traceid, zlist, stem, "ZLIST", zlist, strlen(zlist))                      ;

 TRACE(traceid, ("Leaving make_stem_from_md\n") ) ;

 return ;
} // End of make_stem_from_md function

//
// make_sd_from_stem will return a Subscription Descriptor from the
//                   contents of a Stem Variable:
//
//                     .VER  -> Version
//                     .OPT  -> Options
//                     .ON   -> ObjectName
//                     .AUID -> AlternateUserId
//                     .ASID -> AlternateSecurityId
//                     .SE   -> SubExpiry
//                     .OS   -> ObjectString
//                     .SN   -> SubName
//                     .SUD  -> SubUserData
//                     .SCID -> SubCorrelId
//                     .PP   -> PubPriority
//                     .PAT  -> PubAccountingToken
//                     .PAID -> PubAppIdentityData
//                     .SS   -> SelectionString
//                     .SL   -> SubLevel
//                     .ROS  -> ResObjectString
//

void make_sd_from_stem ( MQULONG    traceid      // trace id of caller
                       , MQSD     * subdesc      // target MQSD
                       , RXSTRING   stem         // name of stem variable
                       )
{
 TRACE(traceid, ("Entering make_sd_from_stem") ) ;

 memcpy(subdesc, &sd_default, sizeof(MQSD));

 //The given variable is a stem. variable, so get its contents

 // Version 1 of MQSD
 stem_to_long  (traceid, stem, "VER" , &subdesc->Version)                               ;
 stem_to_long  (traceid, stem, "OPT" , &subdesc->Options)                               ;
 stem_to_string(traceid, stem, "ON"  ,  subdesc->ObjectName,          sizeof(MQCHAR48)) ;
 stem_to_string(traceid, stem, "AUID",  subdesc->AlternateUserId,     sizeof(MQCHAR12)) ;
 stem_to_bytes (traceid, stem, "ASID",  subdesc->AlternateSecurityId, sizeof(MQBYTE40)) ;
 stem_to_long  (traceid, stem, "SE"  , &subdesc->SubExpiry)                             ;
 stem_to_strinv(traceid, stem, "OS"  , &subdesc->ObjectString)                          ;
 stem_to_strinv(traceid, stem, "SN"  , &subdesc->SubName)                               ;
 stem_to_strinv(traceid, stem, "SUD" , &subdesc->SubUserData)                           ;
 stem_to_bytes (traceid, stem, "SCID",  subdesc->SubCorrelId,         sizeof(MQBYTE24)) ;
 stem_to_long  (traceid, stem, "PP"  , &subdesc->PubPriority)                           ;
 stem_to_bytes (traceid, stem, "PAT" ,  subdesc->PubAccountingToken,  sizeof(MQBYTE32)) ;
 stem_to_string(traceid, stem, "PAID",  subdesc->PubApplIdentityData, sizeof(MQCHAR32)) ;
 stem_to_strinv(traceid, stem, "SS"  , &subdesc->SelectionString)                       ;
 stem_to_long  (traceid, stem, "SL"  , &subdesc->SubLevel)                              ;
 stem_to_strinv(traceid, stem, "ROS" , &subdesc->ResObjectString)                       ;

 DUMPCB(traceid, subdesc) ;
 TRACE(traceid, ("Leaving make_sd_from_stem\n") ) ;

 return ;
} // End of make_sd_from_stem function

//
// make_stem_from_sd will return a Subscription Descriptor as the
//                   contents of a Stem Variable:
//
//                     .VER  -> Version
//                     .OPT  -> Options
//                     .ON   -> ObjectName
//                     .AUID -> AlternateUserId
//                     .ASID -> AlternateSecurityId
//                     .SE   -> SubExpiry
//                     .OS   -> ObjectString
//                     .SN   -> SubName
//                     .SUD  -> SubUserData
//                     .SCID -> SubCorrelId
//                     .PP   -> PubPriority
//                     .PAT  -> PubAccountingToken
//                     .PAID -> PubAppIdentityData
//                     .SS   -> SelectionString
//                     .SL   -> SubLevel
//                     .ROS  -> ResObjectString
//
//                     .ZLIST -> 'VER OPT ON AUID ASID
//                                SE OS. SN. SUD. SCID
//                                PP PAT PAID
//                                SS. SL ROS.
//

void make_stem_from_sd ( MQULONG    traceid      // trace id of caller
                       , MQSD     * subdesc      // source MQSD
                       , RXSTRING   stem         // name of stem variable
                       )
{
 char                        zlist[200]   ;  // Char version of .ZLIST
 zlist[0] = '\0'                          ;

 TRACE(traceid, ("Entering make_stem_from_sd\n") ) ;
 DUMPCB(traceid, subdesc)                          ;

 // Version 1 of MQSD
 stem_from_long  (traceid, zlist, stem, "VER" , subdesc->Version)                               ;
 stem_from_long  (traceid, zlist, stem, "OPT" , subdesc->Options)                               ;
 stem_from_string(traceid, zlist, stem, "ON"  , subdesc->ObjectName,          sizeof(MQCHAR48)) ;
 stem_from_string(traceid, zlist, stem, "AUID", subdesc->AlternateUserId,     sizeof(MQCHAR12)) ;
 stem_from_bytes (traceid, zlist, stem, "ASID", subdesc->AlternateSecurityId, sizeof(MQBYTE40)) ;
 stem_from_long  (traceid, zlist, stem, "SE"  , subdesc->SubExpiry)                             ;
 stem_from_strinv(traceid, zlist, stem, "OS"  , subdesc->ObjectString)                          ;
 stem_from_strinv(traceid, zlist, stem, "SN"  , subdesc->SubName)                               ;
 stem_from_strinv(traceid, zlist, stem, "SUD" , subdesc->SubUserData)                           ;
 stem_from_bytes (traceid, zlist, stem, "SCID", subdesc->SubCorrelId,         sizeof(MQBYTE24)) ;
 stem_from_long  (traceid, zlist, stem, "PP"  , subdesc->PubPriority)                           ;
 stem_from_bytes (traceid, zlist, stem, "PAT" , subdesc->PubAccountingToken,  sizeof(MQBYTE32)) ;
 stem_from_string(traceid, zlist, stem, "PAID", subdesc->PubApplIdentityData, sizeof(MQCHAR32)) ;
 stem_from_strinv(traceid, zlist, stem, "SS"  , subdesc->SelectionString)                       ;
 stem_from_long  (traceid, zlist, stem, "SL"  , subdesc->SubLevel)                              ;
 stem_from_strinv(traceid, zlist, stem, "ROS" , subdesc->ResObjectString)                       ;

 stem_from_string(traceid, zlist, stem, "ZLIST", zlist, strlen(zlist))                         ;

 TRACE(traceid, ("Leaving make_stem_from_sd\n") ) ;

 return ;
} // End of make_stem_from_sd function


//
// Genuine Internal functions, not concerned with specific things
//

//
// setcons - sets up the MQ constants in the Rexx Variable Space
//           Traced by INIT setting
//

void setcons (MQULONG traceid)
 {

//
// Structure to define MQ literals, to be setup in Variable Space
//

 struct s_define_mq_ints
        {
          char           s_define_mq_ints_name[255] ; // Constant name
          int32_t        s_define_mq_ints_value     ; // Constant value
          uint32_t       s_define_mq_ints_type      ; // 0 -> number
                                                      // 1 -> MQRC_
                                                      // 2 -> not used
                                                      // 3 -> MQCC_
                                                      // 4 -> MQCA_
        } ;

 typedef struct s_define_mq_ints   t_define_mq_ints ;
 typedef        t_define_mq_ints * p_define_mq_ints ;

 struct s_define_mq_str
        {
          char           s_define_mq_str_name[32]   ;  // 32 seems to be max
          char           s_define_mq_str_value[9]   ;  // 8 is biggest so far
          MQULONG        s_define_mq_str_type       ;  // 0 -> char
        } ;

 typedef struct s_define_mq_str   t_define_mq_str  ;
 typedef        t_define_mq_str * p_define_mq_str  ;

 struct s_define_mq_byte
        {
          char           s_define_mq_byte_name[32]  ;  // 32 seems to be max
          MQBYTE         s_define_mq_byte_value[33] ;  // 33 is biggest so far
          MQULONG        s_define_mq_byte_size      ;
        } ;

 typedef struct s_define_mq_byte   t_define_mq_byte ;
 typedef        t_define_mq_byte * p_define_mq_byte ;

 struct s_define_mq_char
        {
          char           s_define_mq_char_name[32]  ;  // 32 seems to be max
          char           s_define_mq_char_value     ;  // 1-byte char
          MQULONG        s_define_mq_char_type      ;  // 0 -> char
        } ;

 typedef struct s_define_mq_char   t_define_mq_char ;
 typedef        t_define_mq_char * p_define_mq_char ;

 //
 //  Usual local variables
 //

 MQULONG                  i                ;  // Looper
 p_define_mq_ints         pi               ;  // Table pointer
 p_define_mq_str          ps               ;  // Table pointer
 p_define_mq_byte         pb               ;  // Table pointer
 p_define_mq_char         pc               ;  // Table pointer
 RXSTRING                 varname          ;  // Variable name
 RXSTRING                 varname_new      ;  // Variable name
 RXSTRING                 varname_old      ;  // Variable name
 char                     varvalc[100]     ;  // Char version of variable

//
// Array and Structure to define MQ literals, to be setup in Variable Space
// Constants are placed in separate header file for ease of use
//

#include <rxmqcons.h>

 TRACE(traceid, ("Entering setcons\n") ) ;

 //
 // initialize all the constants required.
 //
 // This is done by looping through the table of names and
 //      settings, invoking the RexxVariable interface to set up
 //      each literal.
 //
 // Additionally, if the constant represents Completion Code (MQCC_)
 //               then create an number->name entry in the
 //               RXMQ.CCMAP. stem variable
 //
 // Additionally, if the constant represents Reason Code (MQRC_)
 //               then create an number->name entry in the
 //               RXMQ.RCMAP. stem variable
 //
 // Additionally, if the constant represents Event (MQCA_,...)
 //               then create an number->name entry in the
 //               RXMQ.CAMAP. stem variable
 //

 MAKERXSTRING(varname_new, "RXMQ.", sizeof("RXMQ.")-1)  ;
 MAKERXSTRING(varname_old, PREFIX,  sizeof(PREFIX)-1)   ;

 for (i=0; ; i++)
   {
    pi = &define_mq_ints[i]                           ;
    if ( pi->s_define_mq_ints_name[0] == '?' )  break ;

    MAKERXSTRING(varname,
                 &(pi->s_define_mq_ints_name[0]),
                 strlen(pi->s_define_mq_ints_name))         ; // REXX variable name
    stem_from_long  (traceid, NULL, varname, "", pi->s_define_mq_ints_value);

    if ( pi->s_define_mq_ints_type == 1 )    //Only for MQRC_ stuff
     {
      sprintf(varvalc, "RCMAP.%"PRId32, pi->s_define_mq_ints_value)   ;
      stem_from_string(traceid, NULL, varname_new, varvalc,
                       &(pi->s_define_mq_ints_name[0]),
                       strlen(pi->s_define_mq_ints_name))   ; // like RXMQ.RCMAP.value
      stem_from_string(traceid, NULL, varname_old, varvalc,
                       &(pi->s_define_mq_ints_name[0]),
                       strlen(pi->s_define_mq_ints_name))   ; // like RXMQV.RCMAP.value
      }

    if ( pi->s_define_mq_ints_type == 3 )    //Only for MQCC_ stuff
     {
      sprintf(varvalc, "CCMAP.%"PRId32, pi->s_define_mq_ints_value)   ;
      stem_from_string(traceid, NULL, varname_new, varvalc,
                       &(pi->s_define_mq_ints_name[0]),
                       strlen(pi->s_define_mq_ints_name))   ; // like RXMQ.CCMAP.value
     }

    if ( pi->s_define_mq_ints_type == 4 )    //Only for Selector/Event Attributes
     {
      sprintf(varvalc, "CAMAP.%"PRId32, pi->s_define_mq_ints_value)   ;

      varname.strptr    = strstr(&(pi->s_define_mq_ints_name[0]), "_") + 1 ;
      varname.strlength = strlen(varname.strptr)            ;

      stem_from_string(traceid, NULL, varname_new, varvalc,
                       varname.strptr, varname.strlength)   ; // like RXMQ.CAMAP.value
     }
   } // End of Integer initializations Loop

 for (i=0; ; i++)
   {
    ps = &define_mq_str[i]                           ;
    if ( ps->s_define_mq_str_name[0] == '?' ) break  ;

    MAKERXSTRING(varname,
                 &(ps->s_define_mq_str_name[0]),
                 strlen(ps->s_define_mq_str_name))         ; // REXX variable name
    stem_from_string(traceid, NULL, varname, "",
                     &(ps->s_define_mq_str_value[0]),
                     strlen(ps->s_define_mq_str_value))    ;
   } // End of String initializations Loop

 for (i=0; ; i++)
   {
    pb = &define_mq_byte[i]                           ;
    if ( pb->s_define_mq_byte_name[0] == '?' ) break  ;

    MAKERXSTRING(varname,
                 &(pb->s_define_mq_byte_name[0]),
                 strlen(pb->s_define_mq_byte_name))       ; // REXX variable name
    stem_from_bytes(traceid, NULL, varname, "",
                    &(pb->s_define_mq_byte_value[0]),
                    pb->s_define_mq_byte_size)            ;
   } // End of Byte initializations Loop

 for (i=0; ; i++)
   {
    pc = &define_mq_char[i]                           ;
    if ( pc->s_define_mq_char_name[0] == '?' ) break  ;

    MAKERXSTRING(varname,
                 &(pc->s_define_mq_char_name[0]),
                 strlen(pc->s_define_mq_char_name))   ; // REXX variable name
    stem_from_char(traceid, NULL, varname, "",
                   (pc->s_define_mq_char_value))      ;
   } // End of Character initializations Loop

 TRACE(traceid, ("Leaving setcons\n") ) ;

 return ;

 } // End of setcons function

//
// Routine geteventname - converts the NUMBER of an Event into
//                        a name (for use as a Component Variable),
//                        which is defined event name like MQCA_...
//                        without MQCA_ prefix itself
//
//         If the PCF field number is unknown, then the
//            component is set to the character version of the
//            PCF event number. This only works for EVENT PCFs!!!!
//
//         Tracing is via the EVENT setting
//
//

void geteventname(char * output, const MQLONG pcfnum )
 {
  char                    varnamc[100] ;  // Char version of var name
  char                    varvalc[31]  ;  // Char version of var value
  SHVBLOCK                sv1          ;  // Var interface CB
  MQULONG                 sv1rc        ;  // Var interface RC

  // TRACE(EVENT, ("Entering geteventname for %"PRId32"\n",(int32_t)pcfnum) ) ;

  sv1.shvnext     = 0           ; // Fetch only one variable
  sv1.shvcode     = RXSHV_SYFET ; // Fetch operation
  sv1.shvret      = 0           ; // Zero out RC

  sprintf(varnamc,"RXMQ.CAMAP.%"PRId32,(int32_t)pcfnum) ; // Construct REXX variable name
  MAKERXSTRING(sv1.shvname,varnamc,strlen(varnamc))     ; // Construct REXX variable name structure
  sv1.shvnamelen  = sv1.shvname.strlength               ; // REXX variable name length

  sv1.shvvalue.strptr   = varvalc                     ; // Set pointer to value buffer for REXX
  sv1.shvvalue.strlength= 30                          ; // Set max value length for REXX
  sv1.shvvaluelen       = 30                          ; // Set max accepted value length

  sv1rc           = RexxVariablePool(&sv1)            ; // Call REXX variable interface

  varvalc[sv1.shvvalue.strlength] = 0                 ; // Ensure zero terminated

  if (   (sv1rc != RXSHV_OK)
      || (sv1.shvname.strlength == 0) )
    sprintf(varvalc,"%"PRId32,(int32_t)pcfnum)        ; // When something goes wrong

  // TRACE(EVENT, ("Converted %"PRId32" into %s\n",(int32_t)pcfnum,varvalc) )  ;

  strcat(output, varvalc)                             ; // Concatenate

  return ;

 } // End of geteventname function

//
// External Functions, callable from Rexx
//

//
// initialize the interface - RXMQINIT
//
//    For Windows, AIX, Linux this function installs the other functions of the DLL.
//    It MUST be called before any of the other Rexx/MQ functions are
//    available for usage.
//
//    This is done by a RxFuncAdd('RXMQINIT','RXMQx',RXMQINIT')
//         call to let Rexx know about the DLL, and then a
//         rcc = RXMQINIT() to call the initialization function.
//
//    The following things are done:
//
//        * Create Rexx variables for ALL known MQ Constants
//        * Create number->name translation variables for MQ ACs
//        * initialize the DLL Global variables
//            (including obtaining the Mutex for Conn/Open/Close processing)
//        * Make the RXMQ... functions known to Rexx
//
//
//
FTYPE  RXMQINIT  RXMQPARM
 {

 RXMQCB                 * anchor = 0       ;  // RXMQ Control Block
 int                      i                ;  // Looper
 int                      rc = 0           ;  // Function Return Code
 MQLONG                   mqrc = 0         ;  // MQ RC
 MQLONG                   mqac = 0         ;  // MQ AC
 MQULONG                  traceid = INIT   ;  // This function trace id

 RETMSG ReturnMsg[] = {
        { -99, "UNKNOWN FAILURE"}} ;
 //
 // Prepare Copyright message
 //
 char * moremsg = "WMQ Support Pac MA95. Version 2.0 "
                  "(C) Copyright IBM Corporation. 1997, 2012.\n";

 rc = set_envir (afuncname, &traceid, &anchor)    ;

 //
 // Now, initialize all the constants required by calling setcons.
 //
 // Additionally, if the constant represents Reason Code (MQRC_)
 //               then create an number->name entry in the
 //               RXMQ.RCMAP. stem variable
 //
 // Additionally, if the constant represents Completion Code (MQCC_)
 //               then create an number->name entry in the
 //               RXMQ.CCMAP. stem variable
 //
 // Additionally, if the constant represents Event Name (MQCA_,...)
 //               then create an number->name entry in the
 //               RXMQ.CAMAP. stem variable
 //
 if ( rc == 0 ) setcons(traceid) ;

#ifndef __MVS__
 //
 // Register the DLL functions in Windows/Linux/AIX
 //
 if ( rc == 0 )
   {
    for (i= 0; i< numfuncs ; i++ )
      {
       rc = RexxRegisterFunctionDll(        funcs[i],
                                      (PSZ) THISDLL,
                                            funcs[i]) ;
       TRACE(traceid, ("Registration of %s, rc = %d\n",funcs[i],rc) ) ;
      }
   }
#endif

//
// Set the LAST variables, and the function return string
//
 set_return(rc,mqrc,mqac,afuncname,ReturnMsg,aretstr,traceid,moremsg) ;

return 0 ;
 } // End of RXMQINIT function

//
// RXMQCONS will simply setup the MQ mappings in the Rexx Workspace.
//
//           This is useful in a thread based environment where RXMQINIT
//                has been done elsewhere (so setting the mappings elsewhere)
//                to act, essentially, as the initialization for the thread
//
FTYPE  RXMQCONS  RXMQPARM
 {
 RXMQCB                 * anchor = 0       ;  // RXMQ Control Block
 MQLONG                   rc = 0           ;  // Function Return Code
 MQLONG                   mqrc = 0         ;  // MQ RC
 MQLONG                   mqac = 0         ;  // MQ AC
 MQULONG                  traceid = INIT   ;  // This function trace id

 RETMSG ReturnMsg[] = {
        { -99, "UNKNOWN FAILURE"}} ;

 rc = set_envir (afuncname, &traceid, &anchor)    ;

 //
 // Just initialize all the constants etc. required by calling setcons.
 //
 if ( rc == 0 ) setcons(traceid) ;

//
// Set the LAST variables, and the function return string
//
 set_return(rc,mqrc,mqac,afuncname,ReturnMsg,aretstr,traceid,"") ;

return 0;
 } // End of RXMQCONS function

//
// Terminate the interface - RXMQTERM
//
//    For Windows, AIX, Linux only this function removes access to the functions of the DLL.
//         After calling, no other Rexx/MQ functions are available.
//
//    However, all the REXX name bindings are left in the
//         Workspace; this is done to provide a method of assembling
//         stem variable with the correct settings.
//
//    NO end of process actions are taken. If a prior Syncpoint is
//         not taken, or the Queue Closed, or the QM Disconnected
//         before this routine is invoked, then the usual MQ
//         End of Process action will occur at that time.
//
//
FTYPE  RXMQTERM  RXMQPARM
 {

 RXMQCB                 * anchor = 0       ;  // RXMQ Control Block
 int                      i                ;  // Looper
 int                      rc = 0           ;  // Function Return Code
 MQLONG                   mqrc = 0         ;  // MQ RC
 MQLONG                   mqac = 0         ;  // MQ AC
 MQULONG                  traceid = TERM   ;  // This function trace id

 RETMSG ReturnMsg[] = {
        { -99, "UNKNOWN FAILURE"}} ;

 rc = set_envir (afuncname, &traceid, &anchor)    ;

#ifndef __MVS__
//
// Deregister the DLL functions on distributed
//

 for (i= 0; i< numfuncs ; i++ )
   {
    rc = RexxDeregisterFunction(funcs[i]) ;
    TRACE(traceid, ("Deregistration of %s, rc = %d\n",funcs[i],rc) ) ;
   }
#endif


// Return a good return Code

// strcpy((char *)returnc,"0 0 0 RXMQTERM OK Rexx MQ Interface terminated." );
//
// Set the LAST variables, and the function return string
//
 set_return(rc,mqrc,mqac,afuncname,ReturnMsg,aretstr,traceid,"") ;

#ifdef __MVS__
 return 4;
#else
 return 0;
#endif
 } // End of RXMQTERM function

//
// Do a Connect MQCONN
//
//   Call:   rc = RXMQconn(qmname)
//
FTYPE  RXMQCONN  RXMQPARM
 {

 RXMQCB                * anchor = 0       ;  // RXMQ Control Block
 MQLONG                  rc = 0           ;  // Function Return Code
 MQLONG                  mqrc = 0         ;  // MQ RC
 MQLONG                  mqac = 0         ;  // MQ AC
 MQULONG                 traceid = CONN   ;  // This function trace id

 RETMSG ReturnMsg[] = {
        {  -1, "Bad number of parms" },
        {  -2, "Null QM name"},
        {  -3, "Zero length QM name"},
        {  -4, "QM name too long"},
        { -98, "Already Connected to a QM"},
        { -99, "UNKNOWN FAILURE"}} ;

 rc = set_envir (afuncname, &traceid, &anchor)    ;

//
// Check the parms
//
 if ( (rc == 0) && (aargc != 1 ) )              rc =  -1 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[0]) )     rc =  -2 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[0]) )  rc =  -3 ;
 if ( (rc == 0) && ( aargv[0].strlength >
                     MQ_Q_MGR_NAME_LENGTH ) )   rc =  -4 ;
//
// Is this thread already connected ?
//
 if ( (rc == 0) && ( anchor->QMh != 0) )        rc = -98 ;

//
// Now the parms are correct, get them
//
 if (rc == 0)
   {
    memcpy(anchor->QMname,aargv[0].strptr,aargv[0].strlength)        ;
    TRACE(traceid, ("Requested connection with %s\n",anchor->QMname) ) ;
   }

//
// Do the MQCONN
//
 if (rc == 0)
   {
    MQCONN ( anchor->QMname, &anchor->QMh, &mqrc, &mqac ) ;
    rc = mqrc                                             ;
    TRACE(traceid, ("MQCONN handle is %"PRIX32", QM is %s\n",
          (uint32_t)anchor->QMh,anchor->QMname) ) ;
   }

//
// Set the LAST variables, and the function return string
//
 set_return(rc,mqrc,mqac,afuncname,ReturnMsg,aretstr,traceid,"") ;

return 0;
 } // End of RXMQCONN function

//
// Do a Disconnect   MQDISC
//
//   Call:   rc = RXMQdisc()
//
FTYPE  RXMQDISC  RXMQPARM
 {

 RXMQCB                * anchor = 0       ;  // RXMQ Control Block
 MQLONG                  rc = 0           ;  // Function Return Code
 MQLONG                  mqrc = 0         ;  // MQ RC
 MQLONG                  mqac = 0         ;  // MQ AC
 MQULONG                 traceid = DISC   ;  // This function trace id

 RETMSG ReturnMsg[] = {
        {  -1, "Bad number of parms" },
        { -98, "Not Connected to a QM"},
        { -99, "UNKNOWN FAILURE"}} ;

 rc = set_envir (afuncname, &traceid, &anchor)    ;

//
// Check the parms
//
 if ( (rc == 0) && ( aargc != 0 ) )        rc =  -1 ;
 if ( (rc == 0) && ( anchor->QMh == 0 ) )  rc = -98 ;

//
// Do the MQDISC
//
 if (rc == 0)
   {
    TRACE(traceid, ("Disconnecting from QM %s\n",anchor->QMname) ) ;
    MQDISC ( &anchor->QMh, &mqrc, &mqac ) ;
    rc = mqrc                             ;
    memset (anchor, 0, sizeof(RXMQCB))   ;
    memcpy (&anchor->StrucId, RXMQeyecatcher, sizeof(MQCHAR4)) ;
   }
//
// Set the LAST variables, and the function return string
//
 set_return(rc,mqrc,mqac,afuncname,ReturnMsg,aretstr,traceid,"") ;

return 0;
 } // End of RXMQDISC function


//
// Do an Open   MQOPEN
//
//   Call:   rc = RXMQopen(iMQOD,opts,handle,oMQOD)
//
FTYPE  RXMQOPEN  RXMQPARM
 {

 RXMQCB                 * anchor = 0      ;  // RXMQ Control Block
 int                     i                ;  // Looper
 MQLONG                  rc = 0           ;  // Function Return Code
 MQLONG                  mqrc = 0         ;  // MQ RC
 MQLONG                  mqac = 0         ;  // MQ AC
 MQULONG                 traceid = OPEN   ;  // This function trace id

 RXSTRING                RXi_od           ;  // Stem Var Obj Desc Input
 RXSTRING                RXo_od           ;  // Stem Var Obj Desc Output
 RXSTRING                RX_opt           ;  // Open     Options
 RXSTRING                RX_handle        ;  //      Var Obj Handle

 MQOD                    od               ;  //MQ object desc
 MQLONG                  options    =  0  ;  //MQ open options
 int                     theobj     = -1  ;  //gmqo object to use

 RETMSG ReturnMsg[] = {
        {  -1, "Bad number of parms" },
        {  -2, "Null Input OD/Qname"},
        {  -3, "Zero length input OD/Qname"},
        {  -4, "Null options"},
        {  -5, "Zero length options"},
        {  -6, "Null handle name"},
        {  -7, "Zero length handle name"},
        {  -8, "Null Output OD"},
        {  -9, "Zero length output OD"},
        { -10, "No available Q objects"},
        { -98, "Not connected to a QM"},
        { -99, "UNKNOWN FAILURE"}} ;

 rc = set_envir (afuncname, &traceid, &anchor)    ;

//
// Check the parms
//
 if ( (rc == 0) && (aargc != 4 ) )             rc =  -1 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[0]) )    rc =  -2 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[0]) ) rc =  -3 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[1]) )    rc =  -4 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[1]) ) rc =  -5 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[2]) )    rc =  -6 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[2]) ) rc =  -7 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[3]) )    rc =  -8 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[3]) ) rc =  -9 ;
 if ( (rc == 0) && ( anchor->QMh == 0 ) )      rc = -98 ;

//
// Now the parms are correct, get them
//
 if (rc == 0)
    {
      memcpy(&RXi_od    ,&aargv[0],sizeof(RXi_od))    ;
      memcpy(&RX_opt    ,&aargv[1],sizeof(RX_opt  ))  ;
      memcpy(&RX_handle ,&aargv[2],sizeof(RX_handle)) ;
      memcpy(&RXo_od    ,&aargv[3],sizeof(RXo_od))    ;

      TRACE(traceid, ("RXi_od = %.*s\n",   (int)RXi_od.strlength,   RXi_od.strptr) )     ;
      TRACE(traceid, ("RX_opt = %.*s\n",   (int)RX_opt.strlength,   RX_opt.strptr) )     ;
      TRACE(traceid, ("RX_handle = %.*s\n",(int)RX_handle.strlength,RX_handle.strptr) )  ;
      TRACE(traceid, ("RXo_od = %.*s\n",   (int)RXo_od.strlength,   RXo_od.strptr) )     ;

      make_od_from_stem(traceid,&od,RXi_od)    ;
      parm_to_ulong(RX_opt, &options)          ;
    }


//
// Select the handle slot
//
 if (rc == 0)
   {
    for ( i=MINQS ; i <= MAXQS ; i++ )
      {
       if ( anchor->Qh[i] == 0 )
         {
          theobj = i ;
          TRACE(traceid, ("Selected Qh Object [%d]\n",theobj) ) ;
          break      ;
         }
      }

    if ( (theobj == -1) ) rc = -10 ;
   }

//
// Do the MQOPEN on the obtained Queue object
//
 if (rc == 0)
   {
    TRACE(traceid, ("Calling MQOPEN with options = %"PRId32"\n",(int32_t)options) ) ;
    MQOPEN ( anchor->QMh, &od, options, &anchor->Qh[theobj], &mqrc, &mqac ) ;
    rc = mqrc ;

    if ( anchor->Qh[theobj] != 0 )   //If the Open worked,
      {                              //then .....
       stem_from_long(traceid, NULL, RX_handle, ""  , theobj) ;
       make_stem_from_od(traceid,&od,RXo_od) ; //and update the OD
      }
   }

//
// Set the LAST variables, and the function return string
//
 set_return(rc,mqrc,mqac,afuncname,ReturnMsg,aretstr,traceid,"") ;

return 0;
 } // End of RXMQOPEN function

//
// Do a Close   MQCLOSE
//
//   Call:   rc = RXMQclos(handle,opts)
//
FTYPE  RXMQCLOS  RXMQPARM
 {

 RXMQCB                 * anchor = 0      ;  // RXMQ Control Block
 MQLONG                  rc = 0           ;  // Function Return Code
 MQLONG                  mqrc = 0         ;  // MQ RC
 MQLONG                  mqac = 0         ;  // MQ AC
 MQULONG                 traceid = CLOSE  ;  // This function trace id

 RXSTRING                RX_opts          ;  // Data     Options
 RXSTRING                RX_handle        ;  // Data     Obj Handle

 MQLONG                  options  = 0     ;  //MQ close options
 MQLONG                  handle   = 0     ;  //MQ object number

 RETMSG ReturnMsg[] = {
        {  -1, "Bad number of parms" },
        {  -2, "Null handle"},
        {  -3, "Zero length handle"},
        {  -4, "Null options"},
        {  -5, "Zero length options"},
        {  -6, "Handle out of range"},
        {  -7, "Invalid handle"},
        { -98, "Not connected to a QM"},
        { -99, "UNKNOWN FAILURE"}} ;

 rc = set_envir (afuncname, &traceid, &anchor)    ;

//
// Check the parms
//
 if ( (rc == 0) && (aargc != 2 ) )             rc =  -1 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[0]) )    rc =  -2 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[0]) ) rc =  -3 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[1]) )    rc =  -4 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[1]) ) rc =  -5 ;
 if ( (rc == 0) && ( anchor->QMh == 0 ) )      rc = -98 ;

//
// Now the parms are correct, get them
//
 if (rc == 0)
   {
    memcpy(&RX_handle,&aargv[0],sizeof(RX_handle))   ;
    memcpy(&RX_opts  ,&aargv[1],sizeof(RX_opts  ))   ;

    TRACE(traceid, ("RX_opts = %.*s\n",  (int)RX_opts.strlength,  RX_opts.strptr) )   ;
    TRACE(traceid, ("RX_handle = %.*s\n",(int)RX_handle.strlength,RX_handle.strptr) ) ;

    parm_to_ulong(RX_opts,   &options)                 ;
    parm_to_ulong(RX_handle, &handle)                  ;
   }

//
// See if the handle is valid (ie: the gmqo to use)
//
 if ( (rc == 0) && ( ( handle > MAXQS ) || ( handle <= 0 ) ) ) rc = -6 ;
 if ( (rc == 0) && ( anchor->Qh[handle] == 0 ) )               rc = -7 ;

//
// Now close the object
//
 if (rc == 0)
   {
    TRACE(traceid, ("Closing object handle [%"PRIu32"]\n",(uint32_t)handle) ) ;
    MQCLOSE ( anchor->QMh, &anchor->Qh[handle], options, &mqrc, &mqac )       ;
    rc = mqrc ;
    if ( mqac == 0 )                  //If the Close worked,
      {                               //then .....
       anchor->Qh[handle] = 0 ;       //loose the MQ object
      }
   }

//
// Set the LAST variables, and the function return string
//
 set_return(rc,mqrc,mqac,afuncname,ReturnMsg,aretstr,traceid,"") ;

return 0;
 } // End of RXMQCLOS function

//
// Do a Syncpoint    MQCMIT
//
//   Call:   rc = RXMQcmit()
//
FTYPE  RXMQCMIT  RXMQPARM
 {

 RXMQCB                 * anchor = 0      ;  // RXMQ Control Block
 MQLONG                  rc = 0           ;  // Function Return Code
 MQLONG                  mqrc = 0         ;  // MQ RC
 MQLONG                  mqac = 0         ;  // MQ AC
 MQULONG                 traceid = CMIT   ;  // This function trace id

 RETMSG ReturnMsg[] = {
        {  -1, "Bad number of parms" },
        { -98, "Not connected to a QM"},
        { -99, "UNKNOWN FAILURE"}} ;

 rc = set_envir (afuncname, &traceid, &anchor)    ;

//
// Check the parms
//
 if ( (rc == 0) && (aargc != 0 ) )             rc =  -1 ;
 if ( (rc == 0) && ( anchor->QMh == 0 ) )      rc = -98 ;

//
// Do the Actual Syncpoint on the required Queue Manager
//        This also forces it for all others on the thread
//
 if (rc == 0)
   {
    MQCMIT ( anchor->QMh, &mqrc, &mqac ) ;
    rc   = mqrc                          ;
   }

//
// Set the LAST variables, and the function return string
//
 set_return(rc,mqrc,mqac,afuncname,ReturnMsg,aretstr,traceid,"") ;

return 0;
 } // End of RXMQCMIT function

//
// Do a Rollback     MQBACK
//
//   Call:   rc = RXMQback()
//
FTYPE  RXMQBACK  RXMQPARM
 {

 RXMQCB                * anchor = 0       ;  // RXMQ Control Block
 MQLONG                  rc = 0           ;  // Function Return Code
 MQLONG                  mqrc = 0         ;  // MQ RC
 MQLONG                  mqac = 0         ;  // MQ AC
 MQULONG                 traceid = BACK   ;  // This function trace id

 RETMSG ReturnMsg[] = {
        {  -1, "Bad number of parms" },
        { -98, "Not connected to a QM"},
        { -99, "UNKNOWN FAILURE"}} ;

 rc = set_envir (afuncname, &traceid, &anchor)    ;

//
// Check the parms
//
 if ( (rc == 0) && (aargc != 0 ) )             rc =  -1 ;
 if ( (rc == 0) && (anchor->QMh == 0 ) )       rc = -98 ;

//
// Do the Actual Backout on the required Queue Manager
//        This also forces it for all others on the thread
//
 if (rc == 0)
   {
    MQBACK ( anchor->QMh, &mqrc, &mqac ) ;
    rc   = mqrc                          ;
   }

//
// Set the LAST variables, and the function return string
//
 set_return(rc,mqrc,mqac,afuncname,ReturnMsg,aretstr,traceid,"") ;

 return 0;
 } // End of RXMQBACK function

//
// Do a Put     MQPUT
//
//   Call:   rc = RXMQput(handle, data,
//                        input_msgdesc, output_msgdesc,
//                        input_pmo, output_pmo)
//
FTYPE  RXMQPUT  RXMQPARM
 {

 RXMQCB                * anchor = 0       ;  // RXMQ Control Block
 MQLONG                  rc   = 0         ;  // Function Return Code
 MQLONG                  mqrc = 0         ;  // MQ RC
 MQLONG                  mqac = 0         ;  // MQ AC
 MQULONG                 traceid = PUT    ;  // This function trace id

 RXSTRING                RX_handle        ;  // Obj Handle
 RXSTRING                RX_data          ;  // Variable Data
 RXSTRING                RXi_md           ;  // Variable Input  Msg Desc
 RXSTRING                RXo_md           ;  // Variable Output Msg Desc
 RXSTRING                RXi_pmo          ;  // Variable Input  PMO
 RXSTRING                RXo_pmo          ;  // Variable Output PMO

 MQLONG                  handle           ;  //MQ object number
 MQMD2                   od               ;  //MQ Message descriptor
 MQPMO                   pmo              ;  //MQ Put Message options
 MQLONG                  data0 = 0        ;  // Variable Data len
 void                 *  data  = 0        ;  //-> Data buffer
 int                     datalen          ;  //   Data length

 RETMSG ReturnMsg[] = {
        {  -1, "Bad number of parms" },
        {  -2, "Null handle"},
        {  -3, "Zero data handle"},
        {  -4, "Null data stem var"},
        {  -5, "Zero data stem var"},
        {  -6, "Null input MsgDesc"},
        {  -7, "Zero length input MsgDesc"},
        {  -8, "Null output MsgDesc"},
        {  -9, "Zero length output MsgDesc"},
        { -10, "Null input PMO"},
        { -11, "Zero length input PMO"},
        { -12, "Null output PMO"},
        { -13, "Zero length output PMO"},
        { -14, "Handle out of range"},
        { -15, "Invalid handle"},
        { -16, "malloc failure, check reason code"},
        { -17, "Zero length input data buffer"},
        { -18, "Data length is not equal to specified value"},
        { -19, "Context handle out of range"},
        { -20, "Invalid Context handle"},
        { -98, "Not connected to a QM"},
        { -99, "UNKNOWN FAILURE"}} ;

 rc = set_envir (afuncname, &traceid, &anchor)    ;

//
// Check the parms
//
 if ( (rc == 0) && (aargc != 6 ) )             rc =  -1 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[0]) )    rc =  -2 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[0]) ) rc =  -3 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[1]) )    rc =  -4 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[1]) ) rc =  -5 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[2]) )    rc =  -6 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[2]) ) rc =  -7 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[3]) )    rc =  -8 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[3]) ) rc =  -9 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[4]) )    rc = -10 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[4]) ) rc = -11 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[5]) )    rc = -12 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[5]) ) rc = -13 ;
 if ( (rc == 0) && ( anchor->QMh == 0 ) )      rc = -98 ;

//
// Now the parms are correct, get them
//
 if (rc == 0)
   {
    memcpy(&RX_handle,&aargv[0],sizeof(RX_handle)) ;
    memcpy(&RX_data,  &aargv[1],sizeof(RX_data))   ;
    memcpy(&RXi_md,   &aargv[2],sizeof(RXi_md))    ;
    memcpy(&RXo_md,   &aargv[3],sizeof(RXo_md))    ;
    memcpy(&RXi_pmo,  &aargv[4],sizeof(RXi_pmo))   ;
    memcpy(&RXo_pmo,  &aargv[5],sizeof(RXo_pmo))   ;

    TRACE(traceid, ("RX_handle = %.*s\n",(int)RX_handle.strlength,RX_handle.strptr) ) ;
    TRACE(traceid, ("RX_data = %.*s\n",  (int)RX_data.strlength,  RX_data.strptr)   ) ;
    TRACE(traceid, ("RXi_md = %.*s\n",   (int)RXi_md.strlength,   RXi_md.strptr)    ) ;
    TRACE(traceid, ("RXo_md = %.*s\n",   (int)RXo_md.strlength,   RXo_md.strptr)    ) ;
    TRACE(traceid, ("RXi_pmo = %.*s\n",  (int)RXi_pmo.strlength,  RXi_pmo.strptr)   ) ;
    TRACE(traceid, ("RXo_pmo = %.*s\n",  (int)RXo_pmo.strlength,  RXo_pmo.strptr)   ) ;

    make_md_from_stem(traceid,&od, RXi_md )          ;
    make_po_from_stem(traceid,&pmo , RXi_pmo )       ;

    parm_to_ulong(RX_handle, &handle)                ;
    stem_to_long(traceid, RX_data, "0" , &data0)     ;
   }

//
// Now check the input Stem variable to see that there is
//     some valid data to obtain
//
 if ( (rc == 0) && ( data0 <= 0 ) ) rc = -17    ;

//
// Allocate data buffer to store stem.1 variable data.
//
 if ( rc == 0 )
   {
    TRACE(traceid, ("Doing malloc for %"PRId32" bytes\n",(int32_t)data0) ) ;
    data = malloc(data0)                                                   ;
    if ( data == NULL )
      {
       mqac = errno                                                        ;
       TRACE(traceid, ("malloc rc = %"PRId32"\n",(int32_t)mqac) )          ;
       rc = -16                                                            ;
      }
    else
    {
     datalen = stem_to_data(traceid, RX_data, "1", (MQBYTE *)data, data0)  ;
     TRACE(traceid, ("Length of data received = %d\n",datalen) )           ;
    }
   }

//
// Now check if stem.0 specifies the same value as stem.1 length.
// If not, don't know what to do.
//
 if ( (rc == 0) && ( datalen != data0 ) ) rc = -18;

//
// See if the output queue handle is valid
//
 if ( (rc == 0) && ( ( handle > MAXQS ) || ( handle <= 0 ) ) )   rc = -14 ;
 if ( (rc == 0) && ( anchor->Qh[handle] == 0 ) )                 rc = -15 ;


//
// If Context is specified (for MQPMO_PASS_ALL_CONTEXT)
// see if the input queue handle is valid
//
 if ( (rc == 0) && ( pmo.Context != 0 ) )
   {
    if ( ( pmo.Context > MAXQS ) || ( pmo.Context < 0 ) )  rc = -19 ;
    if ( (rc == 0) && (anchor->Qh[pmo.Context] == 0 ) )    rc = -20 ;
    if   (rc == 0) pmo.Context = anchor->Qh[pmo.Context];
    }

//
// Now put the data to the queue
//
 if (rc == 0)
   {
    TRACE(traceid, ("PUT Maxdatalen = %"PRId32"\n",(int32_t)data0) )                ;
    MQPUT ( anchor->QMh, anchor->Qh[handle], &od, &pmo, data0, data, &mqrc, &mqac ) ;
    rc = mqrc ;
    TRACE(traceid, ("PUT rc = %"PRId32", ac = %"PRId32"\n",(int32_t)mqrc, (int32_t)mqac) ) ;

    make_stem_from_md(traceid,&od, RXo_md )    ;   //Set the return Variables
    make_stem_from_po(traceid,&pmo , RXo_pmo ) ;
   }

//
// Free data buffer for stem.1 variable data, if allocated.
//
 if ( data != 0 )
   {
    TRACE(traceid, ("Free area\n") ) ;
    free(data) ;
   }

//
// Set the LAST variables, and the function return string
//
 set_return(rc,mqrc,mqac,afuncname,ReturnMsg,aretstr,traceid,"") ;

 return 0;
 } // End of RXMQPUT function

//
// Do a put    MQPUT1
//
//   Call:   rc = RXMQPUT1(input1_objdesc, output1_objdesc, data,
//                         input1_msgdesc,output1_msgdesc,
//                         input1_pmo,output1_pmo)
//        or
//           rc = RXMQPUT1(queue_name, output1_objdesc, data,
//                         input1_msgdesc,output1_msgdesc,
//                         input1_pmo,output1_pmo)
//
FTYPE RXMQPUT1  RXMQPARM
 {
 RXMQCB                * anchor = 0       ;  // RXMQ Control Block
 MQLONG                  rc = 0           ;  // Function Return Code
 MQLONG                  mqrc = 0         ;  // MQ RC
 MQLONG                  mqac = 0         ;  // MQ AC
 MQULONG                 traceid = PUT1   ;  // This function trace id

 RXSTRING                RXi_od           ;  // Stem Var Obj Desc Input
 RXSTRING                RXo_od           ;  // Stem Var Obj Desc Output
 RXSTRING                RX_data          ;  // Variable Data
 RXSTRING                RXi_md           ;  // Variable Input1  Msg Desc
 RXSTRING                RXo_md           ;  // Variable Output1 Msg Desc
 RXSTRING                RXi_pmo          ;  // Variable Input1  PMO
 RXSTRING                RXo_pmo          ;  // Variable Output1 PMO

 MQOD                    od               ;  // MQ object descriptor
 MQMD2                   md               ;  // MQ Message descriptor
 MQPMO                   pmo              ;  // MQ put1 Message options
 void                 *  data  = 0        ;  //-> Data buffer
 MQLONG                  data0 = 0        ;  // Variable Data len
 int                     datalen          ;  //   Data length

 RETMSG ReturnMsg[] = {
        {  -1, "Bad number of parms" },
        {  -2, "Null input OD/Qname"},
        {  -3, "Zero length input OD/Qname"},
        {  -4, "Null output OD"},
        {  -5, "Zero length output OD"},
        {  -6, "Null data stem var"},
        {  -7, "Zero data stem var"},
        {  -8, "Null input MsgDesc"},
        {  -9, "Zero length input MsgDesc"},
        { -10, "Null output MsgDesc"},
        { -11, "Zero length output MsgDesc"},
        { -12, "Null input PMO"},
        { -13, "Zero length input PMO"},
        { -14, "Null output PMO"},
        { -15, "Zero length output PMO"},
        { -17, "malloc failure, check reason code"},
        { -18, "Zero length input data buffer"},
        { -19, "Data length is not equal to specified value"},
        { -20, "Context handle out of range"},
        { -21, "Invalid context handle"},
        { -98, "Not connected to a QM"},
        { -99, "UNKNOWN FAILURE"}} ;

 rc = set_envir (afuncname, &traceid, &anchor)    ;

//
// Check the parms
//
 if ( (rc == 0) && (aargc != 7 ) )             rc =  -1 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[0]) )    rc =  -2 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[0]) ) rc =  -3 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[1]) )    rc =  -4 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[1]) ) rc =  -5 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[2]) )    rc =  -6 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[2]) ) rc =  -7 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[3]) )    rc =  -8 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[3]) ) rc =  -9 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[4]) )    rc = -10 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[4]) ) rc = -11 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[5]) )    rc = -12 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[5]) ) rc = -13 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[6]) )    rc = -14 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[6]) ) rc = -15 ;
 if ( (rc == 0) && ( anchor->QMh == 0 ) )      rc = -98 ;

//
// Now the parms are correct, get them
//
 if (rc == 0)
   {
    memcpy(&RXi_od, &aargv[0],sizeof(RXi_od))  ;
    memcpy(&RXo_od, &aargv[1],sizeof(RXo_od))  ;
    memcpy(&RX_data,&aargv[2],sizeof(RX_data)) ;
    memcpy(&RXi_md, &aargv[3],sizeof(RXi_md))  ;
    memcpy(&RXo_md, &aargv[4],sizeof(RXo_md))  ;
    memcpy(&RXi_pmo,&aargv[5],sizeof(RXi_pmo)) ;
    memcpy(&RXo_pmo,&aargv[6],sizeof(RXo_pmo)) ;

    TRACE(traceid, ("RXi_od = %.*s\n", (int)RXi_od.strlength, RXi_od.strptr)  ) ;
    TRACE(traceid, ("RXo_od = %.*s\n", (int)RXo_od.strlength, RXo_od.strptr)  ) ;
    TRACE(traceid, ("RX_data = %.*s\n",(int)RX_data.strlength,RX_data.strptr) ) ;
    TRACE(traceid, ("RXi_md = %.*s\n", (int)RXi_md.strlength, RXi_md.strptr)  ) ;
    TRACE(traceid, ("RXo_md = %.*s\n", (int)RXo_md.strlength, RXo_md.strptr)  ) ;
    TRACE(traceid, ("RXi_pmo = %.*s\n",(int)RXi_pmo.strlength,RXi_pmo.strptr) ) ;
    TRACE(traceid, ("RXo_pmo = %.*s\n",(int)RXo_pmo.strlength,RXo_pmo.strptr) ) ;

    make_od_from_stem(traceid,&od, RXi_od            ) ;
    make_md_from_stem(traceid,&md, RXi_md            ) ;
    make_po_from_stem(traceid,&pmo,RXi_pmo           ) ;
    stem_to_long     (traceid,RX_data, "0" , &data0  ) ;
   }

//
// Now check the input Stem variable to see that there is
//     some valid data to obtain
//
 if ( (rc == 0) && ( data0 <= 0 ) )  rc = -18 ;

//
// Allocate data buffer to store stem.1 variable data.
//
 if ( rc == 0 )
   {
    TRACE(traceid, ("Doing malloc for %"PRId32" bytes\n",(int32_t)data0) )  ;
    data = malloc(data0)                                                    ;
    if ( data == NULL )
      {
       mqac = errno                                                         ;
       TRACE(traceid, ("malloc rc = %"PRId32"\n",(int32_t)mqac) )           ;
       rc = -17                                                             ;
      }
    else
      {
       datalen = stem_to_data(traceid, RX_data, "1", (MQBYTE *)data, data0) ;
       TRACE(traceid, ("Length of data received = %d\n",datalen) )          ;
      }
   }

//
// Now check if stem.0 specifies the same value as stem.1 length.
// If not, don't know what to do.
//
 if ( (rc == 0) && ( datalen != data0 ) ) rc = -19 ;

 //
 // If Context is specified (for MQPMO_PASS_ALL_CONTEXT)
 // see if the input queue handle is valid (ie: the gmqo to use)
 //
  if ( (rc == 0) && ( pmo.Context != 0 ) )
    {
     if ( ( pmo.Context > MAXQS ) || ( pmo.Context < 0 ) ) rc = -20 ;
     if ( (rc == 0) & (anchor->Qh[pmo.Context] == 0 ) )    rc = -21 ;
     if (rc == 0) pmo.Context = anchor->Qh[pmo.Context]             ;
    }

//
// Now execute PUT1
//
 if (rc == 0)
   {
    TRACE(traceid, ("PUT1 Maxdatalen is %"PRId32"\n",(int32_t)data0) ) ;
    MQPUT1 ( anchor->QMh, &od, &md, &pmo, data0, data, &mqrc, &mqac )  ;
    rc   = mqrc ;
    TRACE(traceid, ("PUT1 rc = %"PRId32", ac = %"PRId32"\n",
          (int32_t)mqrc,(int32_t)mqac) )                               ;

    make_stem_from_md(traceid,&md,  RXo_md  ) ;   //Set the return Variables
    make_stem_from_po(traceid,&pmo, RXo_pmo ) ;
    make_stem_from_od(traceid,&od,  RXo_od  ) ;
   }


//
// Free data buffer for stem.1 variable data, if allocated.
//
 if ( data != 0 )
   {
    TRACE(traceid, ("Free area\n") ) ;
    free(data) ;
   }

//
// Set the LAST variables, and the function return string
//
 set_return(rc,mqrc,mqac,afuncname,ReturnMsg,aretstr,traceid,"") ;

return 0;
} // End of RXMQPUT1 function

//
// Do a Get     MQGET
//
//   Call:   rc = RXMQget(handle, data,
//                        input_msgdesc,output_msgdesc,
//                        input_gmo,output_gmo)
//
FTYPE  RXMQGET  RXMQPARM
 {

 RXMQCB                * anchor = 0       ;  // RXMQ Control Block
 MQLONG                  rc = 0           ;  // Function Return Code
 MQLONG                  mqrc = 0         ;  // MQ RC
 MQLONG                  mqac = 0         ;  // MQ AC
 MQULONG                 traceid = GET    ;  // This function trace id

 RXSTRING                RX_handle        ;  // Obj Handle
 RXSTRING                RX_data          ;  // Variable Data
 RXSTRING                RXi_md           ;  // Variable Input  Msg Desc
 RXSTRING                RXo_md           ;  // Variable Output Msg Desc
 RXSTRING                RXi_gmo          ;  // Variable Input  GMO
 RXSTRING                RXo_gmo          ;  // Variable Output GMO

 MQLONG                  handle    = 0    ;  // MQ object number
 MQMD2                   md               ;  // MQ Message descriptor
 MQGMO                   gmo              ;  // MQ Get Message options
 void                 *  data      = 0    ;  //-> Data buffer
 MQLONG                  data0     = 0    ;  //   Data length max
 MQLONG                  datalen   = 0    ;  //   Data length actual

 RETMSG ReturnMsg[] = {
        {  -1, "Bad number of parms" },
        {  -2, "Null handle"},
        {  -3, "Zero data handle"},
        {  -4, "Null data stem var"},
        {  -5, "Zero data stem var"},
        {  -6, "Null input MsgDesc"},
        {  -7, "Zero length input MsgDesc"},
        {  -8, "Null output MsgDesc"},
        {  -9, "Zero length output MsgDesc"},
        { -10, "Null input GMO"},
        { -11, "Zero length input GMO"},
        { -12, "Null output GMO"},
        { -13, "Zero length output GMO"},
        { -14, "Handle out of range"},
        { -15, "Invalid handle"},
        { -16, "malloc failure, check reason code"},
        { -17, "Zero length input data buffer"},
        { -98, "Not connected to a QM"},
        { -99, "UNKNOWN FAILURE"}} ;

 rc = set_envir (afuncname, &traceid, &anchor)    ;

//
// Check the parms
//

 if ( (rc == 0) && (aargc != 6 ) )             rc =  -1 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[0]) )    rc =  -2 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[0]) ) rc =  -3 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[1]) )    rc =  -4 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[1]) ) rc =  -5 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[2]) )    rc =  -6 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[2]) ) rc =  -7 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[3]) )    rc =  -8 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[3]) ) rc =  -9 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[4]) )    rc = -10 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[4]) ) rc = -11 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[5]) )    rc = -12 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[5]) ) rc = -13 ;
 if ( (rc == 0) && ( anchor->QMh == 0 ) )      rc = -98 ;

//
// Now the parms are correct, get them
//

 if (rc == 0)
   {
    memcpy(&RX_handle,&aargv[0],sizeof(RX_handle)) ;
    memcpy(&RX_data,  &aargv[1],sizeof(RX_data))   ;
    memcpy(&RXi_md,   &aargv[2],sizeof(RXi_md))    ;
    memcpy(&RXo_md,   &aargv[3],sizeof(RXo_md))    ;
    memcpy(&RXi_gmo,  &aargv[4],sizeof(RXi_gmo))   ;
    memcpy(&RXo_gmo,  &aargv[5],sizeof(RXo_gmo))   ;

    TRACE(traceid, ("RX_handle = %.*s\n",(int)RX_handle.strlength,RX_handle.strptr) ) ;
    TRACE(traceid, ("RX_data = %.*s\n",  (int)RX_data.strlength,  RX_data.strptr)   ) ;
    TRACE(traceid, ("RXi_md = %.*s\n",   (int)RXi_md.strlength,   RXi_md.strptr)    ) ;
    TRACE(traceid, ("RXo_md = %.*s\n",   (int)RXo_md.strlength,   RXo_md.strptr)    ) ;
    TRACE(traceid, ("RXi_gmo = %.*s\n",  (int)RXi_gmo.strlength,  RXi_gmo.strptr)   ) ;
    TRACE(traceid, ("RXo_gmo = %.*s\n",  (int)RXo_gmo.strlength,  RXo_gmo.strptr)   ) ;


    make_md_from_stem(traceid,&md, RXi_md )      ;
    make_go_from_stem(traceid,&gmo , RXi_gmo )   ;
    parm_to_ulong(RX_handle, &handle)            ;
    stem_to_long(traceid, RX_data, "0" , &data0) ;
   }

//
// Now check the input Stem variable to see that there is
//     some valid data to obtain
//
  if ( (rc == 0) && ( data0 <= 0 ) ) rc = -17 ;

//
// See if the handle is valid
//
 if ( (rc == 0) && ( ( handle > MAXQS ) || ( handle <= 0 ) ) ) rc = -14 ;
 if ( (rc == 0) && ( anchor->Qh[handle] == 0 ) )               rc = -15 ;

//
// Now GETMAIN the buffer to receive the data records
//
 if (( rc == 0) && ( data0 != 0 ) )
   {
    TRACE(traceid, ("Doing malloc for %"PRId32" bytes\n",(int32_t)data0) ) ;
    data = malloc(data0)                                                   ;
    if ( data == NULL )
    {
     mqac = errno                                                          ;
     TRACE(traceid, ("malloc rc = %"PRId32"\n",(int32_t)mqac) )            ;
     rc = -16                                                              ;
    }
   }

//
// Now get the data from the queue
//
 if (rc == 0)
   {
    TRACE(traceid, ("GET Maxdatalen = %"PRId32"\n",(int32_t)data0) )                          ;
    MQGET ( anchor->QMh, anchor->Qh[handle], &md, &gmo, data0, data, &datalen, &mqrc, &mqac ) ;
    rc = mqrc                                                                                 ;
    TRACE(traceid, ("GET rc = %"PRId32", ac = %"PRId32", datalen = %"PRId32"\n",
          (int32_t)mqrc,(int32_t)mqac,(int32_t)datalen) )                                     ;

    make_stem_from_md(traceid,&md,  RXo_md  ) ;  //Set the return Variables
    make_stem_from_go(traceid,&gmo, RXo_gmo ) ;

    stem_from_long (traceid, NULL, RX_data, "0" , datalen)                 ;
    if (datalen > data0) datalen = data0                                   ;
    stem_from_bytes(traceid, NULL, RX_data, "1" , (MQBYTE *)data, datalen) ;
   }

//
// Free data buffer for stem.1 variable data, if allocated.
//
 if ( data != 0 )
   {
    TRACE(traceid, ("Free area\n") ) ;
    free(data) ;
   }

//
// Set the LAST variables, and the function return string
//
 set_return(rc,mqrc,mqac,afuncname,ReturnMsg,aretstr,traceid,"") ;

return 0;
 } // End of RXMQGET function

//
// Do an inquire MQINQ
//
//   Call:   rc = RXMQinq(handle, attribute, attrsetting )
//
//           NB: only 1 attribute at a time!
//
//
FTYPE  RXMQINQ  RXMQPARM
 {

 RXMQCB         * anchor = 0           ;  // RXMQ Control Block
 MQLONG           rc = 0               ;  // Function Return Code
 MQLONG           mqrc = 0             ;  // MQ RC
 MQLONG           mqac = 0             ;  // MQ AC
 MQULONG          traceid = INQ        ;  // This function trace id

 RXSTRING         RX_handle            ;  // Data     Obj Handle
 RXSTRING         RX_attr              ;  // Variable Input  Attr
 RXSTRING         RX_value             ;  // Variable Output Attr

 MQLONG           handle       = 0     ;  // MQ object number
 MQLONG           attrib       = 0     ;  // object attribute

 MQLONG           inqselcount  = 1     ;  //INQ - number of sels
 MQLONG           inqselicount = 1     ;  //INQ - number of int sels
 MQLONG           inqints      = 0     ;  //INQ - integer return
 MQLONG           inqcharlen   = 600   ;  //INQ - char return length
 char             inqchars[601]        ;  //INQ - char return

 RETMSG ReturnMsg[] = {
        {  -1, "Bad number of parms" },
        {  -2, "Null handle"},
        {  -3, "Zero data handle"},
        {  -4, "Null data input attr"},
        {  -5, "Zero data input attr"},
        {  -6, "Null output attr"},
        {  -7, "Zero length output attr"},
        {  -8, "No attribute supplied"},
        {  -9, "Attribute out of valid range"},
        { -10, "Handle out of range"},
        { -11, "Invalid handle"},
        { -98, "Not connected to a QM"},
        { -99, "UNKNOWN FAILURE"}} ;

 rc = set_envir (afuncname, &traceid, &anchor)    ;

//
// Check the parms
//
 if ( (rc == 0) && (aargc != 3 ) )             rc =  -1 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[0]) )    rc =  -2 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[0]) ) rc =  -3 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[1]) )    rc =  -4 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[1]) ) rc =  -5 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[2]) )    rc =  -6 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[2]) ) rc =  -7 ;
 if ( (rc == 0) && ( anchor->QMh == 0 ) )      rc = -98 ;

//
// Now the parms are correct, get them
//
 if (rc == 0)
   {
    memcpy(&RX_handle,&aargv[0],sizeof(RX_handle)) ;
    memcpy(&RX_attr,  &aargv[1],sizeof(RX_attr))   ;
    memcpy(&RX_value, &aargv[2],sizeof(RX_value))  ;

    TRACE(traceid, ("RX_handle = %.*s\n",(int)RX_handle.strlength,RX_handle.strptr) ) ;
    TRACE(traceid, ("RX_attr = %.*s\n",  (int)RX_attr.strlength,  RX_attr.strptr)   ) ;
    TRACE(traceid, ("RX_value = %.*s\n", (int)RX_value.strlength, RX_value.strptr)  ) ;

    parm_to_ulong(RX_handle, &handle) ;
    parm_to_ulong(RX_attr,   &attrib) ;

    if (attrib == 0) rc = -8          ;

    if ( !( ( (attrib >= MQIA_FIRST) && (attrib <= MQIA_LAST  ) )
         || ( (attrib >= MQCA_FIRST) && (attrib <= MQCA_LAST  ) ) ) ) rc = -9;
   }
//
// See if the handle is valid
//
 if ( (rc == 0) && ( ( handle > MAXQS ) || ( handle <= 0 ) ) ) rc = -10 ;
 if ( (rc == 0) && ( anchor->Qh[handle] == 0 ) )               rc = -11 ;

//
// Format up either a character or an integer area
//
 if (rc == 0)
   {
    memset(inqchars,0,sizeof(inqchars)) ;
    if ( (attrib >= MQCA_FIRST  ) && (attrib <= MQCA_LAST  ) )
      inqselicount = 0 ;
    else
      inqcharlen   = 0 ;
   } //End of Attribute setup

//
// Now do the Inquiry and return the setting
//
 if (rc == 0)
   {
    TRACE(traceid, ("Attr = %"PRId32", IntSelNum = %"PRId32", CharAttrLen = %"PRId32"\n",
                    (int32_t)attrib,(int32_t)inqselicount,(int32_t)inqcharlen) );
    MQINQ ( anchor->QMh , anchor->Qh[handle],
            inqselcount , &attrib  ,
            inqselicount, &inqints ,
            inqcharlen  , inqchars ,
            &mqrc, &mqac ) ;
    rc   = mqrc ;
    TRACE(traceid, ("INQ rc = %"PRId32", ac = %"PRId32", Intval = %"PRId32", Charval = %s\n",
          (int32_t)mqrc,(int32_t)mqac,(int32_t)inqints,inqchars) ) ;

    if ( (attrib >= MQCA_FIRST  ) && (attrib <= MQCA_LAST) )
      stem_from_string(traceid, NULL, RX_value, "", inqchars, inqcharlen) ;
    else
      stem_from_long  (traceid, NULL, RX_value, "", inqints) ;
   }

//
// Set the LAST variables, and the function return string
//
 set_return(rc,mqrc,mqac,afuncname,ReturnMsg,aretstr,traceid,"") ;

return 0;
 } // End of RXMQINQ function

//
// Do a set      MQSET
//
//   Call:   rc = RXMQset(handle, attribute, atttrsetting )
//
//           NB: only 1 attribute at a time!
//
//
FTYPE  RXMQSET  RXMQPARM
 {

 RXMQCB         * anchor = 0           ;  // RXMQ Control Block
 MQLONG           rc = 0               ;  // Function Return Code
 MQLONG           mqrc = 0             ;  // MQ RC
 MQLONG           mqac = 0             ;  // MQ AC
 MQULONG          traceid = SET        ;  // This function trace id

 RXSTRING         RX_handle            ;  // Data     Obj Handle
 RXSTRING         RX_attr              ;  // Variable Input  Attr
 RXSTRING         RX_value             ;  // Variable Set    Attr

 MQLONG           handle       = 0     ;  //MQ object number
 MQLONG           attrib       = 0     ;  // object attribute

 MQLONG           setselcount  = 1     ;  //SET - number of sels
 MQLONG           setselicount = 1     ;  //SET - number of int sels
 MQLONG           setints      = 0     ;  //SET - integer return
 MQLONG           setcharlen   = 600   ;  //SET - char return length
 char             setchars[601]        ;  //SET - char return

 RETMSG ReturnMsg[] = {
        {  -1, "Bad number of parms" },
        {  -2, "Null handle"},
        {  -3, "Zero data handle"},
        {  -4, "Null data attribute"},
        {  -5, "Zero data attribute"},
        {  -6, "Null setting"},
        {  -7, "Zero length setting"},
        {  -8, "No attribute supplied"},
        {  -9, "Attribute out of valid range"},
        { -10, "Handle out of range"},
        { -11, "Invalid handle"},
        { -98, "Not connected to a QM"},
        { -99, "UNKNOWN FAILURE"}} ;

 rc = set_envir (afuncname, &traceid, &anchor)    ;

//
// Check the parms
//

 if ( (rc == 0) && (aargc != 3 ) )             rc =  -1 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[0]) )    rc =  -2 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[0]) ) rc =  -3 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[1]) )    rc =  -4 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[1]) ) rc =  -5 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[2]) )    rc =  -6 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[2]) ) rc =  -7 ;
 if ( (rc == 0) && ( anchor->QMh == 0 ) )      rc = -98 ;

//
// Now the parms are correct, get them
//
 if (rc == 0)
   {
    memcpy(&RX_handle,&aargv[0],sizeof(RX_handle)) ;
    memcpy(&RX_attr,  &aargv[1],sizeof(RX_attr))   ;
    memcpy(&RX_value, &aargv[2],sizeof(RX_value))  ;

    TRACE(traceid, ("RX_handle = %.*s\n",(int)RX_handle.strlength,RX_handle.strptr) ) ;
    TRACE(traceid, ("RX_attr = %.*s\n",  (int)RX_attr.strlength,  RX_attr.strptr)   ) ;
    TRACE(traceid, ("RX_value = %.*s\n", (int)RX_value.strlength, RX_value.strptr)  ) ;

    parm_to_ulong(RX_handle, &handle) ;
    parm_to_ulong(RX_attr,   &attrib) ;

    if (attrib == 0) rc = -8 ;

    if ( !( ( (attrib >= MQIA_FIRST) && (attrib <= MQIA_LAST  ) )
         || ( (attrib >= MQCA_FIRST) && (attrib <= MQCA_LAST  ) ) ) ) rc = -9;
   }

//
// See if the handle is valid (ie: the gmqo to use)
//
 if ( (rc == 0) && ( ( handle > MAXQS ) || ( handle <= 0 ) ) ) rc = -10 ;
 if ( (rc == 0) && ( anchor->Qh[handle] == 0 ) )               rc = -11 ;

//
//
// Format up either a character or an integer area
//
 if (rc == 0)
   {
    memset(setchars,' ',sizeof(setchars))                     ;
    if ( (attrib >= MQCA_FIRST  ) && (attrib <= MQCA_LAST  ) )
      {
       setselicount = 0                                       ;
       memcpy(setchars,RX_value.strptr,RX_value.strlength)    ;

      }      //End of Char Attr processing
    else      //Int attr processing
      {
       setcharlen   = 0                                       ;
       parm_to_ulong(RX_value,  &setints)                     ;
      }      //End of Int Attr processing

   } //End of Attribute building

//
// Now do the Setting
//
 if (rc == 0)
   {
    TRACE(traceid, ("Attr = %"PRId32", IntSelNum = %"PRId32", IntSelVal = %"PRId32", CharAtrLen = %"PRId32", CharSetVal = %s\n",
          (int32_t)attrib,(int32_t)setselicount,(int32_t)setints,(int32_t)setcharlen,setchars) ) ;
    MQSET ( anchor->QMh , anchor->Qh[handle],
            setselcount , &attrib   ,
            setselicount, &setints  ,
            setcharlen  , setchars  ,
            &mqrc, &mqac ) ;
    rc   = mqrc ;
    TRACE(traceid, ("MQSET rc = %"PRId32", ac = %"PRId32"\n",(int32_t)mqrc,(int32_t)mqac) ) ;
   }

//
// Set the LAST variables, and the function return string
//
 set_return(rc,mqrc,mqac,afuncname,ReturnMsg,aretstr,traceid,"") ;

return 0;
 } // End of RXMQSET function

//
// Do a Subscribe MQSUB
//
//   Call:   rc = RXMQsub(isdesc,handle,osdesc)
//
FTYPE  RXMQSUB  RXMQPARM
 {

 RXMQCB                * anchor = 0       ;  // RXMQ Control Block
 MQULONG                 i                ;  // Looper
 MQLONG                  rc = 0           ;  // Function Return Code
 MQLONG                  mqrc = 0         ;  // MQ RC
 MQLONG                  mqac = 0         ;  // MQ AC
 MQULONG                 traceid = SUB    ;  // This function trace id

 RXSTRING                RX_handle        ;  //      Var Obj Handle
 RXSTRING                RXi_sd           ;  // Stem Var Sub Desc Input
 RXSTRING                RXo_sd           ;  // Stem Var Sub Desc Output

 MQSD                    sd               ;  // MQ subscription desc
 int                     theobj     = -1  ;  // gmqo object to use
 MQHOBJ                  sh               ;  // Subscription handle

 RETMSG ReturnMsg[] = {
        {  -1, "Bad number of parms" },
        {  -2, "Null input SD"},
        {  -3, "Zero length input SD"},
        {  -6, "Null handle name"},
        {  -7, "Zero length handle name"},
        {  -8, "Null Output SD"},
        {  -9, "Zero length Output SD"},
        { -10, "No available objects"},
        { -98, "Not connected to a QM"},
        { -99, "UNKNOWN FAILURE"}} ;

 rc = set_envir (afuncname, &traceid, &anchor)    ;

//
// Check the parms
//
 if ( (rc == 0) && (aargc != 3 ) )             rc =  -1 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[0]) )    rc =  -2 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[0]) ) rc =  -3 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[1]) )    rc =  -6 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[1]) ) rc =  -7 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[2]) )    rc =  -8 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[2]) ) rc =  -9 ;
 if ( (rc == 0) && ( anchor->QMh == 0 ) )      rc = -98 ;

//
// Now the parms are correct, get them
//
 if (rc == 0)
    {
      memcpy(&RXi_sd ,   &aargv[0],sizeof(RXi_sd))    ;
      memcpy(&RX_handle ,&aargv[1],sizeof(RX_handle)) ;
      memcpy(&RXo_sd ,   &aargv[2],sizeof(RXo_sd))    ;

      TRACE(traceid, ("RXi_sd = %.*s\n",   (int)RXi_sd.strlength,   RXi_sd.strptr)    )  ;
      TRACE(traceid, ("RX_handle = %.*s\n",(int)RX_handle.strlength,RX_handle.strptr) )  ;
      TRACE(traceid, ("RXo_sd = %.*s\n",   (int)RXo_sd.strlength,   RXo_sd.strptr)    )  ;

      make_sd_from_stem(traceid,&sd,RXi_sd)           ;
    }

//
// Select the handle
//
 if (rc == 0)
   {
    for ( i=MINQS ; i <= MAXQS ; i++ )
      {
       if ( anchor->Qh[i] == 0 )
         {
          theobj = i ;
          TRACE(traceid, ("Selected Object [%d]\n",theobj) ) ;
          break      ;
         }
      }
    if ( (theobj == -1) ) rc = -10;
   }

//
// Do the MQSUB on the obtained Subscription object
//
 if (rc == 0)
   {
    TRACE(traceid, ("Calling MQSUB\n") )                               ;
    MQSUB ( anchor->QMh, &sd, &anchor->Qh[theobj], &sh, &mqrc, &mqac ) ;
    rc   = mqrc                                                        ;

    if ( anchor->Qh[theobj] != 0 )   //If the Subscribe worked,
      {                              //then .....
       stem_from_long(traceid, NULL, RX_handle, ""  , theobj) ;
       make_stem_from_sd(traceid,&sd,RXo_sd)                  ; //and update the SD
      }
   }

//
// Set the LAST variables, and the function return string
//
 set_return(rc,mqrc,mqac,afuncname,ReturnMsg,aretstr,traceid,"") ;

return 0;
 } // End of RXMQSUB function

//
// Extension functions
//

//
// Do a Browse
//
//   Call:   rc = RXMQbrws(handle, data)
//
//           This function issues a browse on the Queue, and
//                returns the next message (if available). If you
//                need the Message descriptor as well, then use the
//                native Get interface.
//
//
FTYPE  RXMQBRWS  RXMQPARM
 {

 RXMQCB                * anchor = 0       ;  // RXMQ Control Block
 MQLONG                  rc = 0           ;  // Function Return Code
 MQLONG                  mqrc = 0         ;  // MQ RC
 MQLONG                  mqac = 0         ;  // MQ AC
 MQULONG                 traceid = BRO    ;  // This function trace id

 RXSTRING                RX_handle        ;  // Data     Obj Handle
 RXSTRING                RX_data          ;  // Variable Data

 MQMD2                   md               ;  // Browse MD
 MQGMO                   gmo              ;  // Browase GMO

 MQLONG                  handle    = 0    ;  //MQ object number
 void                 *  data      = 0    ;  //-> Data buffer
 MQLONG                  data0     = 0    ;  //   Data length max
 MQLONG                  datalen   = 0    ;  //   Data length actual

 RETMSG ReturnMsg[] = {
        {  -1, "Bad number of parms" },
        {  -2, "Null handle"},
        {  -3, "Zero data handle"},
        {  -4, "Null data stem var"},
        {  -5, "Zero data stem var"},
        {  -6, "Handle out of range"},
        {  -7, "Invalid handle"},
        {  -8, "malloc failure, check reason code"},
        {  -9, "Zero length input data buffer"},
        { -98, "Not connected to a QM"},
        { -99, "UNKNOWN FAILURE"}} ;

 rc = set_envir (afuncname, &traceid, &anchor)    ;

//
// Check the parms
//
 if ( (rc == 0) && (aargc != 2 ) )             rc =  -1 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[0]) )    rc =  -2 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[0]) ) rc =  -3 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[1]) )    rc =  -4 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[1]) ) rc =  -5 ;
 if ( (rc == 0) && ( anchor->QMh == 0 ) )      rc = -98 ;

//
// Now the parms are correct, get them
//
 if (rc == 0)
   {
    memcpy(&RX_handle,&aargv[0],sizeof(RX_handle)) ;
    memcpy(&RX_data,  &aargv[1],sizeof(RX_data))   ;

    TRACE(traceid, ("RX_handle = %.*s\n",(int)RX_handle.strlength,RX_handle.strptr) ) ;
    TRACE(traceid, ("RX_data   = %.*s\n",(int)RX_data.strlength,  RX_data.strptr)   ) ;

    parm_to_ulong(RX_handle, &handle)            ;
    stem_to_long(traceid, RX_data, "0" , &data0) ;
   }

//
// Now check the input Stem variable to see that there is
//     some valid data to obtain
//
 if ( (rc == 0) && ( data0 <= 0 ) ) rc = -9 ;

//
// See if the handle is valid
//
 if ( (rc == 0) && ( ( handle > MAXQS ) || ( handle <= 0 ) ) ) rc = -6 ;
 if ( (rc == 0) && ( anchor->Qh[handle] == 0 ) )               rc = -7 ;

//
// Now GETMAIN the buffer to receive the data records
//
 if (( rc == 0) && ( data0 != 0 ) )
   {
    TRACE(traceid, ("Doing malloc for %"PRId32" bytes\n",(int32_t)data0) ) ;
    data = malloc(data0)                                                   ;
    if ( data == NULL )
      {
       mqac = errno                                                        ;
       TRACE(traceid, ("malloc rc = %"PRId32"\n",(int32_t)mqac) )          ;
       rc = -8                                                             ;
      }
   }

//
// Now get the data from the queue
//
 if (rc == 0)
   {
    TRACE(traceid, ("BROWSE Maxdatalen = %"PRId32"\n",(int32_t)data0) )       ;
    memcpy(&md , &md_default , sizeof(MQMD2))                  ;
    memcpy(&gmo, &gmo_default, sizeof(MQGMO))                  ;
    gmo.Options        = MQGMO_NO_WAIT+MQGMO_BROWSE_NEXT
                       + MQGMO_ACCEPT_TRUNCATED_MSG
                       + MQGMO_FAIL_IF_QUIESCING               ;
    gmo.WaitInterval   = MQWI_UNLIMITED                        ;
    MQGET ( anchor->QMh, anchor->Qh[handle], &md, &gmo, data0, data, &datalen, &mqrc, &mqac ) ;
    rc   = mqrc ;
    TRACE(traceid, ("MQGET rc = %"PRId32", ac = %"PRId32", Datalen = %"PRId32"\n",
                    (int32_t)mqrc,(int32_t)mqac,(int32_t)datalen) ) ;

    stem_from_long  (traceid, NULL, RX_data, "0" , datalen)                 ;
    if (datalen > data0) datalen = data0                                    ;
    stem_from_bytes (traceid, NULL, RX_data, "1" , (MQBYTE *)data, datalen) ;
   }

//
// Free data buffer for stem.1 variable data, if allocated.
//
  if ( data != 0 )
    {
     TRACE(traceid, ("Free area\n") ) ;
     free(data) ;
    }

//
// Set the LAST variables, and the function return string
//
 set_return(rc,mqrc,mqac,afuncname,ReturnMsg,aretstr,traceid,"") ;

return 0;
 } // End of RXMQBRWS function

//
// Do a Header extract  MQHXT
//
//   Call:   rc = RXMQhxt( input_Stem, output_stem )
//
//
//
//   This function takes an input Stem variable, and builds an
//        output stem variable, extracting the Headers from the
//        'message' data.
//
//        The actual message & its length are placed in .1 and .0 , with
//            all the other info being expanded into the .item variables.
//            In addition, for compatability with the EVENT splitting up
//            capability .NAME and .TYPE are also set (both to either
//            DLH or XQH). .ZLIST is also set to contain the exploded
//            components (including TYPE NAME 0 1).
//
//
//
//   The headers processed are:
//
//       XQH - Transmission Header
//
//             .0      -> Data Length
//             .1      -> Message Data
//
//             .RQM    -> Remote Queue Manager Name
//             .RQN    -> Remote Queue Name
//
//             .TYPE   -> XQH     for compatability with
//             .NAME   -> XQH    Event splitting up
//
//             + all the MD options
//
//             .ZLIST  -> 0 1 AID AOD AT BC CID CCSI ENC EXP FBK
//                        FORM MSG MSGID NAME PAN PAT PD PER PRI
//                        PT REP RQM RQN RTOQ RTOQM TYPE UID
//
//
//       DLH - Dead Letter Header
//
//             .0      -> Data Length
//             .1      -> Message Data
//
//             .TYPE   -> DLH     for compatability with
//             .NAME   -> DLH     Event splitting up
//
//             .REA    -> Reason
//             .DQM    -> Destination Queue Manager Name
//             .DQN    -> Destination Queue Name
//             .ENC    -> Encoding
//             .CCSI   -> Coded Character Set ID
//             .FORM   -> Format
//             .PAT    -> Put Appl Type
//             .PAN    -> Put Appl Name
//             .PD     -> Put Date
//             .PT     -> Put Time
//
//             .ZLIST  -> 0 1 CCSI DQM DQN ENC FORM NAME PAN
//                        PAT PD PT REA TYPE
//
//
FTYPE  RXMQHXT  RXMQPARM
 {

 RXMQCB                * anchor = 0       ;  // RXMQ Control Block
 MQLONG                  rc = 0           ;  // Function Return Code
 MQLONG                  mqrc = 0         ;  // MQ RC
 MQLONG                  mqac = 0         ;  // MQ AC
 MQULONG                 traceid = HXT    ;  // This function trace id

 RXSTRING                RX_input         ;  // Variable Data - Input
 RXSTRING                RX_output        ;  // Variable Data - Output

 void                 *  data  = 0        ;  //-> Data buffer
 MQLONG                  data0            ;  // Input Data - len
 MQULONG                 datalen          ;  //   Data length
 MQLONG                  out0             ;  // Output Data - len

 MQXQH                 * thexqh           ;  // -> XQH
 MQDLH                 * thedlh           ;  // -> DLH

 char                    zlist[200]       ;  // Char version of .ZLIST


 RETMSG ReturnMsg[] = {
        {  -1, "Bad number of parms" },
        {  -2, "Null input stem var"},
        {  -3, "Zero input stem var"},
        {  -4, "Null output stem var"},
        {  -5, "Zero output stem var"},
        {  -6, "No input data"},
        {  -8, "Cannot verify Header"},
        { -10, "Unknown Header"},
        { -11, "Too short for a DLH"},
        { -12, "Too short for a XQH"},
        { -13, "malloc failure, check reason code"},
        { -14, "Data length is not equal to specified value"},
        { -99, "UNKNOWN FAILURE"}} ;

 rc = set_envir (afuncname, &traceid, &anchor)    ;

 sprintf((char *)zlist,"0 1") ;

//
// Check the parms
//
 if ( (rc == 0) && (aargc != 2 ) )             rc = -1 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[0]) )    rc = -2 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[0]) ) rc = -3 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[1]) )    rc = -4 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[1]) ) rc = -5 ;

//
// The input parms are OK, so obtain them
//
 if (rc == 0)
   {
    memcpy(&RX_input, &aargv[0],sizeof(RX_input))    ;
    memcpy(&RX_output,&aargv[1],sizeof(RX_output))   ;
    stem_to_long  (traceid, RX_input, "0" , &data0)  ;
   }

//
// Now check the input Stem variable to see that there is
//     some valid data to obtain
//
 if ( (rc == 0) && ( data0 <= 0 ) ) rc = -6  ; // No input data

//
// Allocate data buffer to store stem.1 variable data
//
 if ( rc == 0 )
   {
    TRACE(traceid, ("Doing malloc for %"PRId32" bytes\n",(int32_t)data0) ) ;
    data = malloc(data0)                                                   ;
    if ( data == NULL )
      {
       mqac = errno                                               ;
       TRACE(traceid, ("malloc rc = %"PRId32"\n",(int32_t)mqac) ) ;
       rc = -13                                                   ;
      }
    else
    {
     datalen = stem_to_data(traceid, RX_input, "1", (MQBYTE *)data, data0)        ;
     TRACE(traceid, ("Length of data received = %"PRIu32"\n",(uint32_t)datalen) ) ;
    }
   }

//
// Now check if stem.0 specifies the same value as stem.1 length.
// If not, don't know what to do.
//
 if ((rc == 0) && ( datalen != (MQULONG) data0 ) ) rc = -14 ;

//
// Now check the input Stem variable to see that there is
//     some valid data to obtain
//
 if ( (rc == 0) && ( datalen <= 3 ) ) rc = -8 ; // Cannot verify header

//
// There is the possibility of a Header, so see if it is a known
//       one, and ignore it, if it is not one to process.
//
 if ( rc == 0 )
   {
    if (    ( memcmp(data, MQDLH_STRUC_ID, sizeof(MQCHAR4)) != 0 )
         && ( memcmp(data, MQXQH_STRUC_ID, sizeof(MQCHAR4)) != 0 ) ) rc = -10   ;
   }

//
// Lets's try DLH
//
 if ( ( rc == 0 ) && ( memcmp(data, MQDLH_STRUC_ID, sizeof(MQCHAR4)) == 0 ) )
   {
    if ( datalen < sizeof(MQDLH) ) rc = -11 ; // Too short for DLH
    else
      {
       TRACE(traceid, ("Unravelling a DLH\n")  ) ;
       thedlh = (MQDLH *)data                    ; // Set Header pointer
       out0 = data0 - sizeof(MQDLH)              ; // Calc actual Data length

       stem_from_long  (traceid, NULL, RX_output, "0" , out0)                                 ;
       stem_from_bytes (traceid, NULL, RX_output, "1" , (MQBYTE *)data + sizeof(MQDLH), out0) ;

       TRACE(traceid, (" Msg  length = %"PRId32", DLH length = %"PRId32", Datalen = %"PRId32"\n",
             (int32_t)data0,(int32_t)sizeof(MQDLH),(int32_t)out0) )                 ;
       TRACE(traceid, ("Data ptr = %p, DLH ptr = %p, Start of data ",data,thedlh) ) ;
       TRACX(traceid, ((MQBYTE *)data + sizeof(MQDLH),out0) )                       ;
       TRACE(traceid, ("\n") )                                                      ;

       stem_from_string(traceid, zlist, RX_output, "TYPE" , "DLH",                   3               ) ;
       stem_from_string(traceid, zlist, RX_output, "NAME" , "DLH",                   3               ) ;
       stem_from_long  (traceid, zlist, RX_output, "REA"  , thedlh->Reason)                            ;
       stem_from_string(traceid, zlist, RX_output, "DQM"  , thedlh->DestQMgrName,    sizeof(MQCHAR48)) ;
       stem_from_string(traceid, zlist, RX_output, "DQN"  , thedlh->DestQName,       sizeof(MQCHAR48)) ;
       stem_from_long  (traceid, zlist, RX_output, "ENC"  , thedlh->Encoding)                          ;
       stem_from_long  (traceid, zlist, RX_output, "CCSI" , thedlh->CodedCharSetId)                    ;
       stem_from_string(traceid, zlist, RX_output, "FORM" , thedlh->Format,          sizeof(MQCHAR8) ) ;
       stem_from_long  (traceid, zlist, RX_output, "PAT"  , thedlh->PutApplType)                       ;
       stem_from_string(traceid, zlist, RX_output, "PAN"  , thedlh->PutApplName,     sizeof(MQCHAR28)) ;
       stem_from_string(traceid, zlist, RX_output, "PD"   , thedlh->PutDate,         sizeof(MQCHAR8) ) ;
       stem_from_string(traceid, zlist, RX_output, "PT"   , thedlh->PutTime,         sizeof(MQCHAR8) ) ;
       stem_from_string(traceid, zlist, RX_output, "ZLIST", zlist, strlen(zlist))                      ;

       TRACE(traceid, ("Unravelled the DLH\n") ) ;
      }
   }

 //
 // Lets's try XQH
 //
 if ( ( rc == 0 ) && ( memcmp(data, MQXQH_STRUC_ID, sizeof(MQCHAR4)) == 0 ) )
   {
    if ( datalen < sizeof(MQXQH) ) rc = -12 ; // Too short for XQH
      {
       TRACE(traceid, ("Unravelling a XQH\n") ) ;
       thexqh = (MQXQH *)data                   ; // Set Header pointer
       out0 = data0 - sizeof(MQXQH)             ; // Calc actual Data length

       stem_from_long  (traceid, NULL, RX_output, "0" , out0)                               ;
       stem_from_string(traceid, NULL, RX_output, "1" , (char *)data + sizeof(MQXQH), out0) ;

       TRACE(traceid, (" Msg  length = %"PRId32", XQH length = %"PRId32", Datalen = %"PRId32"\n",
             (int32_t)data0,(int32_t)sizeof(MQXQH),(int32_t)out0) )                 ;
       TRACE(traceid, ("Data ptr = %p, XQH ptr = %p, Start of data ",data,thexqh) ) ;
       TRACX(traceid, ((MQBYTE *)data + sizeof(MQXQH),out0) )                       ;
       TRACE(traceid, ("\n") )                                                      ;

       stem_from_string(traceid, zlist, RX_output, "TYPE" , "XQH",                   3               ) ;
       stem_from_string(traceid, zlist, RX_output, "NAME" , "XQH",                   3               ) ;
       stem_from_string(traceid, zlist, RX_output, "RQN"  , thexqh->RemoteQName,     sizeof(MQCHAR48)) ;
       stem_from_string(traceid, zlist, RX_output, "RQM"  , thexqh->RemoteQMgrName,  sizeof(MQCHAR48)) ;

       // Only Version 1 of MQMD is created here
       TRACE(traceid, ("Generating the XQH.MD\n") )                                                    ;
       stem_from_long  (traceid, zlist, RX_output, "VER"  , thexqh->MsgDesc.Version)                               ;
       stem_from_long  (traceid, zlist, RX_output, "REP"  , thexqh->MsgDesc.Report)                                ;
       stem_from_long  (traceid, zlist, RX_output, "MSG"  , thexqh->MsgDesc.MsgType)                               ;
       stem_from_long  (traceid, zlist, RX_output, "EXP"  , thexqh->MsgDesc.Expiry)                                ;
       stem_from_long  (traceid, zlist, RX_output, "FBK"  , thexqh->MsgDesc.Feedback)                              ;
       stem_from_long  (traceid, zlist, RX_output, "ENC"  , thexqh->MsgDesc.Encoding)                              ;
       stem_from_long  (traceid, zlist, RX_output, "CCSI" , thexqh->MsgDesc.CodedCharSetId)                        ;
       stem_from_string(traceid, zlist, RX_output, "FORM" , thexqh->MsgDesc.Format,              sizeof(MQCHAR8))  ;
       stem_from_long  (traceid, zlist, RX_output, "PRI"  , thexqh->MsgDesc.Priority)                              ;
       stem_from_long  (traceid, zlist, RX_output, "PER"  , thexqh->MsgDesc.Persistence)                           ;
       stem_from_bytes (traceid, zlist, RX_output, "MSGID", thexqh->MsgDesc.MsgId,               sizeof(MQBYTE24)) ;
       stem_from_bytes (traceid, zlist, RX_output, "CID"  , thexqh->MsgDesc.CorrelId,            sizeof(MQBYTE24)) ;
       stem_from_long  (traceid, zlist, RX_output, "BC"   , thexqh->MsgDesc.BackoutCount)                          ;
       stem_from_string(traceid, zlist, RX_output, "RTOQ" , thexqh->MsgDesc.ReplyToQ,            sizeof(MQCHAR48)) ;
       stem_from_string(traceid, zlist, RX_output, "RTOQM", thexqh->MsgDesc.ReplyToQMgr,         sizeof(MQCHAR48)) ;
       stem_from_string(traceid, zlist, RX_output, "UID"  , thexqh->MsgDesc.UserIdentifier,      sizeof(MQCHAR12)) ;
       stem_from_bytes (traceid, zlist, RX_output, "AT"   , thexqh->MsgDesc.AccountingToken,     sizeof(MQBYTE32)) ;
       stem_from_string(traceid, zlist, RX_output, "AID"  , thexqh->MsgDesc.ApplIdentityData,    sizeof(MQCHAR32)) ;
       stem_from_long  (traceid, zlist, RX_output, "PAT"  , thexqh->MsgDesc.PutApplType)                           ;
       stem_from_string(traceid, zlist, RX_output, "PAN"  , thexqh->MsgDesc.PutApplName,         sizeof(MQCHAR28)) ;
       stem_from_string(traceid, zlist, RX_output, "PD"   , thexqh->MsgDesc.PutDate,             sizeof(MQCHAR8))  ;
       stem_from_string(traceid, zlist, RX_output, "PT"   , thexqh->MsgDesc.PutTime,             sizeof(MQCHAR8))  ;
       stem_from_string(traceid, zlist, RX_output, "AOD"  , thexqh->MsgDesc.ApplOriginData,      sizeof(MQCHAR4))  ;
       TRACE(traceid, ("End of XQH.MD generation\n") )                                 ;
       stem_from_string(traceid, zlist, RX_output, "ZLIST", zlist, strlen(zlist))      ;
       TRACE(traceid, ("Unravelled the XQH\n") )                                       ;
      }
   }

//
// Free data buffer for stem.1 variable data, if allocated.
//
 if ( data != 0 )
   {
    TRACE(traceid, ("Free area\n") ) ;
    free(data) ;
   }

//
// Set the LAST variables, and the function return string
//
 set_return(rc,mqrc,mqac,afuncname,ReturnMsg,aretstr,traceid,"") ;

return 0;
 } // End of RXMQHXT function

//
// Do an Event extract  RXMQEVNT
//
//   Call:   rc = RXMQevnt( input_Stem, output_stem )
//
//
//
//   This function takes an input Stem variable, and builds an
//        output stem variable, extracting the Event Data from the
//        'message' data.
//
//        Stem.TYPE variable is set to 'EVENT'
//        Stem.REA variable is set to the event numerical value
//        Stem.NAME variable is set to the name of the event
//        corresponding to the numerical value expressed as a
//        MQRC_... constant name with MQRC_ prefix stripped.
//        All other info is expanded into Stem.item variables
//        where item is parameter attribute name like MQCA_...
//        with MQCA_prefix stripped.
//
//        Additionally (.)ZLIST is set to contain a list of all the
//        components (without the leading dot).
//
//     The events are splitup into the normal stem.component name, but
//                if the component is not a SINGLE string or Integer (as
//                defined by the IN & ST PCF types for the given field), then
//                the explosion is stem.component.0 for the number of items
//                                 stem.component.n for the individual items
//
//     The naming of the fields within the event is controlled by the
//                geteventname routine, which converts a known set of
//                fields (as defined by the Event part of the PCF book)
//                into character component names. Note that wherever the
//                field occurs generates its expansion - no matter whether
//                or not the PCF book say it should or should not occur!
//
FTYPE  RXMQEVNT  RXMQPARM
 {

 RXMQCB                * anchor = 0       ;  // RXMQ Control Block
 int                     i                ;  // Looper for parms
 int                     j                ;  // Looper for list
 int                     g                ;  // Looper for group
 MQLONG                  rc = 0           ;  // Function Return Code
 MQLONG                  mqrc = 0         ;  // MQ RC
 MQLONG                  mqac = 0         ;  // MQ AC
 MQULONG                 traceid = EVENT  ;  // This function trace id

 RXSTRING                RX_input         ;  // Variable Data - Input
 RXSTRING                RX_output        ;  // Variable Data - Output

 char                 *  data  = 0        ;  //-> Data buffer
 MQLONG                  data0            ;  // Input Data length
 MQLONG                  datalen          ;  //   Data length

 MQCFH                 * bufpcf           ;  // Pointer to PCF header
 MQCFIN                * bufpcfi          ;  // Pointer to PCF sub-structure

 MQLONG                  parms            ;  //Number of elements
 MQLONG                  grpparms         ;  //Number of elements in a group
 MQLONG                  lstparms         ;  //Number of elements in a list
 MQLONG                  parmtype         ;  //Type of element
 MQLONG                  parmsize         ;  //Length of element
 MQLONG                  parmnumb         ;  //Key of element
 char                  * sp               ;  //-> String element
 int                     sl               ;  //String length

 char                    comp[62]         ;  //Component name
 char                 *  zvars = 0        ;  //List of components
 MQULONG                 zvarlen = 4096U  ;  //Current length of list

 RXSTRING                varname          ;  // REXX string of varnamc
 RXSTRING                varvalu          ;  // REXX string of varvalc
 char                    varnamc[100]     ;  // Char version of variable name
 char                    varvalc[100]     ;  // Char version of variable value

 RETMSG ReturnMsg[] = {
        {  -1, "Bad number of parms" },
        {  -2, "Null input stem var"},
        {  -3, "Zero input stem var"},
        {  -4, "Null output stem var"},
        {  -5, "Zero output stem var"},
        {  -6, "No input data"},
        {  -8, "Cannot verify Header"},
        { -10, "Not an Event Header"},
        { -11, "Too short for an Event"},
        { -12, "Unknown Event Category"},
        { -13, "Unknown Event Type"},
        { -15, "malloc failure, check reason code"},
        { -16, "No elements in the Event"},
        { -97, "Handle not owned by current thread"},
        { -98, "Not connected to a QM"},
        { -99, "UNKNOWN FAILURE"}} ;

 rc = set_envir (afuncname, &traceid, &anchor)    ;

//
// Check the parms
//
 if ( (rc == 0) && (aargc != 2 ) )             rc = -1 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[0]) )    rc = -2 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[0]) ) rc = -3 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[1]) )    rc = -4 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[1]) ) rc = -5 ;

//
// The input parms are OK, so obtain them
//
 if (rc == 0)
   {
    memcpy(&RX_input, &aargv[0],sizeof(RX_input))  ;
    memcpy(&RX_output,&aargv[1],sizeof(RX_output)) ;
    stem_to_long(traceid, RX_input, "0" , &data0)  ;
   }

//
// Now check the input Stem variable to see that there is
//     some valid data to obtain
//
 if ( (rc == 0) && ( data0 <= 0 ) ) rc = -6 ;

//
// Allocate data buffer to store stem.1 variable data
//
 if ( rc == 0 )
   {
    TRACE(traceid, ("Doing malloc for %"PRId32" bytes\n",(int32_t)data0) ) ;
    data = (char *) malloc(data0)                                          ;
    if ( data == NULL )
      {
       mqac = errno                                               ;
       TRACE(traceid, ("malloc rc = %"PRId32"\n",(int32_t)mqac) ) ;
       rc = -15                                                   ;
      }
    else
    {
     datalen = stem_to_data(traceid, RX_input, "1", (MQBYTE *)data, data0)       ;
     TRACE(traceid, ("Length of data received = %"PRId32"\n",(int32_t)datalen) ) ;
    }
   }

//
// Now check if stem.0 specifies the same value as stem.1 length.
// If not, don't know what to do.
//
 if ((rc == 0) && ( datalen != data0 ) ) rc = -16 ;

//
// Now check the input Stem variable to see that there is
//     some valid data to obtain
//
 if ( (rc == 0) && ( datalen <= 3 ) ) rc = -8 ; // Cannot verify header

//
// There is the possibility of a Header, so see if it is an event
//       one, and ignore it it is not one to process.
//
 bufpcf    = (MQCFH  *)  data                      ;  //Point to PCF header
 bufpcfi   = (MQCFIN *) (data + MQCFH_STRUC_LENGTH);  //Point to Integer structure

 if ( ( rc == 0 ) && ( bufpcf->Type != MQCFT_EVENT ) ) rc = -10 ;

//
// Although we have a valid header, just check the prefix length
//          to ensure the WHOLE header is present
//
 if ( ( rc == 0 ) && ( datalen < MQCFH_STRUC_LENGTH) ) rc = -11 ;

//
// Now there is an event Header, check the general category
//
 if ( rc == 0 )
   {
    TRACE(traceid, ("Determining Event general Category\n") ) ;

    parms     = bufpcf->ParameterCount ;

    switch ( bufpcf->Command )  //See what the event category is
      {
       case MQCMD_Q_MGR_EVENT   : TRACE(traceid, ("It is a QMGR    Event\n") ) ; break ;
       case MQCMD_PERFM_EVENT   : TRACE(traceid, ("It is a PERF    Event\n") ) ; break ;
       case MQCMD_CHANNEL_EVENT : TRACE(traceid, ("It is a CHANNEL Event\n") ) ; break ;
       case MQCMD_CONFIG_EVENT  : TRACE(traceid, ("It is a CONFIG  Event\n") ) ; break ;
       case MQCMD_COMMAND_EVENT : TRACE(traceid, ("It is a COMMAND Event\n") ) ; break ;
       case MQCMD_LOGGER_EVENT  : TRACE(traceid, ("It is a LOGGER  Event\n") ) ; break ;
       default : rc = -12 ; break ;         //Other categories are not known yet
      } // End of Category determination select
   }

//
// Now there is a valid event Header, decide which one it is
//     and then create the NAME (and TYPE=EVENT) components
//
 if ( rc == 0 )
   {
    TRACE(traceid, ("Starting to examine the Event\n") ) ;

    sprintf(varnamc,"RXMQ.RCMAP.%"PRId32,(int32_t)bufpcf->Reason)  ; // Construct variable name
    MAKERXSTRING(varname,varnamc,strlen(varnamc))                  ; // Construct REXX variable name structure
    stem_to_string(traceid, varname, "", varvalc, sizeof(varvalc)) ; // Get variable value

    if ( strlen(varvalc) == 0) rc = -13          ;
    varvalu.strptr = strstr(varvalc, "_") + 1    ; // Bypass prefix
    varvalu.strlength = strlen(varvalu.strptr)   ; // New string length

    TRACE(traceid, ("Event %"PRId32" maps to %s %s\n",
          (int32_t)bufpcf->Reason,varvalu.strptr,(varvalu.strlength != 0) ? " " : " which is unknown ") ) ;
   }

//
// There is something that looks like a valid event, but just
//       ensure there are some elements in it
//
if ( rc == 0 )
  {
   TRACE(traceid, ("There are %"PRId32" elements in the Event\n",(int32_t)bufpcf->ParameterCount) ) ;
   if ( bufpcf->ParameterCount == 0 ) rc = -14 ;
  }

//
// Now there is a valid event ,
//           allocate zvars buffer,
//           create the TYPE and NAME & REAson components from the Prefix Header.
//           initialize the zvars (to goto .ZLIST) component store.
//
 if ( rc == 0 )
   {
    TRACE(traceid, ("Doing malloc for zvars %"PRId32" bytes\n",(int32_t)zvarlen) ) ;
    zvars = (char *) malloc(zvarlen)                                               ;
    if ( zvars == NULL )
      {
      mqac = errno                                           ;
      TRACE(traceid, ("malloc rc = %"PRId32,(int32_t)mqac) ) ;
      rc = -15                                               ;
      }
   }

if ( rc == 0 )
  {
   zvars[0] = '\0'                                                                         ;
   stem_from_string(traceid, zvars, RX_output, "TYPE" , "EVENT", strlen("EVENT"))          ;
   stem_from_string(traceid, zvars, RX_output, "NAME" , varvalu.strptr, varvalu.strlength) ;
   stem_from_long  (traceid, zvars, RX_output, "REA"  , bufpcf->Reason)                    ;
  }

//
// Now parse the event and create the relevant components
//
//    Build the ZLIST results into zvars for each parm
//    List entries are setup as stem.component.0 = number of items
//                                            .n = the nth item
// Note. Event parameters potentially may contain the following structures:
//             MQCFST,MQCFIN  - all
//             MQCFBS         - CONFIG_EVENT, COMMAND_EVENT only
//             MQCFGR         - COMMAND_EVENT only
//             MQCFSL         - Q_MGR_EVENT only
//       MQCFGR structure may have only one layer
//       Hook is set to catch unsupported parameters
//
 if ( rc == 0 )                      // Scan and Extract
   {
    TRACE(traceid, ("Now starting to scan the Event\n") ) ;

    memset(&comp,0,sizeof(comp));
    for (i=0 ; i < parms ; i++ )
      {
       if ( ( strlen(zvars) + RX_output.strlength + sizeof(comp) ) > zvarlen )
         {                           // If zvars is potentially too small
          zvarlen = 2 * zvarlen                                                           ;
          TRACE(traceid, ("Doing realloc for zvars %"PRId32" bytes\n",(int32_t)zvarlen) ) ;
          zvars = (char *) realloc(zvars, zvarlen)                                        ;
          if ( zvars == NULL )
            {
             mqac = errno ;
             TRACE(traceid, ("malloc rc = %"PRId32"\n",(int32_t)mqac) ) ;
             rc = -15     ;
             break        ;
            }
         }
       grpparms = 0 ;                    // Force pseudo group for 1 shot
       comp[0]  = 0 ;                    // Clear component name

       for ( g=0 ; g <= grpparms ; g++ )
         {
          parmtype = bufpcfi->Type        ; //All types share a
          parmsize = bufpcfi->StrucLength ; // common prefix
          parmnumb = bufpcfi->Parameter   ;

          sp = strstr(comp, ".")          ;   // If group component was used,
          if ( sp != NULL ) *(sp + 1) = 0 ;   // limit string to 1st qualifier
          geteventname(comp, parmnumb)    ;   // Translate parm to more readable

          switch ( parmtype )               // Select structure type
            {
             case MQCFT_GROUP  :            //Group of attributes
               strcat      (comp, ".")                          ;
               grpparms = ((MQCFGR *) bufpcfi)->ParameterCount  ;
               TRACE(traceid, (" Group Parm %"PRId32" into %s. Count = %"PRId32"\n",
                     (int32_t)parmnumb,comp,(int32_t)grpparms) ) ;
               break ;

             case MQCFT_INTEGER  :          //Integer type of attribute
               switch ( parmnumb ) //Display in Hex or (mostly) decimal
                 {
                  case MQIACF_AUX_ERROR_DATA_INT_1      :
                  case MQIACF_AUX_ERROR_DATA_INT_2      :
                  case MQIACF_ERROR_IDENTIFIER          :
                  case MQIACH_SSL_RETURN_CODE           :
                    sprintf(varvalc,"%08"PRIX32,(uint32_t)bufpcfi->Value)                                 ; // Display in HEX
                    TRACE(traceid, (" Integer Parm %"PRId32". Value = %"PRId32" <%08"PRIX32">\n",
                          (int32_t)bufpcfi->Parameter,(int32_t)bufpcfi->Value,(uint32_t)bufpcfi->Value) ) ;
                    break ;

                  default  :
                    sprintf(varvalc,"%"PRIu32,(uint32_t)bufpcfi->Value)  ; // Display in decimal
                    break ;
                 }
               stem_from_string (traceid, zvars, RX_output, comp ,
                                 varvalc, strlen(varvalc));
               TRACE(traceid, (" Integer Parm %"PRId32" ->%"PRId32"<- into %s\n",
                               (int32_t)bufpcfi->Parameter,(int32_t)bufpcfi->Value,comp) ) ;
               break ;

             case MQCFT_BYTE_STRING :       //Byte-string attribute
               sp = (char *) &((MQCFBS *)bufpcfi)->String          ;
               sl = ((MQCFBS *)bufpcfi)->StringLength              ;
               stem_from_bytes(traceid, zvars, RX_output, comp, (MQBYTE *)sp, sl) ;
               TRACE(traceid, (" Byte String %"PRId32" ->",(int32_t)parmnumb) )   ;
               TRACX(traceid, ((MQBYTE *)sp,sl) )                  ;
               TRACE(traceid, ("<- into %s\n",comp) )              ;
               break                                               ;

             case MQCFT_STRING :            //String attribute
               sp = ((MQCFST *)bufpcfi)->String                    ;
               sl = ((MQCFST *)bufpcfi)->StringLength              ;
               stem_from_string(traceid, zvars, RX_output, comp, sp, sl);
               TRACE(traceid, (" String Parm %"PRId32" ->%.*s<- into %s\n",
                     (int32_t)parmnumb,sl,sp,comp) ) ;
               break                                               ;

             case MQCFT_STRING_LIST : //Print Strings in the Stem Variable
               lstparms = ((MQCFSL *) bufpcfi)->Count     ;
               TRACE(traceid, (" String List %"PRId32" values\n",(int32_t)lstparms) ) ;
               if ( lstparms != 0 )
                 {
                  sprintf(varnamc,"%s.0",comp)            ;
                  stem_from_long(traceid, zvars, RX_output,
                                 varnamc, lstparms)       ;

                  sp = ((MQCFSL *)bufpcfi)->Strings       ;
                  sl = ((MQCFSL *)bufpcfi)->StringLength  ;
                  for ( j=0 ; j < lstparms ; j++ )
                    {
                     sprintf(varnamc,"%s.%u",comp,(j+1))  ;
                     stem_from_string(traceid, zvars, RX_output, varnamc, sp, sl);
                     TRACE(traceid, (" String Parm %"PRId32" ->*%.*s<- into %s\n",
                           (int32_t)parmnumb,sl,sp,comp) ) ;
                     sp = sp + sl ;
                    }
                 }
               break        ;
             default :
               TRACE(traceid, (" Unsupported type %"PRId32", component %s\n",
                     (int32_t)parmtype,comp) ) ;
               break ;
            } ; //End of Switch group

          bufpcfi  = (MQCFIN *)( (char *)bufpcfi  + parmsize ) ; //Bump up pointer

         } ; //End of Group scanning loop
      } ; //End of Parameter scanning loop


// All Parms/Components extracted, so create ZLIST

    stem_from_string(traceid, NULL, RX_output, "ZLIST", zvars , strlen(zvars));

    TRACE(traceid, ("All Event fields extracted\n") ) ;
   } // End of Event Processing Block

//
// Free ZLIST buffer, if allocated.
//
 if ( zvars != 0 )
   {
    TRACE(traceid, ("Free zlist\n") ) ;
    free(zvars) ;
   }

//
// Free data buffer for stem.1 variable data, if allocated.
//
 if ( data != 0 )
   {
    TRACE(traceid, ("Free area\n") )  ;
    free(data) ;
   }

//
// Set the LAST variables, and the function return string
//
 set_return(rc,mqrc,mqac,afuncname,ReturnMsg,aretstr,traceid,"") ;

return 0;
 } // End of RXMQEVNT function


//
// Process a Trigger Message RXMQTM
//
//   Call:   rc = RXMQtm( input_Stem,   output_stem )
//
//           rc = RXMQtm( trig_message, output_stem )
//
//
//   This function works in two ways, depending upon whether or not the first
//        parameter ends with a dot (indicating a stem variable or some real
//        data).
//
//        If its a stem variable, then the usual .0 .1 convention applies and
//           and the message is deemed to be a Trigger Message derived from a
//           Queue. In this case, it is broken up into its components of
//           QN PN TD AT AID ED UD. These components are only generated if
//           the relevant fields are non-blank and non-zero. ZLIST processing
//           is provided for these components. Additionally, a component called
//           PL is created, which is a string (in TM2 format) suitable for
//           passing to an attached procedure (the QM field will be filled in
//           with the current Queue manager, if connected). PL is NOT part of
//           ZLIST (as it should not be fiddled about with in any way).
//
//        If its NOT a stem variable (not ending in a dot), then the input is
//           deamed to be the TM2 (PL) structure (which must be of the correct
//           length). In this case, the data (and its NOT a REXX variable) is
//           parsed for components QN PN TD AT ED UD QM with ZLIST processing;
//           if the item is all blanks (or all nulls!?!) then the component is
//           not built. Each component should be stripped of blanks before
//           usage in the Rexx environment.
//
//
FTYPE  RXMQTM  RXMQPARM
 {

 RXMQCB                * anchor  = 0      ;  // RXMQ Control Block
 MQLONG                  rc      = 0      ;  // Function Return Code
 MQLONG                  mqrc    = 0      ;  // MQ RC
 MQLONG                  mqac    = 0      ;  // MQ AC
 MQULONG                 traceid = TM     ;  // This function trace id

 RXSTRING                RX_input         ;  // Variable Data - Input
 RXSTRING                RX_output        ;  // Variable Data - Output

 void                 *  data  = 0        ;  //-> Data buffer
 MQLONG                  data0            ;  // Input Data - len
 MQULONG                 datalen          ;  //   Data length

 MQTM                  * thetm            ;  // -> Trigger Message
 MQTMC2                * thetm2           ;  // -> Trigger Parm

 char                    header[5]        ;  // Header  Type
 char                    versionc[5]      ;  // Version Type in Char
 MQLONG                  version          ;  // Version Type
 int                     action = 1       ;  // TM or TM2 processing

 RXSTRING                oldtm2           ;  // Supplied Trigger Message
 MQTMC2                  newtm2           ;  // Build Trigger Parm

 char                    zlist[200]       ;  // Char version of .ZLIST

 RETMSG ReturnMsg[] = {
        {  -1, "Bad number of parms" },
        {  -2, "Null input stem var"},
        {  -3, "Zero input stem var"},
        {  -4, "Null output stem var"},
        {  -5, "Zero output stem var"},
        {  -6, "No input data"},
        {  -7, "Zero input data"},
        {  -8, "Cannot locate Header"},
        {  -9, "Cannot find Header"},
        { -10, "Not a TM Header"},
        { -11, "Cannot find Header"},
        { -12, "Unknown Header"},
        { -13, "Unknown Version"},
        { -14, "Header mismatch (1<>1)"},
        { -15, "Version mismatch (1<>1)"},
        { -16, "Header mismatch (2<>C)"},
        { -17, "Version mismatch (2<>C)"},
        { -18, "Too short for a TM"},
        { -19, "Too short for a TMC"},
        { -21, "Data length is not equal to specified value"},
        { -98, "Not connected to a QM"},
        { -99, "UNKNOWN FAILURE"}} ;

 rc = set_envir (afuncname, &traceid, &anchor)    ;

 action = 1 ;
 zlist[0] = 0                             ;
 memset(&newtm2,  ' ', sizeof(newtm2)   ) ;

//
// Check the parms
//
 if ( (rc == 0) && (aargc != 2 ) )             rc =  -1 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[0]) )    rc =  -2 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[0]) ) rc =  -3 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[1]) )    rc =  -4 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[1]) ) rc =  -5 ;
 if ( (rc == 0) && ( anchor->QMh == 0 ) )      rc = -98 ;

//
// The input parms are OK, so obtain them, and obtain the resolutions
//
 if (rc == 0)
   {
    memcpy(&RX_input, &aargv[0],sizeof(RX_input))  ;
    memcpy(&RX_output,&aargv[1],sizeof(RX_output)) ;

    int dotat = RX_input.strlength - 1                       ;
    if ( RX_input.strptr[dotat] == '.' )
      {
       TRACE(traceid , ("Processing a Trigger Message\n") )  ;
       action = 1                                            ;
       stem_to_long  (traceid, RX_input, "0" , &data0)       ;

//
// Allocate data buffer to store stem.1 variable data
//
       if (( data0 > 0 ) )
         {
          TRACE(traceid, ("Doing malloc for %"PRId32" bytes\n",(int32_t)data0) ) ;
          data = malloc(data0)                                                   ;
          if ( data == NULL )
            {
             mqac = errno                                                        ;
             TRACE(traceid, ("malloc rc = %"PRId32"\n",(int32_t)mqac) )          ;
             rc = -20                                                            ;
            }
          else
           {
            datalen = stem_to_data(traceid, RX_input, "1",(MQBYTE *)data, data0)     ;
            TRACE(traceid , ("Obtained the Trigger Message. Length = %"PRId32"\n",(int32_t)data0) ) ;
            if ( datalen != (MQULONG) data0 ) rc = -21                               ;
           }
         }
      }
    else
      {    // RX_input.strptr[dotat] != '.'
       TRACE(traceid , ("Processing Trigger Data\n") ) ;
       action = 2                                      ;
       oldtm2.strptr    = RX_input.strptr    ;
       oldtm2.strlength = RX_input.strlength ;
       TRACE(traceid , ("Obtained the Trigger Data. Length = %"PRIu32"\n",(uint32_t)RX_input.strlength) ) ;
      }
   }

//
// Now check the input Stem variable to see that there is
//     some valid data to obtain
//
 if ( (rc == 0) && (action == 1) && ( data0 <= 0 ) )   rc = -6 ;
 if ( (rc == 0) && (action == 1) && ( datalen == 0 ) ) rc = -7 ;
 if ( (rc == 0) && (action == 1) && ( data0 <= 3 ) )   rc = -8 ;
 if ( (rc == 0) && (action == 1) && ( datalen <= 3 ) ) rc = -9 ;

//
// Now check the input data to see that there is
//     some valid data to obtain
//
 if ( (rc == 0) && (action == 2) && ( oldtm2.strlength == 0 ) ) rc = -10 ;
 if ( (rc == 0) && (action == 2) && ( oldtm2.strlength <= 3 ) ) rc = -11 ;

//
// There is the possibility of an Trigger Message, so see if it is a known
//       one, and ignore it it is not one to process.
//
 if ( rc == 0 )
   {
    memset(header,    0, sizeof(header)   ) ;
    memset(&versionc, 0, sizeof(versionc) ) ;
    version = 0                             ;

    if ( action == 1 ) strncpy( header,   (const char *) data         , sizeof(MQCHAR4) ) ;
    else               strncpy( header,   (const char *)oldtm2.strptr , sizeof(MQCHAR4) ) ;

    if ( action == 1 ) version = ((MQTM *)data)->Version  ;
    else    strncpy( versionc, (const char *)&(((MQTMC2 *)oldtm2.strptr )->Version) , sizeof(MQCHAR4) ) ;

    TRACE(traceid, ("Action = %d, Header = /%s/ Versionc = /%s/ Version = /%"PRId32"/\n",
                    action,header,versionc,(int32_t)version) ) ;

    if (    ( rc == 0 )
         && ( strcmp(header, MQTM_STRUC_ID ) != 0 )
         && ( strcmp(header, MQTMC_STRUC_ID) != 0 ) ) rc = -12 ;

    if (    ( rc == 0 )
         && ( version  != MQTM_VERSION_1 )
         && ( strncmp(versionc, MQTMC_VERSION_2, sizeof(MQCHAR4) ) != 0 ) ) rc = -13 ;
   }

//
// Now there is a trigger area of some sort, just check to see its the right one
//
 if ( ( rc == 0 ) && (action == 1) && ( strcmp(header, MQTM_STRUC_ID ) != 0 ) )  rc = -14 ;
 if ( ( rc == 0 ) && (action == 1) && ( version != MQTM_VERSION_1 ) )            rc = -15 ;
 if ( ( rc == 0 ) && (action == 2) && ( strcmp(header, MQTMC_STRUC_ID ) != 0 ) ) rc = -16 ;
 if ( ( rc == 0 ) && (action == 2) && ( strncmp(versionc, MQTMC_VERSION_2, sizeof(MQCHAR4) ) != 0 ) ) rc = -17 ;

//
// Although we have a valid item, just check the lengths
//          to ensure the WHOLE thing is present
//
 if ( ( rc == 0 ) && ( action == 1 ) && ( datalen < sizeof(MQTM) ) )             rc = -18 ;
 if ( ( rc == 0 ) && ( action == 2 ) && ( oldtm2.strlength <  sizeof(MQTMC2) ) ) rc = -19 ;

//
// Now we have got a valid Trigger Message, split it up
//
if ( ( rc == 0 ) && ( action == 1 ) )
  {
   TRACE(traceid, ("Unravelling a TM message\n")       ) ;
   TRACE(traceid, ("QM name is /%s/\n",anchor->QMname) ) ;
   TRACE(traceid, ("Now starting the unpacking\n")     ) ;
   thetm = (MQTM *) data; // Set Header pointer

                         // Build the output TMC2 area Header

   memcpy(&newtm2.StrucId,     MQTMC_STRUC_ID,     sizeof(MQCHAR4)  ) ;
   memcpy(&newtm2.Version,     MQTMC_VERSION_2,    sizeof(MQCHAR4)  ) ;
   memcpy(&newtm2.QName,       thetm->QName,       sizeof(MQCHAR48) ) ;
   memcpy(&newtm2.ProcessName, thetm->ProcessName, sizeof(MQCHAR48) ) ;
   memcpy(&newtm2.TriggerData, thetm->TriggerData, sizeof(MQCHAR64) ) ;
   memcpy(&newtm2.ApplId,      thetm->ApplId,      sizeof(MQCHAR256)) ;
   memcpy(&newtm2.EnvData,     thetm->EnvData,     sizeof(MQCHAR128)) ;
   memcpy(&newtm2.UserData,    thetm->UserData,    sizeof(MQCHAR128)) ;
   memcpy(&newtm2.QMgrName,    anchor->QMname,     sizeof(MQCHAR48) ) ;

                         //Build the components

   stem_from_string(traceid, zlist, RX_output, "QN"  , thetm->QName,                sizeof(MQCHAR48)) ;
   stem_from_string(traceid, zlist, RX_output, "PN"  , thetm->ProcessName,          sizeof(MQCHAR48)) ;
   stem_from_string(traceid, zlist, RX_output, "TD"  , thetm->TriggerData,          sizeof(MQCHAR64)) ;
   stem_from_long  (traceid, zlist, RX_output, "AT"  , thetm->ApplType)                               ;
   stem_from_string(traceid, zlist, RX_output, "AID" , thetm->ApplId,              sizeof(MQCHAR256)) ;
   stem_from_string(traceid, zlist, RX_output, "ED"  , thetm->EnvData,             sizeof(MQCHAR128)) ;
   stem_from_string(traceid, zlist, RX_output, "UD"  , thetm->UserData,            sizeof(MQCHAR128)) ;
   stem_from_string(traceid, zlist, RX_output, "ZLIST", zlist, strlen(zlist))                         ;
   stem_from_string(traceid, zlist, RX_output, "PL"  , (MQCHAR *)&newtm2,sizeof(MQTMC2))              ;

   TRACE(traceid, ("Unravelled the TM\n") ) ;
  }

//
// Now we have got valid Trigger Data, split it up
//
if ( ( rc == 0 ) && ( action == 2 ) )
  {
   TRACE(traceid, ("Unravelling Trigger Data\n") ) ;

   thetm2 = (MQTMC2 *)oldtm2.strptr   ; // Set Header pointer

//Build the components

   stem_from_string(traceid, zlist, RX_output, "QN"  , thetm2->QName,          sizeof(MQCHAR48)) ;
   stem_from_string(traceid, zlist, RX_output, "PN"  , (char*)thetm2->ProcessName, sizeof(MQCHAR48)) ;
   stem_from_string(traceid, zlist, RX_output, "TD"  , thetm2->TriggerData,          sizeof(MQCHAR64)) ;
   stem_from_string(traceid, zlist, RX_output, "AID" , thetm2->ApplId,              sizeof(MQCHAR256)) ;
   stem_from_string(traceid, zlist, RX_output, "ED"  , thetm2->EnvData,             sizeof(MQCHAR128)) ;
   stem_from_string(traceid, zlist, RX_output, "UD"  , thetm2->UserData,            sizeof(MQCHAR128)) ;
   stem_from_string(traceid, zlist, RX_output, "QM"  , thetm2->QMgrName,             sizeof(MQCHAR48)) ;
   stem_from_string(traceid, zlist, RX_output, "ZLIST", zlist, strlen(zlist))                         ;

   TRACE(traceid, ("Unravelled the Trigger Data\n") ) ;
  }

//
// Free data buffer for stem.1 variable data, if allocated.
//
 if ( data != 0 )
   {
    TRACE(traceid, ("Free area\n") ) ;
    free(data) ;
   }

//
// Set the LAST variables, and the function return string
//
 set_return(rc,mqrc,mqac,afuncname,ReturnMsg,aretstr,traceid,"") ;

return 0;
 } // End of RXMQTM function


#ifdef __MVS__
//
// Execute a Command (use MQSC interface for MVS)
//
//   Call:   rc = RXMQC(parms, input_command, output_response )
//
FTYPE RXMQC  RXMQPARM
{
 RXMQCB                * anchor = 0       ;  // RXMQ Control Block
 MQLONG                  rc      = 0      ;  // Function Return Code
 MQLONG                  mqrc    = 0      ;  // MQ RC
 MQLONG                  mqac    = 0      ;  // MQ AC
 MQULONG                 traceid = COM    ;  // This function trace id
 MQLONG                  dummy            ;  // No interest rc
 MQULONG                 count   = 0      ;  // Responses received
 MQULONG                 linecnt = 0      ;  // Line count in reply msg
 MQULONG                 lines   = 0      ;  // Lines processed in a group

 RXSTRING                RX_parm          ;  // Variable Parms
 RXSTRING                RX_command       ;  // Variable Command
 RXSTRING                RX_response      ;  // Variable Reply Stem Var

 char                 *  buffer   =     0 ;  //-> Data buffer
 MQLONG                  bufflen  = 15000 ;  // Default buffer length
 MQLONG                  reclen   =     0 ;  // Received record length

 MQLONG        DisconnectFinally = 0      ;  // Disconnect after completion
 MQHCONN                 qmh              ;  // Queue Manager handle
 MQHOBJ                  cQh     = 0      ;  // Command queue handle
 MQOD                    cod              ;  // Command queue object descriptor
 MQHOBJ                  rQh     = 0      ;  // Reply queue handle
 MQOD                    rod              ;  // Reply queue object descriptor
 MQMD2                   md               ;  // Message descriptor for PUT & GET
 MQPMO                   pmo              ;  // PUT message options
 MQGMO                   gmo              ;  // GET message options


 char        qm   [MQ_Q_MGR_NAME_LENGTH+1] ; //QM name
 char        cq   [MQ_Q_NAME_LENGTH+1    ] ; //Q  name - command
 char        rq   [MQ_Q_NAME_LENGTH+1    ] ; //Q  name - replyToq
 MQBYTE24    CorrelMsg                     ; //PUT MsgId = GET CorrelId
 char        var  [10]                     ;
 MQLONG      to   = 5000                   ; //Timeout for MQ Get in msec

 RETMSG ReturnMsg[] = {
        {  -1, "Bad number of parms" },
        {  -2, "Null parms"},
        {  -3, "Zero parms"},
        {  -4, "Null command var"},
        {  -5, "Zero command var"},
        {  -6, "Null response stem var"},
        {  -7, "Zero length response stem var"},
        {  -8, "No command supplied"},
        {  -9, "Too big a Command supplied"},
        { -10, "malloc failure, check reason code"},
        { -11, "Connect to QMgr failed, check rc/rsn"},
        { -12, "Open command queue failed, check rc/rsn"},
        { -13, "Open response queue failed, check rc/rsn"},
        { -14, "Put command to queue failed, check rc/rsn"},
        { -15, "Get response from queue failed, check rc/rsn"},
        { -99, "UNKNOWN FAILURE"}} ;

 rc = set_envir (afuncname, &traceid, &anchor)    ;

 if ( (rc == 0) && (aargc != 3 ) )             rc = -1 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[0]) )    rc = -2 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[0]) ) rc = -3 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[1]) )    rc = -4 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[1]) ) rc = -5 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[2]) )    rc = -6 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[2]) ) rc = -7 ;

 if (rc == 0)
   {
    memcpy(&RX_parm,    &aargv[0],sizeof(RX_parm))     ; //QM Name (or stem.)
    memcpy(&RX_command, &aargv[1],sizeof(RX_command))  ; //Command to issue
    memcpy(&RX_response,&aargv[2],sizeof(RX_response)) ; //Return stem.

    TRACE(traceid, ("RX_parm     = %.*s\n",(int)RX_parm.strlength,    RX_parm.strptr)     ) ;
    TRACE(traceid, ("RX_command  = %.*s\n",(int)RX_command.strlength, RX_command.strptr)  ) ;
    TRACE(traceid, ("RX_response = %.*s\n",(int)RX_response.strlength,RX_response.strptr) ) ;

    memset(qm,0,MQ_Q_MGR_NAME_LENGTH+1     ) ; //Clear data areas
    memset(cq,0,MQ_Q_NAME_LENGTH+1         ) ;
    memset(rq,0,MQ_Q_NAME_LENGTH+1         ) ;
    strcpy(cq,"SYSTEM.COMMAND.INPUT" )       ; // and set defaults
    strcpy(rq,"SYSTEM.COMMAND.REPLY.MODEL" ) ;

 //If the given variable is not a stem. variable, then take the
 //   quicker option of assuming it's the Queue Manager name

    if ( RX_parm.strptr[RX_parm.strlength-1] != '.' )
      memcpy(qm, RX_parm.strptr, RX_parm.strlength ) ;

 //The given variable is a stem. variable, so get its contents
    else
      {
       stem_to_string(traceid, RX_parm, "QM"  , qm, MQ_Q_MGR_NAME_LENGTH) ;
       stem_to_string(traceid, RX_parm, "CQ"  , cq, MQ_Q_NAME_LENGTH    ) ;
       stem_to_string(traceid, RX_parm, "RQ"  , rq, MQ_Q_NAME_LENGTH    ) ;
       stem_to_long  (traceid, RX_parm, "TO"  , &to                     ) ;
      }

    TRACE(traceid, ("QM = %s\n",qm) ) ;
    TRACE(traceid, ("CQ = %s\n",cq) ) ;
    TRACE(traceid, ("RQ = %s\n",rq) ) ;
    TRACE(traceid, ("Command = <%.*s>\n",(int)RX_command.strlength,RX_command.strptr) ) ;

   }

 if ( (rc == 0 ) && ( RX_command.strlength == 0 ) )                     rc = -8 ;
 if ( (rc == 0 ) && ( RX_command.strlength > MQ_COMMAND_MQSC_LENGTH ) ) rc = -9 ;

//
//The return stem variable is initially set to no info
//
 if ( rc == 0 ) stem_from_long (traceid, NULL, RX_response, "0" , 0)                 ;

// 1) Connect to queue manager, if not yet connected

 if (rc == 0)
   if ( (anchor->QMh == 0) || strcmp(anchor->QMname, qm) )   // Is it connected to correct QM ?
     {                                                       // No
      DisconnectFinally = 1                         ;
      TRACE(traceid, ("Connecting to QM %s\n",qm) ) ;
      MQCONN ( qm, &qmh, &mqrc, &mqac )             ;
      TRACE(traceid, ("MQCONN rc = %ld\n",mqrc ) )  ;
      if ( mqrc != 0 )  rc = -11                    ;
     }
   else qmh = anchor->QMh                           ;


// 2) Open the command queue for PUT access

 if ( rc == 0 )
   {
    memcpy ( &cod, &od_default, sizeof(MQOD))     ;
    strncpy( cod.ObjectQMgrName, qm, strlen(qm))  ;
    strncpy( cod.ObjectName, cq, strlen(cq))      ;
    TRACE(traceid, ("Opening Command Q %s for output\n",cq) ) ;
    MQOPEN ( qmh, &cod, MQOO_OUTPUT, &cQh, &mqrc, &mqac )     ;
    TRACE(traceid, ("MQOPEN rc = %ld\n",mqrc) )               ;
    if ( mqrc != 0 ) rc = -12                             ;
   }

// 3) Open/Create the ReplyToQ for Get access using model

 if ( rc == 0)
   {
    memcpy(&rod, &od_default, sizeof(MQOD));
    strcpy(rod.ObjectName, rq)             ;
    strcpy(rod.DynamicQName,"RXMQ.*")      ;
    TRACE(traceid, ("Opening Response Q by model %s for Destructive access\n",rq) ) ;
    MQOPEN ( qmh, &rod, MQOO_INPUT_SHARED, &rQh, &mqrc, &mqac ) ;
    TRACE(traceid, ("MQOPEN rc = %ld\n",mqrc) )                 ;
    if ( mqrc != 0 ) rc = -13              ;
   }

// 4) Put command on command queue

 if ( rc == 0)
   {
    memcpy(rq, rod.ObjectName, MQ_Q_NAME_LENGTH)         ;
    TRACE(traceid, ("The ReplyToQ name is %.*s\n",MQ_Q_NAME_LENGTH,rq) ) ;

    memcpy(&md,  &md_default,  sizeof(MQMD2))            ;
    md.MsgType = MQMT_REQUEST                            ;
    memcpy(md.Format, MQFMT_STRING, sizeof(MQCHAR8))     ;
    memcpy(md.ReplyToQ, rq, MQ_Q_NAME_LENGTH)            ;
    memcpy(md.ReplyToQMgr, qm, MQ_Q_MGR_NAME_LENGTH)     ;

    memcpy(&pmo, &pmo_default, sizeof(MQPMO))            ;
    pmo.Options = MQPMO_NO_SYNCPOINT
                | MQPMO_DEFAULT_CONTEXT                  ;

    TRACE(traceid, ("Now issuing the MQPUT to the Command Queue\n") ) ;
    MQPUT ( qmh, cQh, &md, &pmo, RX_command.strlength, RX_command.strptr, &mqrc, &mqac ) ;
    TRACE(traceid, ("MQPUT rc = %ld\n",mqrc) )                   ;
    if ( mqrc != 0 ) rc = -14                            ;
   }

 if (cQh) MQCLOSE ( qmh, &cQh, MQCO_NONE, &dummy, &dummy ) ;


//
// Allocate data buffer to receive the ReplyToQ records
//
 if ( rc == 0 )
   {
    TRACE(traceid, ("Doing malloc for %ld bytes\n",bufflen) )  ;
    buffer = (char *) malloc(bufflen)                          ;
    if ( buffer == NULL )
      {
       mqac = errno                                ;
       TRACE(traceid, ("malloc rc = %ld\n",mqac) ) ;
       rc = -10                                    ;
      }
   }

// 5) Get results

 if ( rc == 0 )
   {
    TRACE(traceid, ("Now starting to obtain the ReplyToQ messages\n") ) ;
    memcpy(CorrelMsg, md.MsgId, sizeof(MQBYTE24))   ;
    memcpy(&gmo, &gmo_default, sizeof(MQGMO))       ;
    gmo.Options = MQGMO_NO_SYNCPOINT                |
                  MQGMO_WAIT                        ;
    gmo.WaitInterval =    to                        ;  // 5 sec

    mqac = 4                                        ;  // CSQN205I rsn to continue
    while ( ( (mqrc == 0) && (mqac == 4) ) || (linecnt < lines) )
      {
       TRACE(traceid, ("Issuing a MQGET to the ReplyToQ %s\n",rq) ) ;
       memcpy(&md,  &md_default,  sizeof(MQMD2))             ;
       memcpy(md.CorrelId, CorrelMsg, sizeof(MQBYTE24))      ;
       memset(buffer, 0, bufflen)                            ;
       MQGET ( qmh, rQh, &md, &gmo, bufflen, buffer, &reclen, &mqrc, &mqac ) ;
       TRACE(traceid, ("MQGET rc = %ld, ac = %ld, Datalen = %ld\n",mqrc,mqac,reclen) ) ;

       if (mqac == MQRC_TRUNCATED_MSG_FAILED)            // buffer is too small
         {
          bufflen = bufflen * 2              ;         // double the size of buffer
          TRACE(traceid, ("Reallocating buffer with double size = %s\n",bufflen) ) ;
          rc = 0                                      ;
          buffer = (char *) realloc(buffer,bufflen)   ;
          if ( buffer == NULL )
            {
             mqac = errno                                ;
             TRACE(traceid, ("malloc rc = %ld\n",mqac) ) ;
             rc = -10                                    ;
             break;
            }
          continue;
         }

    if (mqrc != 0) rc = -15 ;

    if (!strncmp ("CSQN205I", (const char *)buffer, 8))
      {
       linecnt = 1;
       sscanf(((const char *) buffer)+17, "%8u", &lines) ;
       sscanf(((const char *) buffer)+34, "%8X", &mqrc) ;
       sscanf(((const char *) buffer)+51, "%8X", &mqac);
       TRACE(traceid, ("CSQN205I COUNT = %ld, RETURN = %ld, REASON = %ld\n",lines,mqrc,mqac) ) ;
      }
    else
      if ( lines == 0 )              // CSQN205I must be 1st response
        {
        TRACE(traceid, ("No CSQN205I response received\n") ) ;
         break;
        }
      else
        {
         count++   ;
         linecnt++ ;
         sprintf(var,"%d", count );
         stem_from_long  (traceid, NULL, RX_response, "0" , count)                           ;
         stem_from_bytes (traceid, NULL, RX_response, var , (unsigned char *)buffer, reclen) ;
        }
      }
   }

//
// Free data buffer, if allocated.
//
 if ( buffer != 0 )
   {
   TRACE(traceid, ("Free buffer\n") ) ;
    free(buffer) ;
   }

 if (rQh) MQCLOSE ( qmh, &rQh, MQCO_DELETE_PURGE, &dummy, &dummy ) ;

 if( DisconnectFinally ) MQDISC ( &qmh, &dummy, &dummy ) ;

//
// Set the LAST variables, and the function return string
//
 set_return(rc,mqrc,mqac,afuncname,ReturnMsg,aretstr,traceid,"") ;

return 0;
}

#else
//
// Execute a Command (use PCF interface for Windows, AIX, Linux)
//
//   Call:   rc = RXMQC(parms, input_command, output_response )
//
FTYPE RXMQC RXMQPARM
{

 RXMQCB                * anchor  = 0      ;  // RXMQ Control Block
 MQLONG                  rc      = 0      ;  // Function Return Code
 MQLONG                  mqrc    = 0      ;  // MQ RC
 MQLONG                  mqac    = 0      ;  // MQ AC
 MQLONG                  dummy            ;  // No interest rc
 MQULONG                 traceid = COM    ;  // This function trace id

 RXSTRING                RX_parm          ;  // Variable Parms
 RXSTRING                RX_command       ;  // Variable Command
 RXSTRING                RX_response      ;  // Variable Reply Stem Var

 char        qm   [MQ_Q_MGR_NAME_LENGTH+1] ; //QM name
 char        cq   [MQ_Q_NAME_LENGTH+1    ] ; //Q  name - command
 char        rq   [MQ_Q_NAME_LENGTH+1    ] ; //Q  name - replyToq
 MQLONG      to   = 10000                  ; //Timeout for MQ Get (10 sec)

 MQLONG        DisconnectFinally = 0      ;  // Disconnect after completion
 MQHCONN                 qmh              ;  // Queue Manager handle
 MQHOBJ                  cQh              ;  // Command queue handle
 MQOD                    cod              ;  // Command queue object descriptor
 MQHOBJ                  rQh              ;  // Reply queue handle
 MQOD                    rod              ;  // Reply queue object descriptor
 MQMD2                   md               ;  // Message descriptor for PUT & GET
 MQPMO                   pmo              ;  // PUT message options
 MQGMO                   gmo              ;  // GET message options

 char        pcfarea  [MQCFH_STRUC_LENGTH        +    //PCF area - header
                       MQCFIN_STRUC_LENGTH       +    //         - integer parm
                       MQCFST_STRUC_LENGTH_FIXED +    //         - string parm
                       MAXCOMMLEN+5              ] ;  //         - command

 MQCFH     * ptrpcf1 = (MQCFH  *) pcfarea                 ; //Set PCF area pointers
 MQCFIN    * ptrpcf2 = (MQCFIN *)&pcfarea[MQCFH_STRUC_LENGTH]    ;
 MQCFST    * ptrpcf3 = (MQCFST *)&pcfarea[MQCFH_STRUC_LENGTH+MQCFIN_STRUC_LENGTH] ;
 char      * ptrpcf4 = (char   *)&pcfarea[MQCFH_STRUC_LENGTH+MQCFIN_STRUC_LENGTH+MQCFST_STRUC_LENGTH_FIXED] ;

 MQCFH     * bufpcf1         ; //PCF pointers
 MQCFIN    * bufpcf2i        ; // for the returned
 MQCFST    * bufpcf2s        ; // info
 MQCFIL    * bufpcf2il       ;
 MQCFSL    * bufpcf2sl       ;

 MQLONG      pcflen  = 0    ; //Size of PCF to be sent
 MQLONG      recno   = 0    ; //Obtained record number
 MQLONG      reclen  = 0    ;

 void                 *  buffer       = 0     ;  //-> Data buffer
 int                     bufflen      = 10000 ;  //   Data length max

 char                    command[MAXCOMMLEN+5]  ; //Padded Command
 MQLONG                  commandlen             ; // to send to QM

 RETMSG ReturnMsg[] = {
        {  -1, "Bad number of parms" },
        {  -2, "Null parms"},
        {  -3, "Zero parms"},
        {  -4, "Null command var"},
        {  -5, "Zero command var"},
        {  -6, "Null response stem var"},
        {  -7, "Zero length response stem var"},
        {  -8, "No command supplied"},
        {  -9, "Too big a Command supplied"},
        { -10, "malloc failure, check reason code"},
        { -11, "Connect to QMgr failed, check rc/rsn"},
        { -12, "Open command queue failed, check rc/rsn"},
        { -13, "Open response queue failed, check rc/rsn"},
        { -14, "Put command to queue failed, check rc/rsn"},
        { -15, "Get response from queue failed, check rc/rsn"},
        { -99, "UNKNOWN FAILURE"}} ;

 rc = set_envir (afuncname, &traceid, &anchor)    ;

//
// Check the parms
//

 if ( (rc == 0) && (aargc != 3 ) )             rc = -1 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[0]) )    rc = -2 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[0]) ) rc = -3 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[1]) )    rc = -4 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[1]) ) rc = -5 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[2]) )    rc = -6 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[2]) ) rc = -7 ;

//
// Now the parms are correct, get them
//

 if (rc == 0)
   {
    memcpy(&RX_parm,    &aargv[0],sizeof(RX_parm))     ; //QM Name (or stem.)
    memcpy(&RX_command, &aargv[1],sizeof(RX_command))  ; //Command to issue
    memcpy(&RX_response,&aargv[2],sizeof(RX_response)) ; //Return stem.

    TRACE(traceid, ("RX_parm     = %.*s\n",(int)RX_parm.strlength,    RX_parm.strptr)     ) ;
    TRACE(traceid, ("RX_command  = %.*s\n",(int)RX_command.strlength, RX_command.strptr)  ) ;
    TRACE(traceid, ("RX_response = %.*s\n",(int)RX_response.strlength,RX_response.strptr) ) ;


    memset(qm,0,MQ_Q_MGR_NAME_LENGTH+1     ) ; //Clear data areas
    memset(cq,0,MQ_Q_NAME_LENGTH+1         ) ;
    memset(rq,0,MQ_Q_NAME_LENGTH+1         ) ;
    strcpy(cq,"SYSTEM.ADMIN.COMMAND.QUEUE" ) ; // and set defaults
    strcpy(rq,"SYSTEM.MQSC.REPLY.QUEUE"    ) ;

 //If the given variable is not a stem. variable, then take the
 //   quicker option of assuming it's the Queue Manager name

    if ( RX_parm.strptr[RX_parm.strlength-1] != '.' )
      memcpy(qm, RX_parm.strptr, RX_parm.strlength ) ;

 //The given variable is a stem. variable, so get its contents
    else
      {
       stem_to_string(traceid, RX_parm, "QM"  , qm, MQ_Q_MGR_NAME_LENGTH) ;
       stem_to_string(traceid, RX_parm, "CQ"  , cq, MQ_Q_NAME_LENGTH    ) ;
       stem_to_string(traceid, RX_parm, "RQ"  , rq, MQ_Q_NAME_LENGTH    ) ;
       stem_to_long  (traceid, RX_parm, "TO"  , &to                     ) ;
      }

    TRACE(traceid, ("QM = %s\n",qm) ) ;
    TRACE(traceid, ("CQ = %s\n",cq) ) ;
    TRACE(traceid, ("RQ = %s\n",rq) ) ;
    TRACE(traceid, ("Command = <%.*s>\n",(int)RX_command.strlength,RX_command.strptr) ) ;
   }

 //If no command, then nothing to do

 if ( (rc == 0 ) && ( RX_command.strlength == 0 ) ) rc = -8 ;

 //If command is too big, error

 if ( (rc == 0 ) && ( RX_command.strlength > MAXCOMMLEN ) ) rc = -9 ;

//Ensure Command is a multiple of 4 bytes long (PCF String requirement)

if ( rc == 0 )
  {
   memset(command,0,sizeof(command)) ;
   memcpy(command,RX_command.strptr,RX_command.strlength) ;
   switch ( RX_command.strlength % 4 )
     {
      case 1  : strcat(command,"   ") ; break ;
      case 2  : strcat(command,"  " ) ; break ;
      case 3  : strcat(command," "  ) ; break ;
      default : break                         ;
     }
   commandlen = strlen(command) ;
  }

//
// Allocate data buffer to receive the ReplyToQ records
//
 if ( rc == 0 )
   {
    TRACE(traceid, ("Doing malloc for %d bytes\n",bufflen) ) ;
    buffer = malloc(bufflen)                                 ;
    if ( buffer == NULL )
      {
       mqac = errno                                               ;
       TRACE(traceid, ("malloc rc = %"PRId32"\n",(int32_t)mqac) ) ;
       rc = -10                                                   ;
      }
   }

//The return stem variable is initially set to no info

 if ( rc == 0 ) stem_from_long  (traceid, NULL, RX_response, "0" , 0);

//Connect to the QM, open the Command Queue, and create the ReplyToQ

 if (rc == 0)
   {
    if ( (anchor->QMh == 0) || strcmp(anchor->QMname, qm) ) // Is it connected to correct QM ?
      {                                                     // No
       DisconnectFinally = 1                                        ;
       TRACE(traceid, ("Connecting to QM %s\n",qm) )                ;
       MQCONN ( qm, &qmh, &mqrc, &mqac )                            ;
       TRACE(traceid, ("MQCONN rc = %"PRId32"\n",(int32_t)mqrc ) )  ;
       if ( mqrc != 0 )  rc = -11                                   ;
      }
    else qmh = anchor->QMh                                          ;
   }

 if ( rc == 0 )                      // Open the command Q for Put access
   {
    memcpy ( &cod, &od_default, sizeof(MQOD))     ;
    strncpy( cod.ObjectQMgrName, qm, strlen(qm))  ;
    strncpy( cod.ObjectName, cq, strlen(cq))      ;
    TRACE(traceid, ("Opening Command Q %s for output\n",cq) )  ;
    MQOPEN ( qmh, &cod, MQOO_OUTPUT, &cQh, &mqrc, &mqac )      ;
    TRACE(traceid, ("MQOPEN rc = %"PRId32"\n",(int32_t)mqrc) ) ;
    if ( mqrc != 0 ) rc = -12                                  ;
   }

 if ( rc == 0 )                      // Open/Create the ReplyToQ for Get access
   {
    memcpy(&rod, &od_default, sizeof(MQOD));
    strcpy(rod.ObjectName, rq)             ;
    strcpy(rod.DynamicQName,"RXMQ.*")      ;
    TRACE(traceid, ("Opening Response Q by model %s for Destructive access\n",rq) ) ;
    MQOPEN ( qmh, &rod, MQOO_INPUT_SHARED, &rQh, &mqrc, &mqac ) ;
    TRACE(traceid, ("MQOPEN rc = %"PRId32"\n",(int32_t)mqrc) )  ;
    if ( mqrc != 0 ) rc = -13                                   ;
  }

 //Build the PCF ESCAPE command

 if ( rc == 0 )                      // Now build the PCF command
   {
    TRACE(traceid, ("Now building the PCF area\n") ) ;
    memset(pcfarea,0,sizeof(pcfarea))             ;
    ptrpcf1->Type           = MQCFT_COMMAND       ; // Header - 2 parms
    ptrpcf1->StrucLength    = MQCFH_STRUC_LENGTH  ;
    ptrpcf1->Version        = MQCFH_VERSION_1     ;
    ptrpcf1->Command        = MQCMD_ESCAPE        ;
    ptrpcf1->MsgSeqNumber   = 1                   ;
    ptrpcf1->Control        = MQCFC_LAST          ;
    ptrpcf1->ParameterCount = 2                   ;
    ptrpcf2->Type           = MQCFT_INTEGER       ; // Parm1 is ESCAPE
    ptrpcf2->StrucLength    = MQCFIN_STRUC_LENGTH ;
    ptrpcf2->Parameter      = MQIACF_ESCAPE_TYPE  ;
    ptrpcf2->Value          = MQET_MQSC           ;
    ptrpcf3->Type           = MQCFT_STRING        ; // Parm2 is ESCAPE data
    ptrpcf3->StrucLength    = MQCFST_STRUC_LENGTH_FIXED + commandlen  ;
    ptrpcf3->Parameter      = MQCACF_ESCAPE_TEXT  ;
    ptrpcf3->CodedCharSetId = MQCCSI_DEFAULT      ;
    ptrpcf3->StringLength   = commandlen          ;
    memcpy(ptrpcf4,command,commandlen)            ;

    pcflen =   MQCFH_STRUC_LENGTH
             + MQCFIN_STRUC_LENGTH
             + MQCFST_STRUC_LENGTH_FIXED
             + commandlen                         ;

   }

// Now write the Command to the relevant Command Queue

 if ( rc == 0)                       //Now write to the Queue
   {
    memcpy(&md,  &md_default,  sizeof(MQMD2))            ;
    md.MsgType = MQMT_REQUEST                            ;
    memcpy(md.Format, MQFMT_ADMIN, sizeof(MQCHAR8))      ;
    memcpy(md.ReplyToQ, rod.ObjectName, MQ_Q_NAME_LENGTH)            ;
    memcpy(md.ReplyToQMgr, qm, MQ_Q_MGR_NAME_LENGTH)     ;

    memcpy(&pmo, &pmo_default, sizeof(MQPMO))            ;
    pmo.Options = MQPMO_NO_SYNCPOINT         +
                  MQPMO_DEFAULT_CONTEXT      + //Needed to bluff PCF Security!
                  MQPMO_FAIL_IF_QUIESCING    ;

    TRACE(traceid, ("Now issuing the MQPUT to the Command Queue\n") ) ;
    MQPUT ( qmh, cQh, &md, &pmo, pcflen, pcfarea, &mqrc, &mqac )      ;
    TRACE(traceid, ("MQPUT rc = %"PRId32"\n",(int32_t)mqrc) )         ;
    if ( mqrc != 0 ) rc = -14                                         ;
   }


 //Read the ReplyToQ, and display the contained messages

 if ( rc == 0 )                      // Read Q and Display
   {
    TRACE(traceid, ("Now starting to obtain the ReplyToQ messages\n"             ) ) ;
    TRACE(traceid, ("The ReplyToQ name is %.*s\n",MQ_Q_NAME_LENGTH,rod.ObjectName) ) ;

    for ( recno = 1 ; ; recno++ )
      {
       char     exiter = 'N' ;

       bufpcf1   = (MQCFH  *) buffer                               ; //Point to
       bufpcf2i  = (MQCFIN *)&((char *)buffer)[MQCFH_STRUC_LENGTH] ; //Buffer
       bufpcf2s  = (MQCFST *)&((char *)buffer)[MQCFH_STRUC_LENGTH] ; //Structures
       bufpcf2il = (MQCFIL *)&((char *)buffer)[MQCFH_STRUC_LENGTH] ;
       bufpcf2sl = (MQCFSL *)&((char *)buffer)[MQCFH_STRUC_LENGTH] ;

       memcpy(&gmo, &gmo_default, sizeof(MQGMO))       ;
       gmo.Options = MQGMO_NO_SYNCPOINT         +
                     MQGMO_WAIT                 +
                     MQGMO_ACCEPT_TRUNCATED_MSG +
                     MQGMO_FAIL_IF_QUIESCING    ;
       gmo.WaitInterval =    to                        ;  // 5 sec

       memcpy(&md,  &md_default,  sizeof(MQMD2))             ;
       memset(buffer, 0, bufflen)                            ;
       TRACE(traceid, ("Issuing a MQGET to the ReplyToQ\n") )                ;
       MQGET ( qmh, rQh, &md, &gmo, bufflen, buffer, &reclen, &mqrc, &mqac ) ;
       TRACE(traceid, ("MQGET rc = %"PRId32", ac = %"PRId32", Datalen = %"PRId32"\n",
             (int32_t)mqrc,(int32_t)mqac,(int32_t)reclen) )  ;

       switch ( rc ) //Print obtained message
         {
          case MQRC_NONE      :
            TRACE(traceid, ("Message %"PRId32" = MQ Message %"PRId32" %s has MQ rc = %"PRId32", %"PRId32"\n",
                 (int32_t)recno,(int32_t)bufpcf1->MsgSeqNumber,
                 (bufpcf1->Control == MQCFC_LAST) ? " (last)" : " (more)",
                 (int32_t)bufpcf1->Reason,(int32_t)bufpcf1->ParameterCount) ) ;

            if ( bufpcf1->ParameterCount != 0 ) //Do not attempt to ignore null messages
              {                                // when printing the results
               MQLONG   parms           ;
               int      i               ;
               int      j               ;
               MQLONG   parmtype        ;
               MQLONG   parmsize        ;
               MQCFIL * ip              ;
               MQCFSL * sp              ;
               char     resvar[100] ;
               parms = bufpcf1->ParameterCount ;

               for (i=0 ; i < parms ; i++ )
                {
                 parmtype = bufpcf2i->Type        ; //All types share a
                 parmsize = bufpcf2i->StrucLength ; // common prefix
                 switch ( parmtype )
                   {
                    case MQCFT_INTEGER  : //Convert to attr=value
                      TRACE(traceid, (" Integer parm %"PRId32" ->%"PRId32"<-\n",
                            (int32_t)bufpcf2i->Parameter,(int32_t)bufpcf2i->Value) ) ;
                      break        ;

                    case MQCFT_INTEGER_LIST : //Convert to attr=value pairs
                      if ( bufpcf2il->Count != 0 )
                        {
                         TRACE(traceid, (" Integer parm values = %"PRId32"\n",(int32_t)bufpcf2il->Parameter) ) ;
                         for ( j=0 ; j < bufpcf2il->Count ; j++ )
                           {
                            ip = bufpcf2il + (sizeof(MQLONG) * j) ;
                            TRACE(traceid, ("%p",ip->Values) ) ;
                           }
                        }
                      break        ;

                    case MQCFT_STRING : //Print in the Stem variable
                      stem_from_long  (traceid, NULL, RX_response, "0" , recno) ;
                      sprintf(resvar,"%"PRId32, (int32_t)recno)                ;
                      stem_from_bytes(traceid, NULL, RX_response, resvar,
                                     (MQBYTE *)&(bufpcf2s->String) , bufpcf2s->StringLength) ;
                      break        ;

                    case MQCFT_STRING_LIST : //Print Strings in the Stem Variable
                      if ( bufpcf2sl->Count != 0 )
                        {
                         for ( j=0 ; j < bufpcf2sl->Count ; j++ )
                          {
                           sp = bufpcf2sl + ( (bufpcf2sl->StringLength) * j)           ;
                           stem_from_long  (traceid, NULL, RX_response, "0" , recno)   ;
                           sprintf(resvar,"%"PRId32, (int32_t)recno)                   ;
                           stem_from_bytes(traceid, NULL, RX_response, resvar ,
                                          (MQBYTE *)&(sp->Strings), bufpcf2s->StringLength);
                           break                                      ;
                          }
                        }
                      break        ;
                    default : break ;
                   }

                 bufpcf2i  = (MQCFIN *)( (char *)bufpcf2i  + parmsize ) ; //Bump
                 bufpcf2s  = (MQCFST *)( (char *)bufpcf2s  + parmsize ) ; // up
                 bufpcf2il = (MQCFIL *)( (char *)bufpcf2il + parmsize ) ; // structure
                 bufpcf2sl = (MQCFSL *)( (char *)bufpcf2sl + parmsize ) ; // pointers

                }  //End of Parameter Printing loop
              }  //End of Parameter Listing

            if ( bufpcf1->Control == MQCFC_LAST) exiter = 'L' ; //Last means just that!
            break          ;

          default             :
            TRACE(traceid, ("MQGET on ReplyToQ error rc = %"PRId32" on message %"PRId32"\n",
                  (int32_t)rc,(int32_t)recno) ) ;
            rc = -15      ;
            exiter = 'G'  ;
            break         ;
           }

         if ( exiter != 'N' ) break ; //Get next message
                                      //unless error/EOQ occured
        } // End of Message Processing FOR loop
    } // End of Queue/Queue Processing Block

 TRACE(traceid, ("All ReplyToQ records obtained\n") ) ;

//End of processing

 TRACE(traceid, ("Closing the Command Q\n") ) ; //Close the Command Queue
 if (cQh) MQCLOSE ( qmh, &cQh, MQCO_NONE, &dummy, &dummy ) ;

 TRACE(traceid, ("Closing the ReplyToQ\n") ) ; //Close/Delete the ReplyToQ
 if (rQh) MQCLOSE ( qmh, &rQh, MQCO_DELETE_PURGE, &dummy, &dummy ) ;

 if ( DisconnectFinally )
   {
    TRACE(traceid, ("Disconnecting from QM\n") ) ; //Disconnect from QM
    MQDISC ( &qmh, &dummy, &dummy ) ;
   }

//
// Free data buffer, if allocated.
//
 if ( buffer != 0 )
   {
    TRACE(traceid, ("Free area\n") ) ;
    free(buffer) ;
   }
//
// Set the function return string
//
 set_return(rc,mqrc,mqac,afuncname,ReturnMsg,aretstr,traceid,"") ;

//
// Return to caller
//
return 0;

 } // End of RXMQC function
#endif

//
// Perform one of RXMQ operations  RXMQV
//
//   Call:   rc = RXMQV(<RXMQ operation name>, <RXMQ operation's parameters>)
//                   Available operations:
//
//                     INIT     ->  RXMQINIT, performs REXX environment initialization
//                     TERM     ->  RXMQTERM, performs REXX environment termination
//                     CONS     ->  RXMQCONS, setup MQ constants for REXX
//                     CONN     ->  RXMQCONN, connect to QManager
//                     OPEN     ->  RXMQOPEN, open MQ queue
//                     CLOSE    ->  RXMQCLOS, close MQ queue
//                     DISC     ->  RXMQDISC, disconnect from QManager
//                     CMIT     ->  RXMQCMIT, do a syncpoint
//                     BACK     ->  RXMQBACK, do a rollback
//                     PUT      ->  RXMQPUT,  do a MQPUT operation
//                     PUT1     ->  RXMQPUT1, do a MQPUT1 operation
//                     GET      ->  RXMQGET,  do a MQGET operation
//                     INQ      ->  RXMQINQ,  do a MQINQ operation
//                     SET      ->  RXMQSET,  do a MQSET operation
//                     SUB      ->  RXMQSUB,  do a MQSUB operation
//                     BROWSE   ->  RXMQBRWS, do a MQBROWSE operation
//                     HXT      ->  RXMQHXT,  do a Header extract, MQHXT
//                     EVENT    ->  RXMQEVNT, extract the Event Data from the 'message' data
//                     TM       ->  RXMQTM,   process a Trigger Message
//
FTYPE RXMQV  RXMQPARM
{
 MQLONG                  rc = 0           ;  // Function Return Code
 MQLONG                  mqrc = 0         ;  // MQ RC
 MQLONG                  mqac = 0         ;  // MQ AC
 MQULONG                 traceid = MQV    ;  // This function trace id
 MQULONG                 i                ;  // Looper
 char                    name[8]          ;  // Uppercased function name
 MQULONG                 namelen          ;  // Name length

 RETMSG ReturnMsg[] = {
        {  -1, "Bad number of parms" },
        {  -2, "Null MQ function name"},
        {  -3, "Zero length MQ function name"},
        { -20, "Unknown MQ function name"},
        { -99, "UNKNOWN FAILURE"}} ;

 // Check the parms
 if ( (rc == 0) && (aargc == 0 ) )                 rc = -1 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[0]) )    rc = -2 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[0]) ) rc = -3 ;

 typedef struct
         {
          char * func_name;
          int (APIENTRY *func_ptr) RXMQPARM;

 } mqftypes;

 mqftypes funclist[] = {
          {"INIT"  , RXMQINIT},
          {"TERM"  , RXMQTERM},
          {"CONS"  , RXMQCONS},
          {"CONN"  , RXMQCONN},
          {"OPEN"  , RXMQOPEN},
          {"CLOSE" , RXMQCLOS},
          {"DISC"  , RXMQDISC},
          {"CMIT"  , RXMQCMIT},
          {"BACK"  , RXMQBACK},
          {"PUT1"  , RXMQPUT1},
          {"PUT"   , RXMQPUT},
          {"GET"   , RXMQGET},
          {"INQ"   , RXMQINQ},
          {"SET"   , RXMQSET},
          {"SUB"   , RXMQSUB},
          {"BROWSE", RXMQBRWS},
          {"HXT"   , RXMQHXT},
          {"EVENT" , RXMQEVNT},
          {"TM"    , RXMQTM},
          {"?"     , NULL}  };

// Uppercase specified function name
 namelen = (aargv[0].strlength < 8) ? aargv[0].strlength : 8 ;
 for (i = 0; i < namelen; i++)
   name[i] = toupper(aargv[0].strptr[i]);
 name[namelen] = '\0' ;

// Find and call appropriate function
 if (rc == 0)  for (i = 0; ; i++)
   {
    if (funclist[i].func_name[0] == '?') break;
    if ( ( strlen(funclist[i].func_name) == namelen ) &&
         ( memcmp(funclist[i].func_name, name, namelen) == 0) )
      return funclist[i].func_ptr(name, aargc-1, &(aargv[1]), aqname, aretstr);
   }

 rc = -20 ;

 set_return(rc,mqrc,mqac,afuncname,ReturnMsg,aretstr,traceid,"") ;

 return 0;
}

//
// Perform one of RXMQ Command operations  RXMQVC
//
//   Call:   rc = RXMQVC(<RXMQ Command operation name>, <RXMQ Command operation's parameters>)
//                   Available operations:
//
//                     INIT     ->  RXMQINIT, performs REXX environment initialization
//                     TERM     ->  RXMQTERM, performs REXX environment termination
//                     COMMAND  ->  RXMQC,    implements MQ command line
//
FTYPE RXMQVC  RXMQPARM
{
 MQLONG                  rc = 0           ;  // Function Return Code
 MQLONG                  mqrc = 0         ;  // MQ RC
 MQLONG                  mqac = 0         ;  // MQ AC
 MQULONG                 traceid = MQV    ;  // This function trace id
 MQULONG                 i                ;  // Looper
 char                    name[8]          ;  // Uppercased function name
 MQULONG                 namelen          ;  // Name length

 RETMSG ReturnMsg[] = {
        {  -1, "Bad number of parms" },
        {  -2, "Null MQ function name"},
        {  -3, "Zero length MQ function name"},
        { -20, "Unknown MQ function name"},
        { -99, "UNKNOWN FAILURE"}} ;

// Check the parms
 if ( (rc == 0) && (aargc == 0 ) )             rc = -1 ;
 if ( (rc == 0) && RXNULLSTRING(aargv[0]) )    rc = -2 ;
 if ( (rc == 0) && RXZEROLENSTRING(aargv[0]) ) rc = -3 ;

 typedef struct
         {
          char * func_name;
          int (APIENTRY *func_ptr) RXMQPARM;
         } mqftypes;

 mqftypes funclist[] = {
          {"INIT"   , RXMQINIT},
          {"TERM"   , RXMQTERM},
          {"COMMAND", RXMQC},
          {"?"      , NULL}  };

// Uppercase specified function name
 namelen = (aargv[0].strlength < 8) ? aargv[0].strlength : 8 ;
 for (i = 0; i < namelen; i++)
   name[i] = toupper(aargv[0].strptr[i]);
 name[namelen] = '\0' ;

// Find and call appropriate function
 if (rc == 0) for (i = 0; ; i++)
   {
    if (funclist[i].func_name[0] == '?') break;
    if ( ( strlen(funclist[i].func_name) == namelen ) &&
         ( memcmp(funclist[i].func_name, name, namelen) == 0) )
      return funclist[i].func_ptr(name, aargc-1, &(aargv[1]), aqname, aretstr);
    }

 rc = -20;

 set_return(rc,mqrc,mqac,afuncname,ReturnMsg,aretstr,traceid,"") ;

 return 0;
}

//
// Support for long Windows REXX function names compatible with MA78
//
#ifdef _RXMQN
FTYPE  RXMQNINIT  RXMQPARM
 {
  return RXMQINIT(afuncname,aargc,aargv,aqname,aretstr);
 }

FTYPE  RXMQNCONS  RXMQPARM
 {
  return RXMQCONS (afuncname,aargc,aargv,aqname,aretstr);
 }

FTYPE  RXMQNTERM  RXMQPARM
 {
   return RXMQTERM (afuncname,aargc,aargv,aqname,aretstr);
 }

FTYPE  RXMQNCONN  RXMQPARM
 {
  return RXMQCONN (afuncname,aargc,aargv,aqname,aretstr);
 }

FTYPE  RXMQNOPEN  RXMQPARM
 {
  return RXMQOPEN (afuncname,aargc,aargv,aqname,aretstr);
 }

FTYPE  RXMQNCLOSE  RXMQPARM
 {
  return RXMQCLOS (afuncname,aargc,aargv,aqname,aretstr);
 }

FTYPE  RXMQNDISC  RXMQPARM
 {
  return RXMQDISC (afuncname,aargc,aargv,aqname,aretstr);
 }

FTYPE  RXMQNCMIT  RXMQPARM
 {
  return RXMQCMIT (afuncname,aargc,aargv,aqname,aretstr);
 }

FTYPE  RXMQNBACK  RXMQPARM
 {
  return RXMQBACK (afuncname,aargc,aargv,aqname,aretstr);
 }

FTYPE  RXMQNPUT  RXMQPARM
 {
  return RXMQPUT (afuncname,aargc,aargv,aqname,aretstr);
 }

FTYPE  RXMQNPUT1  RXMQPARM
 {
  return RXMQPUT1 (afuncname,aargc,aargv,aqname,aretstr);
 }

FTYPE  RXMQNGET  RXMQPARM
 {
  return RXMQGET (afuncname,aargc,aargv,aqname,aretstr);
 }

FTYPE  RXMQNINQ  RXMQPARM
 {
  return RXMQINQ (afuncname,aargc,aargv,aqname,aretstr);
 }

FTYPE  RXMQNSET  RXMQPARM
 {
  return RXMQSET (afuncname,aargc,aargv,aqname,aretstr);
 }

FTYPE  RXMQNSUB  RXMQPARM
 {
  return RXMQSUB (afuncname,aargc,aargv,aqname,aretstr);
 }

FTYPE  RXMQNBROWSE  RXMQPARM
 {
  return RXMQBRWS (afuncname,aargc,aargv,aqname,aretstr);
 }

FTYPE  RXMQNHXT  RXMQPARM
 {
  return RXMQHXT (afuncname,aargc,aargv,aqname,aretstr);
 }

FTYPE  RXMQNEVENT  RXMQPARM
 {
  return RXMQEVNT (afuncname,aargc,aargv,aqname,aretstr);
 }

FTYPE  RXMQNTM  RXMQPARM
 {
  return RXMQTM (afuncname,aargc,aargv,aqname,aretstr);
 }

FTYPE  RXMQNC  RXMQPARM
 {
  return RXMQC (afuncname,aargc,aargv,aqname,aretstr);
 }
#endif

#ifdef _RXMQT
FTYPE  RXMQTINIT  RXMQPARM
 {
  return RXMQINIT(afuncname,aargc,aargv,aqname,aretstr);
 }

FTYPE  RXMQTCONS  RXMQPARM
 {
  return RXMQCONS (afuncname,aargc,aargv,aqname,aretstr);
 }

FTYPE  RXMQTTERM  RXMQPARM
 {
  return RXMQTERM (afuncname,aargc,aargv,aqname,aretstr);
 }

FTYPE  RXMQTCONN  RXMQPARM
 {
  return RXMQCONN (afuncname,aargc,aargv,aqname,aretstr);
 }

FTYPE  RXMQTOPEN  RXMQPARM
 {
  return RXMQOPEN (afuncname,aargc,aargv,aqname,aretstr);
 }

FTYPE  RXMQTCLOSE  RXMQPARM
 {
  return RXMQCLOS (afuncname,aargc,aargv,aqname,aretstr);
 }

FTYPE  RXMQTDISC  RXMQPARM
 {
  return RXMQDISC (afuncname,aargc,aargv,aqname,aretstr);
 }

FTYPE  RXMQTCMIT  RXMQPARM
 {
  return RXMQCMIT (afuncname,aargc,aargv,aqname,aretstr);
 }

FTYPE  RXMQTBACK  RXMQPARM
 {
  return RXMQBACK (afuncname,aargc,aargv,aqname,aretstr);
 }

FTYPE  RXMQTPUT  RXMQPARM
 {
  return RXMQPUT (afuncname,aargc,aargv,aqname,aretstr);
 }

FTYPE  RXMQTPUT1  RXMQPARM
 {
  return RXMQPUT1 (afuncname,aargc,aargv,aqname,aretstr);
 }

FTYPE  RXMQTGET  RXMQPARM
 {
  return RXMQGET (afuncname,aargc,aargv,aqname,aretstr);
 }

FTYPE  RXMQTINQ  RXMQPARM
 {
  return RXMQINQ (afuncname,aargc,aargv,aqname,aretstr);
 }

FTYPE  RXMQTSET  RXMQPARM
 {
  return RXMQSET (afuncname,aargc,aargv,aqname,aretstr);
 }

FTYPE  RXMQTSUB  RXMQPARM
 {
  return RXMQSUB (afuncname,aargc,aargv,aqname,aretstr);
 }

FTYPE  RXMQTBROWSE  RXMQPARM
 {
  return RXMQBRWS (afuncname,aargc,aargv,aqname,aretstr);
 }

FTYPE  RXMQTHXT  RXMQPARM
 {
  return RXMQHXT (afuncname,aargc,aargv,aqname,aretstr);
 }

FTYPE  RXMQTEVENT  RXMQPARM
 {
  return RXMQEVNT (afuncname,aargc,aargv,aqname,aretstr);
 }

FTYPE  RXMQTTM  RXMQPARM
 {
  return RXMQTM (afuncname,aargc,aargv,aqname,aretstr);
 }

FTYPE  RXMQTC  RXMQPARM
 {
  return RXMQC (afuncname,aargc,aargv,aqname,aretstr);
 }
#endif
