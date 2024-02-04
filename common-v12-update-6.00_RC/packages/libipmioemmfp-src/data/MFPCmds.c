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
* MFPCmds.c
* MFP commands
*
* Author: Changjun Kang
* Date: 08/03/2020
******************************************************************/
#include <fcntl.h>  /*  open    */
#include <string.h>
#include <stdlib.h>
#include "Types.h"
#include "Debug.h"
#include "IPMIDefs.h"
#include "PDKAccess.h"
#include "unix.h"
#include "EINTR_wrappers.h"
#include "IPMI_MFP.h"
#include "mfp.h"
#include "mfp_ami.h"

static int fdMFPPipe = 0;

#if defined(DEBUG)
static void MFPDumpPacket (void *DataPkt, int Len)
{
    char    *Pkt    = (char *)(DataPkt);
    int     index   = 0;
    int     count   = 0;

    printf("%s: Dumping the Packet of length %d:\n", __FILE__, Len);
    printf("==================\n");

    do
    {
        printf("%02x ", Pkt[index]);
        count++;

        if ((count == 16) && (index >= 0))
        {
            count = 0;
            printf("\n");
        }

        index++;

    } while (index < Len);
    printf("\n\n");
    return;
}
#endif

static INT8U validateMFPLog(INT8U *pReq, INT32U length)
{
	MFPReqBIOS_T *pBios;
	MFPReqUT_T *pUT;
	if (pReq != NULL) {
		if ( length == sizeof(MFPReqBIOS_T)) {
			pBios = (MFPReqBIOS_T *)pReq;
			TCRIT("Req MFPReqBIOS_T\n");
			if ((pBios->socket > SOCKET_MASK) || (pBios->imc > IMC_MASK) || (pBios->channel > SOCKET_BASE_CHANNEL_MASK) || (pBios->dimm > DIMM_MASK) || (pBios->rank > RANK_MASK)
					||  (pBios->bg > BG_MASK) || (pBios->ba > BANK_MASK) || (pBios->row > ROW_MASK) || (pBios->column > COLUMN_MASK) || (pBios->ue >UE_MASK))
			{
				TCRIT("Error! Invalid BIOS data\n");
				return CC_INV_DATA_FIELD;
			}
		}
		else if ( length == sizeof(MFPReqUT_T) ) {
			pUT = (MFPReqUT_T *)pReq;
			TCRIT("Req MFPReqUT_T\n");
			if ((pUT->socket > SOCKET_MASK) || (pUT->imc > IMC_MASK) || (pUT->channel > IMC_BASE_CHANNEL_MASK) || (pUT->dimm > DIMM_MASK) || (pUT->rank > RANK_MASK)
					||  (pUT->device > DEVICE_MASK) ||  (pUT->bg > BG_MASK) || (pUT->ba > BANK_MASK) || (pUT->row > ROW_MASK) || (pUT->column > COLUMN_MASK) || (pUT->ue >UE_MASK))
			{
				TCRIT("Error! Invalid UT data\n");
				return CC_INV_DATA_FIELD;
			}
		}
		else {
			TCRIT("Unknown Memory Error Format\n");
			return CC_UNSPECIFIED_ERR;
		}
	}
	else {
		TCRIT("MFP Request data pointer is NULL\n");
		return CC_REQ_INV_LEN;
	}
	return CC_SUCCESS;
}

