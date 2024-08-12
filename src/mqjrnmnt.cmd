/*******************************************************************/
/*                                                                 */
/* Command name: MQJRNMNT                                          */
/*                                                                 */
/* Description:  Manage journal receivers for a queue manager      */
/*                                                                 */
/*               (c) Copyright IBM Corporation 2003                */
/*******************************************************************/

 MQJRNMNT:   CMD        PROMPT('MQ Journal maintenance')

             PARM       KWD(QMGRLIB) TYPE(*NAME) LEN(10) MIN(1) +
                          PROMPT('Queue manager library name')
             PARM       KWD(OUTPUT) TYPE(*CHAR) LEN(6) RSTD(*YES) +
                          DFT(*PRINT) VALUES(*PRINT *MSGQ) +
                          PROMPT('Output type')
             PARM       KWD(DLTRCV) TYPE(*CHAR) LEN(4) RSTD(*YES) +
                          DFT(*NO) VALUES(*YES *NO) PROMPT('Delete +
                          receivers')
                          
/*******************************************************************/

