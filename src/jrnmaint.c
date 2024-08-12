/***************************************************************************/
/*                                                                         */
/*  Program name:   JRNMAINT                                               */
/*  Author:         Mark Phillips                                          */
/*  Function:       Automate jrn maintenance in IBM MQ for IBM i           */
/*                                                                         */
/*  Parameters:     argv[1] - char [10] - Queue manager name               */
/*                  argv[2] - char [6]  - Output type      (*PRINT/*MSGQ)  */
/*                  argv[3] - char [4]  - Delete receivers (*YES/*NO)      */
/*                                                                         */
/* Copyright (c) 2003,2024 IBM Corp.                                       */
/*                                                                         */
/* Licensed under the Apache License, Version 2.0 (the "License");         */
/* you may not use this file except in compliance with the License.        */
/* You may obtain a copy of the License at                                 */
/*                                                                         */
/*    http://www.apache.org/licenses/LICENSE-2.0                           */
/*                                                                         */
/* Unless required by applicable law or agreed to in writing, software     */
/* distributed under the License is distributed on an "AS IS" BASIS,       */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*/
/* See the License for the specific language governing permissions and     */
/* limitations under the License.                                          */
/*                                                                         */
/***************************************************************************/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <stddef.h>
#include <qjournal.h>
#include <qusec.h>
#include <qusrtvus.h>
#include <qmhsndm.h> 
#include <ledate.h>

/***************************************************************************/
/*  Constants used by this program                                         */
/***************************************************************************/
#undef TRUE
#define TRUE 1
#undef FALSE
#define FALSE 0
#undef OK
#define OK 0        

#define ERR_ARGUMENTS_ERROR      1         /* List of error codes */
#define ERR_QMGR_LIB_NOT_FOUND   2         /* returned from main  */
#define ERR_USER_SPACE_MISSING   3
#define ERR_MEMORY_ERROR         4
#define ERR_RETRIEVE_JRN_INF     5

#define MAX_CHAIN 256         /* (Arbitrary) max. no of receivers in chain */
#define JRN_NAME "AMQAJRN   " /* Journal name                              */
#define SPC_NAME "AMQJRNINF " /* User Space Name                           */

/***************************************************************************/
/*  Function prototypes                                                    */
/***************************************************************************/
int GetReceiverChain(char* LibName, int *NumRcvrs,
                     Qjo_JN_Repeating_Key_1_Output_t *pRcvDirectory) ;
void output(char *print_buffer); /* Print data with a timestamp */

/***************************************************************************/
/*  Structure defining format of AMQJRNINF user space                      */
/***************************************************************************/
struct {
  char    JrnName[20];        /* Journal + library name */
  char    CC[2];              /* Year (century)         */
  char    YY[2];              /* Year                   */
  char    MM[2];              /* Month                  */
  char    DD[2];              /* Day                    */
  char    HH[2];              /* Hours                  */
  char    mm[2];              /* Minutes                */
  char    SS[2];              /* Seconds                */
  char    mmm[3];             /* Milliseconds           */
} OldestJrnEntry;

/***************************************************************************/
/*  Global variables                                                       */
/***************************************************************************/
int       print_output = TRUE;
char      message_queue[20];
int       delete_receivers = FALSE;
Qus_EC_t  ErrorData = { sizeof(Qus_EC_t), 0};


