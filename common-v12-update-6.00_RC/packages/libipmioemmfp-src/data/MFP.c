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
* MFPMetaInfo.c
* MFP commands
*
* Author: Changjun Kang
* Date: 08/03/2020
******************************************************************/

#include "Types.h"
#include "MsgHndlr.h"
#include "Support.h"
#include "IPMI_AMI.h"
#include "AMIConf.h"
#include "cmdselect.h"
#include "IPMI_MFP.h"

const ExCmdHndlrMap_T g_MFP_CmdHndlr[] =
{
    { CMD_INTEL_SET_MFP,    PRIV_ADMIN,       INTEL_SET_MFP,  0xFF,   0xAAAA, 0xFFFF  },
    {0x00,0x00,0x00,0x00,0x00,0x00}
};

const NetFnCmds_T g_MFP_CmdSelectHndlr[] =
{
    { CMD_INTEL_SET_MFP,    ENABLED,    NONE },
    {0x00,0x00,0x00}
};

