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
* IPMI_MFPValid.h
* MFP Validation commands
*
* Author: Changjun Kang
* Date: 08/03/2020
******************************************************************/

#ifndef __IPMIAMIOEMMFPVALID_H__
#define __IPMIAMIOEMMFPVALID_H__

#define CMD_AMI_MFP_FPV				0xB9

#define MFP_VALIDATION				MFPValidation

#define MFP_VAL_START	0
#define MFP_VAL_DATA	1
#define MFP_VAL_STOP	2

#define MFP_VAL_START_DATA		0x55
#define MFP_VAL_STOP_DATA		0xAA


typedef struct {
	INT32U	timestamp;
    INT8U	socket;
    INT8U	imc;
    INT8U	channel;
    INT8U 	dimm;
    INT8U	rank;
    INT8U	device;
    INT8U	bg;
    INT8U	ba;
    INT32U	row;
    INT32U	column;
    INT8U	ue;
    uint32_t paritySyndrome;
    uint8_t mode;
} PACKED MFPValErr_T;

typedef struct {
	INT8U	cmd;
	INT8U	pnLen;
	INT8U	pn[32];
} PACKED MFPStartStop;

typedef struct {	
	INT8U	param;
    union   {
		MFPStartStop	START_STOP;					
		MFPValErr_T	data; 
    } reqData;
} PACKED MFPValReq_T;

typedef struct
{
	INT8U completioncode;
}PACKED MFPValRes_T;

int MFPValidation(_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,int BMCInst);

#endif