/***************************************************************************/
/*                    M A I N    F U N C T I O N                           */
/***************************************************************************/
int main(int argc, char **argv)
{
  int             retcode = OK,
  NumRcvrs = 0, 
  count = 0, 
  deletecount = 0,
  foundOldest = FALSE,
  deleteReceivers = FALSE,                  
  oldestReceiverIndex = 0,
  outputMessages = FALSE;
  char            cmdbuffer[200],
  print_buffer[200],
  QmgrLib[10],         /* Blank padded Qmgr library name */
  szQmgrLib[11],       /* NULL terminated Qmgr lib. name */
  *p,
  TimeStamp[13],
  AmqJrnIinfUsrSpc[20];
  Qjo_JN_Repeating_Key_1_Output_t   *pRcvDirectory = 0,
  *pTempRcvr = 0;  

  printf("\n\n");
  output("*****************************************************");
  output("*   Starting JrnMaint journal maintenance program   *");
  output("*****************************************************");

  /*************************************************************************/
  /* Check parameters & queue manager library are valid  & exit if not     */
  /*************************************************************************/
  if (argc != 4 ) {
    printf("ERROR: Invalid Arguments - pass a 10 character "
           "Queue manager library name, \n");
    retcode = ERR_ARGUMENTS_ERROR;
    return retcode;
  }

  /*************************************************************************/
  /* Set up blank padded & null terminated qmgr names and convert to       */
  /* upper case                                                            */
  /*************************************************************************/
  memcpy(QmgrLib, argv[1], 10);    
  for (p = QmgrLib; count<10 ; p++, count++) {          /* Upper case name */
    *p = toupper(*p);
  };

  memcpy(szQmgrLib, QmgrLib, 10);    
  p = strchr(szQmgrLib, ' ');                      /* Find trailing spaces */
  if (p) *p = '\0';                                /* and strip them off   */

  memcpy(AmqJrnIinfUsrSpc, SPC_NAME, 10);         /* Setup user space name */
  memcpy( &(AmqJrnIinfUsrSpc[10]), QmgrLib, 10);

  /*************************************************************************/
  /* Set ouput and deletion flags according to arguments 2 and 3           */
  /*************************************************************************/
  if ( ! memcmp(argv[2], "*PRINT", 6)) {
    print_output = TRUE;
  }
  else {
    print_output = FALSE;  
    memcpy( message_queue, "QMQMMSG   ", 10);
    memcpy( &(message_queue[10]), QmgrLib, 10);
  }

  if ( ! memcmp(argv[3], "*YES", 4))                 /*  Delete receivers? */
    delete_receivers = TRUE;
  else
    delete_receivers = FALSE;  

  /*************************************************************************/
  /* Check queue manager library exists                                    */
  /*************************************************************************/
  sprintf(cmdbuffer, "CHKOBJ %s *LIB", szQmgrLib );  
  if (system(cmdbuffer)) { /* Check library exists */
    printf("ERROR: Queue manager library %1.10s not found\n", szQmgrLib );
    retcode = ERR_QMGR_LIB_NOT_FOUND;
    return retcode;
  }

  /*************************************************************************/
  /* Get the timestamp of the oldest jrn entry from the AMQJRNINF *USRSPC  */
  /*************************************************************************/
  ErrorData.Bytes_Available = 0;
  ErrorData.Bytes_Provided =  sizeof(Qus_EC_t);
  QUSRTVUS( AmqJrnIinfUsrSpc, 1, sizeof(OldestJrnEntry), 
            (char*)&OldestJrnEntry, &ErrorData);

  /*************************************************************************/
  /* If we didn't find the user space we can't go any further.             */
  /*************************************************************************/
  if ( ErrorData.Bytes_Available != OK) {
    printf("ERROR: Journal user space (%s) not found in library %s.  "
           "Error code: %7.7s\n", SPC_NAME, szQmgrLib, 
           ErrorData.Exception_Id);
    retcode = ERR_USER_SPACE_MISSING;
    return retcode;
  };

  /*************************************************************************/
  /* Turn the timestamp into CYYMMDDHHMMSS so can compare it wih the rcvrs */
  /*************************************************************************/
  if (OldestJrnEntry.CC[0] == '1')
    TimeStamp[0] = '0'; /* 20th century */
  else
    TimeStamp[0] = '1'; /* 21st century */

  sprintf( &(TimeStamp[1]), "%2.2s%2.2s%2.2s%2.2s%2.2s%2.2s",
           OldestJrnEntry.YY, OldestJrnEntry.MM, OldestJrnEntry.DD, 
           OldestJrnEntry.HH, OldestJrnEntry.mm, OldestJrnEntry.SS);

  /*************************************************************************/
  /* Allocate the memory for rcvr data                                     */
  /*************************************************************************/
  pRcvDirectory = (Qjo_JN_Repeating_Key_1_Output_t*) 
                  calloc(sizeof(Qjo_JN_Repeating_Key_1_Output_t), MAX_CHAIN);
  if (! pRcvDirectory ) {
    printf("ERROR: Out of memory when allocating pRcvDirectory\n");
    retcode = ERR_MEMORY_ERROR;
    return retcode;
  };

  /*************************************************************************/
  /* Issue call to get the receiver chain                                  */
  /*************************************************************************/
  retcode = GetReceiverChain(QmgrLib, &NumRcvrs, pRcvDirectory);

  /*************************************************************************/
  /* If we got the data OK...                                              */
  /*************************************************************************/
  if (retcode == OK) {

    sprintf(print_buffer, "Timestamp of oldest journal entry: (%13.13s)", 
            TimeStamp);
    output(print_buffer);

    /***********************************************************************/
    /* Loop through the receivers in the receiver chain starting from the  */
    /* newest, looking for receivers that we can delete                    */
    /***********************************************************************/
    for (count=NumRcvrs-1; count >= 0 ;count--) {

      pTempRcvr = &(pRcvDirectory[count]);
      /*********************************************************************/
      /*  If this receiver was attached BEFORE the oldest journal entry... */
      /*********************************************************************/
      if ( memcmp(pTempRcvr->Jrn_Rcv_Att_Date_Time, TimeStamp,
                  sizeof(TimeStamp) ) < 0 ) {

        /*******************************************************************/
        /* If we haven't yet found the oldest receiver, yet then this must */
        /* be the oldest receiver we need to keep on the system            */
        /*******************************************************************/
        if ( foundOldest == FALSE ) {

          foundOldest = TRUE;
          oldestReceiverIndex = count;
          sprintf(print_buffer, 
                  "Keeping receiver: %10.10s attached at: %13.13s",
                  pTempRcvr->Jrn_Rcv_Name,
                  pTempRcvr->Jrn_Rcv_Att_Date_Time);
          output(print_buffer);
          sprintf(print_buffer, 
                  "** %10.10s is the oldest *JRNRCV that we need to keep **",
                  pTempRcvr->Jrn_Rcv_Name);
          output(print_buffer);

          /*****************************************************************/
          /*  If we're really deleting receivers we need to jump out of    */
          /*  this loop and drop into the next one to do the deletes       */
          /*****************************************************************/
          if ( delete_receivers == TRUE ) {
            break;         
          };

        }
        /*******************************************************************/
        /*  ...else, we've already seen the oldest journal receiver in the */
        /*  chain, but we're still in this loop... we're therefore not     */
        /*  actually deleting receivers - just report this one as eligible */
        /*  for deletion                                                   */
        /*******************************************************************/
        else {
          sprintf(print_buffer, 
                  "Receiver %10.10s attached at: %13.13s can be deleted",
                  pTempRcvr->Jrn_Rcv_Name, 
                  pTempRcvr->Jrn_Rcv_Att_Date_Time);
          output(print_buffer);
        };
      }

      /*********************************************************************/
      /*  Else this receiver was attached AFTER the oldest journal entry.  */
      /*********************************************************************/
      else {
        sprintf(print_buffer, 
                "Keeping receiver: %10.10s attached at: %13.13s",
                pTempRcvr->Jrn_Rcv_Name,
                pTempRcvr->Jrn_Rcv_Att_Date_Time);
        output(print_buffer);
      };

    }; /* End first for loop */


    /***********************************************************************/
    /*  If we're actually deleting receivers, then they must be deleted    */
    /*  oldest first.  So now we loop through starting from the oldest,    */
    /*  deleting all the receivers older than the one we need to keep      */
    /***********************************************************************/
    if ( delete_receivers == TRUE && foundOldest == TRUE ) {

      for (count=0; count < oldestReceiverIndex ;count++) {

        pTempRcvr = &(pRcvDirectory[count]);
        sprintf(print_buffer, 
                "Deleting Receiver %10.10s attached at: %13.13s",
                pTempRcvr->Jrn_Rcv_Name, 
                pTempRcvr->Jrn_Rcv_Att_Date_Time);
        output(print_buffer);

        /*******************************************************************/
        /*  Set up delete command and delete receiver                      */
        /*******************************************************************/
        memset( cmdbuffer, 0, sizeof(cmdbuffer) );
        sprintf( cmdbuffer, "DLTJRNRCV %s/%10.10s DLTOPT(*IGNINQMSG)",
                 szQmgrLib, pTempRcvr->Jrn_Rcv_Name);
        retcode = system(cmdbuffer);

        if ( retcode ) {
          printf("ERROR: %d from DLTJRNRCV.  See job log for details\n", 
                 retcode );
          break;
        };

        deletecount++;

      };

    };
  };

  /*************************************************************************/
  /*  Finished - Free the receiver directory if it was allocated           */
  /*************************************************************************/
  sprintf(print_buffer, 
          "JrnMaint finished - %d receiver(s) have been deleted.", 
          deletecount);
  output(print_buffer);

  if (pRcvDirectory) {
    free(pRcvDirectory);
  }
  return retcode;
}


