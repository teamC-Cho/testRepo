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
#include "IPMIConf.h"

IPMILibMetaInfo_T g_MFPMetaInfo[] =
{
    {
        "CONFIG_SPX_FEATURE_MFP",
        "g_MFP_CmdHndlr",
        "",
        ADDTO_EXT_CMD_HNDLR_TBL,// MsgHndlr-Type
        NETFN_30,// NetFn
        0,
        NULL
    },
    {
        "CONFIG_SPX_FEATURE_MFP",
        "g_MFP_CmdSelectHndlr",
        "",
        ADDTO_CMD_SELECT_EXT_HNDLR_TBL,
        NETFN_30,// NetFn
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
    *pMetaInfo = &g_MFPMetaInfo[0];
    // return the argument count
    return (sizeof(g_MFPMetaInfo) / sizeof(g_MFPMetaInfo[0]));
}