static int mfpIPMIToMFP(INT8U  *pIPMIErr, INT32U length, struct mfp_error *pMFPErr)
{
	MFPReqBIOS_T *pBios;
	MFPReqUT_T *pUT;
	if ( (pIPMIErr == NULL) || (pMFPErr == NULL) ) {
		TCRIT(" pIPMIErr == NULL) or pMFPError == NULL");
		return -1;
	}
	/* Missing fields are filled with 0  */
	if ( length == sizeof(MFPReqBIOS_T)) {
		pBios = (MFPReqBIOS_T *)pIPMIErr; 
		pMFPErr->socket = pBios->socket & SOCKET_MASK;
		pMFPErr->imc = pBios->imc & IMC_MASK;
		pMFPErr->channel = pBios->channel%2;   //Convert socket based chan number to imc based chan number
		pMFPErr->dimm = pBios->dimm & DIMM_MASK;
		pMFPErr->rank = pBios->rank & RANK_MASK;
		pMFPErr->device = 0;       //fill with 0
		pMFPErr->bank_group = pBios->bg & BG_MASK;
		pMFPErr->bank = pBios->ba & BANK_MASK;
		pMFPErr->row = pBios->row & ROW_MASK;
		pMFPErr->col = pBios->column & COLUMN_MASK;
		pMFPErr->error_type = pBios->ue & UE_MASK;
	#if defined (CONFIG_SPX_FEATURE_MFP_2_0)
		pMFPErr->par_syn = pBios->syndrome;
	#elif defined (CONFIG_SPX_FEATURE_MFP_2_1)
		pMFPErr->par_syn = (uint64_t )(pBios->syndrome);
	#endif
		//pMFPErr->par_syn = 0;     //fill with 0
		pMFPErr->mode = 0;        //fill with 0
	}
	else if ( length == sizeof(MFPReqUT_T) ) {
		pUT = (MFPReqUT_T *)pIPMIErr;
		pMFPErr->socket = pUT->socket & SOCKET_MASK;
		pMFPErr->imc = pUT->imc & IMC_MASK;
		pMFPErr->channel = pUT->channel & IMC_BASE_CHANNEL_MASK;
		pMFPErr->dimm = pUT->dimm & DIMM_MASK;
		pMFPErr->rank = pUT->rank & RANK_MASK;
		pMFPErr->device = pUT->device & DEVICE_MASK;
		pMFPErr->bank_group = pUT->bg & BG_MASK;
		pMFPErr->bank = pUT->ba & BANK_MASK;
		pMFPErr->row = pUT->row & ROW_MASK;
		pMFPErr->col = pUT->column & COLUMN_MASK;
		pMFPErr->error_type = pUT->ue & UE_MASK;
	#if defined (CONFIG_SPX_FEATURE_MFP_2_0)
		pMFPErr->par_syn = pUT->syndrome;
	#elif defined (CONFIG_SPX_FEATURE_MFP_2_1)
		pMFPErr->par_syn = (uint64_t )(pUT->syndrome);
	#endif
		pMFPErr->mode = (uint32_t)(pUT->mode);
	}

	return 0;
}

int IntelSetMFP(_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,int BMCInst)
{
	UN_USED(BMCInst);
//	MFPReq_T  *pMFPReq = (MFPReq_T *)pReq;
	MFPRes_T  *pMFPRes = (MFPRes_T *)pRes;
	INT8U ResLen=1;
	struct mfp_error compactErr;

#if defined(DEBUG)
	MFPDumpPacket(pReq, ReqLen);    
#endif
	
    if ( (ReqLen!=sizeof(MFPReqBIOS_T)) && (ReqLen!=sizeof(MFPReqUT_T)) ) {
    	if(ReqLen != sizeof(MFPReqBIOS_T)) {
	    	TCRIT("MFPReqBIOS_T length mismatch! %u %u\n", ReqLen, sizeof(MFPReqBIOS_T));
    	} else if(ReqLen != sizeof(MFPReqUT_T)) {
	    	TCRIT("MFPReqUT_T length mismatch! %u %u\n", ReqLen, sizeof(MFPReqUT_T));
    	}
    	pMFPRes->completioncode = CC_REQ_INV_LEN;
	}
    else {   	
		pMFPRes->completioncode = validateMFPLog(pReq, ReqLen);
		if (pMFPRes->completioncode == CC_SUCCESS) {
			mfpIPMIToMFP(pReq, ReqLen, &compactErr);
			if (fdMFPPipe <= 0) {
				if (-1 == (fdMFPPipe = sigwrap_open (MFPQUEUE, O_WRONLY))) {
					TCRIT("can not open MFP Pipe \n");
					goto End;
				}
			}
		    if (sizeof(compactErr) != sigwrap_write (fdMFPPipe, &compactErr, sizeof (compactErr))) {
		    	TCRIT("writing MFP Pipe fails\n");
		    }
		    else {
		    	TDBG("writing MFP Pipe\n");
		    }
		}
    }

End:
	return ResLen;
}