/***************************************************************************/
/* Function:  output()                                                     */
/*                                                                         */
/* Description: Print messages preceded by a timestamp or send to          */
/*              the queue manager message queue depending on the           */
/*              OUTPUT() options specified                                 */
/*                                                                         */
/***************************************************************************/
void output(char *print_buffer) 
{
  char      localdatetime[23];
  _INT4     days;
  _FLOAT8   secs;
  _FEEDBACK fb;
  CEELOCT(&days, &secs, localdatetime, &fb);

  if (print_output) {
    printf("%2.2s:%2.2s:%2.2s-%s \n",
           &localdatetime[8],     /* Hours        */
           &localdatetime[10],    /* Minutes      */
           &localdatetime[12],    /* Seconds      */
           print_buffer);
  }
  else {

    ErrorData.Bytes_Available = 0;
    ErrorData.Bytes_Provided =  sizeof(Qus_EC_t);
    QMHSNDM( "       ",
             "                    ",
             print_buffer,          
             strlen(print_buffer),
             "*INFO     ",
             message_queue,
             1,
             "                    ",
             "    ",
             &ErrorData);
    
    /***********************************************************************/
    /* Report errors from send message                                     */
    /***********************************************************************/
    if ( ErrorData.Bytes_Available ) {
      printf("ERROR: %7.7s from QMHSNDM\n",
             &(ErrorData.Exception_Id) );
    };
  };
}

