/*****************************************************************
******************************************************************
***                                                            ***
***        (C)Copyright 2020, American Megatrends Inc.         ***
***                                                            ***
***                    All Rights Reserved                     ***
***                                                            ***
***       5555 Oakbrook Parkway, Norcross, GA 30093, USA       ***
***                                                            ***
***                     Phone 770.246.8600                     ***
***                                                            ***
******************************************************************
******************************************************************
******************************************************************
*
* MFPValMetaInfo.c
* MFP commands
*
* Author: Changjun Kang
* Date: 08/27/2020
******************************************************************/

#include "Types.h"
#include "MsgHndlr.h"
#include "Support.h"
#include "IPMI_AMI.h"
#include "AMIConf.h"
#include "cmdselect.h"
#include "IPMI_MFPValid.h"

const ExCmdHndlrMap_T g_MFPVal_CmdHndlr[] =
{
    { CMD_AMI_MFP_FPV,    PRIV_ADMIN,       MFP_VALIDATION,  0xFF,   0xAAAA, 0xFFFF  },
    {0x00,0x00,0x00,0x00,0x00,0x00}
};

const NetFnCmds_T g_MFPVal_CmdSelectHndlr[] =
{
    { CMD_AMI_MFP_FPV,    ENABLED,    NONE },
    {0x00,0x00,0x00}
};

