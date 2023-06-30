/*
 *   $Id: //pg/RELEASE/31/FCS/external_demos/examples/include/error.h#1 $
 *
 *   $File: //pg/RELEASE/31/FCS/external_demos/examples/include/error.h $
 *   $Revision: #1 $
 *   $Change: 166877 $
 *   $Author: nigel $
 *   $DateTime: 2021/11/01 15:03:48 $
 *
 *   Description:
 *
 *      'Utility' file for Polygonica examples, to handle errors.
 *
 *   Copyright Notice:
 *
 *      $Copyright: MachineWorks Ltd. 1990-2020, 2021$
 *      All rights reserved.
 *
 *      This software and its associated documentation contains proprietary,
 *      confidential and trade secret information of MachineWorks Limited 
 *      and may not be (a) disclosed to third parties, (b) copied in any form,
 *      or (c) used for any purpose except as specifically permitted in 
 *      writing by MachineWorks Ltd.
 *
 *      This software is provided "as is" without express or implied
 *      warranty.
 */

#ifndef PG_ERROR_HDR
#define PG_ERROR_HDR

#include "pg/pgapi.h"
#include "pg/pgmacros.h"

#ifndef SHOW_ERROR_MESSAGE
#define SHOW_ERROR_MESSAGE print_line
static void print_line(LPCWSTR /*char**/ line) 
{
wprintf("%s\n", line);
}

#endif /* SHOW_ERROR_MESSAGE */

/* 
 * This function is set as PV_ENV_PROP_ERROR_REPORT_CB for most examples 
 */ 
static void simple_error_callback(
   PTStatus status,
   LPCWSTR /*char    **/err_string
   )
{ 
   static struct __status_name {
      PTStatus value;
      char * name;
   } status_names[] = {
      { PV_STATUS_BAD_CALL, "Bad Call" },
      { PV_STATUS_MEMORY, "Memory" },
      { PV_STATUS_EXCEPTION, "Exception" },
      { PV_STATUS_FILE_IO, "File IO" },
      { PV_STATUS_INTERRUPT, "Interrupt" },
      { PV_STATUS_INTERNAL_ERROR, "Internal Error" },
      { PV_STATUS_UNREGISTERED_THREAD, "Unregistered Thread" },
      { PV_STATUS_NOT_COMPLETE, "Not Complete" }
   };

   if ((status == PV_SOCH_ERR_NOT_LICENSED) ||
       (status == PV_WCC_ERR_NOT_LICENSED) ||
       (status == PV_SO_ERR_NOT_LICENSED) ||
       (status == PV_SOSO_ERR_NOT_LICENSED) ||
       (status == PV_SSP_ERR_NOT_LICENSED) ||
       (status == PV_SGSP_ERR_NOT_LICENSED) ||
       (status == PV_SOFSI_ERR_NOT_LICENSED) ||
       (status == PV_SOS_ERR_NOT_LICENSED) ||
       (status == PV_SOPIS_ERR_NOT_LICENSED) ||
       (status == PV_SUCL_ERR_NOT_LICENSED) ||
       (status == PV_SCLWS_ERR_NOT_LICENSED) ||
       (status == PV_SOCM_ERR_NOT_LICENSED) ||
       (status == PV_PCAS_ERR_NOT_LICENSED) ||
       (status == PV_PGDTS_ERR_NOT_LICENSED) ||
       (status == PV_SCWP_ERR_NOT_LICENSED) ||
       (status == PV_CTCR_ERR_NOT_LICENSED) ||
       (status == PV_SCSL_ERR_NOT_LICENSED) ||
       (status == PV_PCCB_ERR_NOT_LICENSED) ||
       (status == PV_RF_ERR_NOT_LICENSED))
   {
      /* Only print license messages to window */
      SHOW_ERROR_MESSAGE(err_string);
   }
   else 
   {
      PTStatus status_code;
      PTStatus err_code;
      PTStatus func_code;
      int i;

      /* The status is made up of 3 parts */
      status_code = PM_STATUS_FROM_API_ERROR_CODE(status);
      func_code = PM_FN_FROM_API_ERROR_CODE(status);
      err_code = PM_ERR_FROM_API_ERROR_CODE(status);
      /* Print other messages to console */
      for (i = 0; i < sizeof(status_names) / sizeof(status_names[0]); i++)
      {
         if (status_code & status_names[i].value)
         {
            printf("PG:%s: Function %d Error %d\n", status_names[i].name, func_code, err_code);
         }
      }
      printf ("   \"%s\"\n", err_string);
      fflush (stdout);
   }
}

/* 
 * This function is called if PFInitialise fails for most examples 
 */ 
static void handle_license_error (PTStatus status)
{ 
   switch (status)
   {
   case PV_INIT_HASP_CANT_FIND_DLL :
      printf("PFInitialise: HASP license cannot load DLL.\n");
      break;
   case PV_INIT_HASP_NO_LOCK :
      printf("PFInitialise: HASP license no lock.\n");
      break;
   case PV_INIT_HASP_NOT_AUTHORISED :
      printf("PFInitialise: HASP license not authorised.\n");
      break;
   case PV_INIT_LICENSE_NOT_SET :
      printf("PFInitialise: License not set.\n");
      break;
   case PV_INIT_LICENSE_BAD_VERSION :
      printf("PFInitialise: License for bad version.\n");
      break;
   case PV_INIT_LICENSE_BAD_OS :
      printf("PFInitialise: License for different OS.\n");
      break;
   case PV_INIT_LICENSE_LIB_NOT_FOUND :
      printf("PFInitialise: License lib not found.\n");
      break;
   case PV_INIT_LICENSE_LIB_FAILED :
      printf("PFInitialise: License lib failed.\n");
      break;
   case PV_INIT_LICENSE_SERVER_FAILED :
      printf("PFInitialise: License server not connected.\n");
      break;
   case PV_INIT_LICENSE_EXPIRED :
      printf("PFInitialise: License expired.\n");
      break;
   case PV_INIT_LICENSE_INVALID  :
      printf("PFInitialise: License invalid.\n");
      break;
   case PV_INIT_LICENSE_BAD_VM :
      printf("PFInitialise: License bad VM.\n");
      break;
   case PV_INIT_LICENSE_BAD_HOST :
      printf("PFInitialise: License bad host.\n");
      break;
   case PV_INIT_LICENSE_SERVER_COMMS :
      printf("PFInitialise: License server communications failed.\n");
      break;
   case PV_INIT_LICENSE_WINDBACK :
      printf("PFInitialise: License detected clock windback.\n");
      break;
   case PV_INIT_LICENSE_CANT_GET_DATE :
      printf("PFInitialise: License can't get date.\n");
      break;
   case PV_INIT_LICENSE_AUTH_FAILED :
      printf("PFInitialise: License authorisation failed.\n");
      break;
   case PV_INIT_LICENSE_ERROR :
      printf("PFInitialise: License error.\n");
      break;
   case PV_INIT_INVALID_MEMORY_CBACK :
      printf("PFInitialise: Optional memory callback invalid\n");
      break;
   default:
      {
         PTStatus status_code;
         PTStatus err_code;

         /* The status is made up of 3 parts - func code will be PV_FUNC_INITIALISE */
         status_code = PM_STATUS_FROM_API_ERROR_CODE(status);
         err_code = PM_ERR_FROM_API_ERROR_CODE(status);

         printf("PFInitialise: Status %d, Error %d\n", status_code, err_code);
      }
      break;
   }
   fflush (stdout);
}

#endif /* PG_ERROR_HDR */
