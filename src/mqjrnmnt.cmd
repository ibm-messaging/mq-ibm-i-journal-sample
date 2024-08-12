/*******************************************************************/
/*                                                                 */
/* Command name: MQJRNMNT                                          */
/*                                                                 */
/* Description:  Manage journal receivers for a queue manager      */
/*                                                                 */
/* Copyright (c) 2003,2024 IBM Corp.                               */
/*                                                                 */
/* Licensed under the Apache License, Version 2.0 (the "License"); */
/* you may not use this file except in compliance with the License.*/
/* You may obtain a copy of the License at                         */
/*                                                                 */
/*    http://www.apache.org/licenses/LICENSE-2.0                   */
/*                                                                 */
/* Unless required by applicable law or agreed to in writing,      */
/* software distributed under the License is distributed on an     */
/* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,    */
/* either express or implied.                                      */
/* See the License for the specific language governing permissions */
/* and limitations under the License.                              */
/*                                                                 */
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
