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
* IPMI_MFP.h
* MFP commands
*
* Author: Changjun Kang
* Date: 08/03/2020
******************************************************************/

#ifndef __IPMIOEMMFP_H__
#define __IPMIOEMMFP_H__

#define CMD_INTEL_SET_MFP			0x45

#define INTEL_SET_MFP				IntelSetMFP

/* *********************************************************
 * Memory Error Format for bios, kept same as MRT 1.0 format 
 * suggested by Intel. Starting MRT2.0, this format is only used 
 * for UCE sent by BIOS. CE is collected in libmfpcollect 
 * through PECI
 * *********************************************************/
typedef struct {
    INT8U	socket;
    INT8U	imc;
    INT8U	channel;
    INT8U 	dimm;
    INT8U	rank;
    INT8U	bg;
    INT8U	ba;
    INT32U	row;
    INT32U	column;
    INT8U	ue;
    INT32U	syndrome;
} PACKED MFPReqBIOS_T;

/***************************************************************
 * This format is used in ipmitool unit test, so keep its format 
 * aligned with struct mfp_error in mfp.h
 ************************************************************/
typedef struct {
    INT8U	socket;
    INT8U	imc;
    INT8U	channel;
    INT8U 	dimm;
    INT8U	rank;
    INT8U 	device;
    INT8U	bg;
    INT8U	ba;
    INT32U	row;
    INT32U	column;
    INT8U	ue;
    INT32U	syndrome;
    INT8U	mode;
} PACKED MFPReqUT_T;

typedef struct
{
	INT8U completioncode;
}PACKED MFPRes_T;

int IntelSetMFP(_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,int BMCInst);

#endif
