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
#include "IPMIConf.h"

IPMILibMetaInfo_T g_MFPMemFaultMetaInfo[] =
{
    {
        "CONFIG_SPX_FEATURE_MFP",
        "g_MFPMemFault_CmdHndlr",
        "",
        ADDTO_EXT_CMD_HNDLR_TBL,// MsgHndlr-Type
        NETFN_AMI,// NetFn
        0,
        NULL
    },
    {
        "CONFIG_SPX_FEATURE_MFP",
        "g_MFPMemFault_CmdSelectHndlr",
        "",
        ADDTO_CMD_SELECT_EXT_HNDLR_TBL,
        NETFN_AMI,// NetFn
        0,
        NULL
    }

};

/**
 ** @fn GetLibMetaInfo
 **
 ** @brief To Get the address of Meta Information table
 **        in the library.
 ** 
 ** @param pMetaInfo Point to Meta Information table.
 ** 
 ** @return RETURN The number of elements in the Meta 
 ** Information table.
*/
INT8U GetLibMetaInfo (IPMILibMetaInfo_T **pMetaInfo)
{
    *pMetaInfo = &g_MFPMemFaultMetaInfo[0];
    // return the argument count
    return (sizeof(g_MFPMemFaultMetaInfo) / sizeof(g_MFPMemFaultMetaInfo[0]));
}

