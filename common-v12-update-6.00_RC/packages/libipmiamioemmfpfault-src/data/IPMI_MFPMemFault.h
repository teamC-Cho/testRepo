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
* IPMI_MFPMemFault.h
* MFP Memory Fault commands
*
* Author: Chang Lee
* Date: 12/29/2021
******************************************************************/

#ifndef __IPMIAMIOEMMFPMEMFAULT_H__
#define __IPMIAMIOEMMFPMEMFAULT_H__
#include "mfp.h"
#include "mfp_ami.h"

//IPMI OEM Command 
#define CMD_AMI_GET_MEM_FAULT	0xB2
#define MFP_MEM_FAULT           MFPMemFault

// Subcommands
#define GET_MEM_FAULT	      0x0 
#define SET_ADDR_TRANS_PARAMS 0x1 
#define SIMULATE_SEND_MEM_FAULTS 0x2

// Fault Types
#define ROW_FAULT	0
#define COL_FAULT	1
#define BANK_FAULT	2

typedef unsigned long long INT64U;

typedef struct mfp_mem_fault_t MFPMemFault_T; 

// IPMI Request 
typedef struct {	
	INT8U	param;	//Subcommand.
	union {
		struct addrTrParam_T addrTr;
		INT8U	SimuMemFaultNum;
	}reqData;
} PACKED MFPMemFaultReq_T;

typedef struct {
	INT8U	entries; //number of faults
	INT64U	phyAddr[MAX_FAULT_ERR];
} PACKED MFPMemFaults_T;

// IPMI Response
typedef struct {
	INT8U completioncode;
	union {
		/* for GET_MEM_FAULT_DIMM or for GET_ALL_MEM_FAULT,
		 * use entries followed by entries*sizeof(MFPMemFault_T) bytes of data;
		*/
		MFPMemFaults_T faults;
	} Respdata;
} PACKED MFPMemFaultRes_T;

int MFPMemFault(_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,int BMCInst);

#endif /* __IPMIAMIOEMMFPMEMFAULT_H__ */