/***************************************************************************/
/*                                                                         */
/* Function:    GetReceiverChain                                           */
/*                                                                         */
/* Description: Get the list of receivers in the current  journal          */
/*              receiver chain                                             */
/*                                                                         */
/* Parameters:  JName         - 10 byte journal library name               */
/*              NumRcvrs      - Number of receivers returned               */
/*              pRcvDirectory - The buffer of journal receiver data        */
/*                                                                         */
/***************************************************************************/

int GetReceiverChain(char* LibName, int *NumRcvrs,
                     Qjo_JN_Repeating_Key_1_Output_t *pRcvDirectory) 
{
  Qjo_RJRN0100_t *pRtnJrnInf; 
  int             retcode = OK;
  long            JrnDataLen = 0;
  char          *JrnData,
  JrnName[20],  /* Jrn name (10) followed by lib name (10) */
  print_buffer[200];

  Qjo_JN_Repeating_Key_Fields_t *pKeyFld1Hdr = 
  (Qjo_JN_Repeating_Key_Fields_t*)0;
  Qjo_JN_Key_1_Output_Section_t *pKeyFld1OutputHdr = 
  (Qjo_JN_Key_1_Output_Section_t*)0;
  Qjo_JN_Repeating_Key_1_Output_t *pKeyFld1Data = 
  (Qjo_JN_Repeating_Key_1_Output_t*)0;

  struct KData {
    _INT4    KeyCount;        /* Count of all keys  (=1)   */
    _INT4    Len;             /* Length of this key  (=12) */
    _INT4    KeyID;           /* Key ID              (=1)  */
    _INT4    DataLen;         /* Length of data      (=0)  */
  } KeyData = {1,12,1,0};

  /*************************************************************************/
  /* Set up the qualified journal name                                     */
  /*************************************************************************/
  memcpy(JrnName, JRN_NAME,10);
  memcpy(&(JrnName[10]), LibName,10);

  /*************************************************************************/
  /* Set up the size of the receiver variable - all the headers            */
  /* plus the buffer for rcvrs                                             */
  /*************************************************************************/
  JrnDataLen =  sizeof(Qjo_RJRN0100_t) + 
                sizeof(Qjo_JN_Repeating_Key_Fields_t) + 
                sizeof(Qjo_JN_Key_1_Output_Section_t) + 
                sizeof(Qjo_RRCV0100_t) * MAX_CHAIN ;
  JrnData = (char*)malloc(JrnDataLen);
  if ( ! JrnData ) {
    printf("ERROR: Memory allocation error (JrnData)\n");
    retcode = ERR_MEMORY_ERROR;    
  };

  /*************************************************************************/
  /* Get the journal receiver chain                                        */
  /*************************************************************************/

  if (retcode == OK) {
    ErrorData.Bytes_Available = 0;
    ErrorData.Bytes_Provided =  sizeof(Qus_EC_t);
    QjoRetrieveJournalInformation(JrnData, &JrnDataLen,
                                  JrnName, QJO_RTV_JRN_INFO_0100,
                                  (char*)&KeyData, &ErrorData);

    /***********************************************************************/
    /* Print error data if the command failed                              */
    /***********************************************************************/
    if ( ErrorData.Bytes_Available ) {
      printf("ERROR: %7.7s from QjoRetrieveJournalInformation\n",
             &(ErrorData.Exception_Id) );
      if ((ErrorData.Bytes_Available - sizeof(ErrorData)) > 0)
        printf("- Error data:", (char*)&ErrorData + sizeof(ErrorData),
               ErrorData.Bytes_Available - sizeof(ErrorData));
      retcode = ERR_RETRIEVE_JRN_INF;      
    };
  };

  /*************************************************************************/
  /* if the API completed successfully, report the basic journal data      */
  /*************************************************************************/
  if (retcode == OK) {
    pRtnJrnInf = (Qjo_RJRN0100_t*)JrnData; 

    /***********************************************************************/
    /* Set up the receiver directory return fields                         */
    /* The journal receiver list is returned at the end of a list of       */
    /* chained structures - unscramble the jrnrcvr directory data          */
    /***********************************************************************/
    pKeyFld1Hdr = (Qjo_JN_Repeating_Key_Fields_t*)
                  ((char*)JrnData + sizeof(Qjo_RJRN0100_t));
    pKeyFld1OutputHdr = (Qjo_JN_Key_1_Output_Section_t*)
                        ((char*)pKeyFld1Hdr + 
                         pKeyFld1Hdr->Off_Strt_Key_Info);
    pKeyFld1Data = (Qjo_JN_Repeating_Key_1_Output_t*)
                   ((char*)pKeyFld1OutputHdr + 
                    sizeof(Qjo_JN_Key_1_Output_Section_t));

    *NumRcvrs = pKeyFld1Hdr->Number_Entries;
    sprintf(print_buffer, 
            "Processing %d receivers for AMQAJRN in %10.10s", 
            *NumRcvrs, LibName );
    output(print_buffer);
    
    if (*NumRcvrs > MAX_CHAIN) {
      sprintf(print_buffer, 
              "WARNING - More than %d receivers found"
              " - not all will be processed", MAX_CHAIN );
      output(print_buffer);
      *NumRcvrs= MAX_CHAIN;
    };
    
    memcpy(pRcvDirectory,  (void*)pKeyFld1Data, 
           *NumRcvrs * sizeof(Qjo_JN_Repeating_Key_1_Output_t));

    /***********************************************************************/
    /* Trace some of the returned data                                     */
    /***********************************************************************/
    sprintf(print_buffer, "Attached receiver name: %10.10s",
            pRtnJrnInf->Att_Jrn_Rcv_Name );
    output(print_buffer);
  }; /* End if retcode == OK */

  /*************************************************************************/
  /* Free the storage used to retrieve the data                            */
  /*************************************************************************/
  if (JrnData) {
    free(JrnData);
  };

  return retcode;
}

