/*****************************************************************
******************************************************************
***                                                            ***
***        (C)Copyright 2021, American Megatrends Inc.         ***
***                                                            ***
***                    All Rights Reserved                     ***
***                                                            ***
***       3095 Satellite Blvd, Duluth, GA 30096, USA           ***
***                                                            ***
***                     Phone 770.246.8600                     ***
***                                                            ***
******************************************************************
******************************************************************
******************************************************************
*
* MFPMemFaultMetaInfo.c
* MFP Memory Fault commands
*
* Author: Chang Lee
* Date: 12/29/2021
******************************************************************/

#include "Types.h"
#include "MsgHndlr.h"
#include "Support.h"
#include "IPMI_AMI.h"
#include "AMIConf.h"
#include "cmdselect.h"
#include "IPMI_MFPMemFault.h"

const ExCmdHndlrMap_T g_MFPMemFault_CmdHndlr[] =
{
    { CMD_AMI_GET_MEM_FAULT,    PRIV_ADMIN,       MFP_MEM_FAULT,  0xFF,   0xAAAA, 0xFFFF  }, /* ReqLen=0xFF: Any Length */
    {0x00,0x00,0x00,0x00,0x00,0x00}
};

const NetFnCmds_T g_MFPMemFault_CmdSelectHndlr[] =
{
    { CMD_AMI_GET_MEM_FAULT,    ENABLED,    NONE },
    {0x00,0x00,0x00}
};

