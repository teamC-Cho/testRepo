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
* MFPValidationCmds.c
* MFP commands
*
* Author: Changjun Kang
* Date: 08/27/2020
******************************************************************/
#include <fcntl.h>  /*  open    */
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "Types.h"
#include "Debug.h"
#include "IPMIDefs.h"
#include "PDKAccess.h"
#include "unix.h"
#include "EINTR_wrappers.h"
#include "IPMI_MFPValid.h"
#include "mfp.h"
#include "mfp_ami.h"

static int fdMFPValPipe = 0;

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

static INT8U validateMFPValLog(MFPValErr_T *pErr)
{
	if (pErr != NULL) {
		if ((pErr->socket > SOCKET_MASK) || (pErr->imc > IMC_MASK) || (pErr->channel > IMC_BASE_CHANNEL_MASK) || (pErr->dimm > DIMM_MASK) || (pErr->rank > RANK_MASK)
				|| (pErr->device > DEVICE_MASK)||  (pErr->bg > BG_MASK) || (pErr->ba > BANK_MASK) || (pErr->row > ROW_MASK) || (pErr->column > COLUMN_MASK) || (pErr->ue >UE_MASK))
			return CC_INV_DATA_FIELD;
	}
	else {
		TCRIT("MFP Validation Error data pointer is NULL\n");
		return CC_REQ_INV_LEN;
	}
	return CC_SUCCESS;
}

static int mfpValIPMIDataToMFPValData(MFPValReq_T  *pIPMIValErr, mfpval_error *pValErr)
{
	if ( (pIPMIValErr == NULL) || (pValErr == NULL) ) {
		TCRIT(" pIPMIValErr == NULL) or pValErr == NULL");
		return -1;
	}
	if (pIPMIValErr->param == MFP_VAL_DATA) {
		pValErr->timestamp = pIPMIValErr->reqData.data.timestamp;
		pValErr->valerr.socket = pIPMIValErr->reqData.data.socket & SOCKET_MASK;
		pValErr->valerr.imc = pIPMIValErr->reqData.data.imc & IMC_MASK;
		pValErr->valerr.channel = pIPMIValErr->reqData.data.channel & IMC_BASE_CHANNEL_MASK;
		pValErr->valerr.dimm = pIPMIValErr->reqData.data.dimm & DIMM_MASK;
		pValErr->valerr.rank = pIPMIValErr->reqData.data.rank & RANK_MASK;
		pValErr->valerr.device = pIPMIValErr->reqData.data.device & DEVICE_MASK;
		pValErr->valerr.bank_group = pIPMIValErr->reqData.data.bg & BG_MASK;
		pValErr->valerr.bank = pIPMIValErr->reqData.data.ba & BANK_MASK;
		pValErr->valerr.row = pIPMIValErr->reqData.data.row & ROW_MASK;
		pValErr->valerr.col = pIPMIValErr->reqData.data.column & COLUMN_MASK;
		pValErr->valerr.error_type = pIPMIValErr->reqData.data.ue & UE_MASK;
#if defined (CONFIG_SPX_FEATURE_MFP_2_0)
		pValErr->valerr.par_syn = pIPMIValErr->reqData.data.paritySyndrome;
#elif defined CONFIG_SPX_FEATURE_MFP_2_1
		pValErr->valerr.par_syn = (uint64_t)(pIPMIValErr->reqData.data.paritySyndrome);
#endif
		pValErr->valerr.mode = pIPMIValErr->reqData.data.mode;
	}
	else {
		TCRIT("*pIPMIValErr is not MFP_VAL_DTAT");
		return CC_INV_DATA_FIELD;
	}
	return 0;
}

int MFPValidation(_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,int BMCInst)
{
    UN_USED(BMCInst);
    MFPValReq_T  *pMFPValReq = (MFPValReq_T *)pReq;
    MFPValRes_T  *pMFPValRes = (MFPValRes_T *)pRes;
    INT8U ResLen=1;
    mfpval_error compactErr = {0};
    FILE *fKey = NULL;
    struct stat info;

#if defined(DEBUG)
	MFPDumpPacket(pReq, ReqLen); 
#endif
    
    pMFPValRes->completioncode=CC_UNSPECIFIED_ERR;
    
    switch(pMFPValReq->param) {
            
        case MFP_VAL_START:
        /*
            if (ReqLen != sizeof(pMFPValReq->param) + sizeof(INT8U)) {
            	pMFPValRes->completioncode = CC_REQ_INV_LEN;
                goto END;
            } 	*/
            if ( pMFPValReq->reqData.START_STOP.cmd != MFP_VAL_START_DATA) {
            	pMFPValRes->completioncode = CC_INV_DATA_FIELD;
            	goto END;
            }

            if( stat(VARTMP_DIR, &info)==0  && info.st_mode & S_IFDIR) { 
				fKey = fopen(MFP_VAL_KEY,"wb");
				if(fKey == NULL) {
					TCRIT("Unable to create %s\n", MFP_VAL_KEY);
				}
				else {
#if defined (CONFIG_SPX_FEATURE_MFP_2_0) || defined (CONFIG_SPX_FEATURE_MFP_2_1)
					if (pMFPValReq->reqData.START_STOP.pnLen>0 && pMFPValReq->reqData.START_STOP.pnLen<=32) {
						if ( pMFPValReq->reqData.START_STOP.pnLen != fwrite(&pMFPValReq->reqData.START_STOP.pn, sizeof(INT8U), pMFPValReq->reqData.START_STOP.pnLen, fKey)) {
							TCRIT("writing %s error\n",MFP_VAL_KEY );
							fclose(fKey);
							pMFPValRes->completioncode = CC_UNSPECIFIED_ERR;
							goto END;
						}
					}
					else {
						TCRIT("PartNumber length %u invalid\n", pMFPValReq->reqData.START_STOP.pnLen);
						pMFPValRes->completioncode = CC_REQ_INV_LEN;
					}
#endif
					fclose(fKey);
					pMFPValRes->completioncode = CC_SUCCESS;
				}     	
            }
            else {
            	TCRIT("%s doesn't exist or not directoy\n", VARTMP_DIR);
            }           
            break;

        case MFP_VAL_DATA:
            if (ReqLen != sizeof(pMFPValReq->param) + sizeof(MFPValErr_T)) {
            	pMFPValRes->completioncode = CC_REQ_INV_LEN;
                goto END;
            }
    		pMFPValRes->completioncode = validateMFPValLog(&pMFPValReq->reqData.data);
    		if (pMFPValRes->completioncode == CC_SUCCESS) {
    			pMFPValRes->completioncode = mfpValIPMIDataToMFPValData(pMFPValReq, &compactErr); 
    			if (pMFPValRes->completioncode == CC_SUCCESS) {
					TDBG("time stamp=%u, socket=%x, row=0x%x, col=0x%x\n", compactErr.timestamp, compactErr.valerr.socket,
							compactErr.valerr.row, compactErr.valerr.col);
					if (fdMFPValPipe <= 0) {
						if (-1 == (fdMFPValPipe = sigwrap_open (MFPVALQUEUE, O_WRONLY))) {
							TCRIT("can not open MFPVAL Pipe \n");
							goto END;
						}
					}
					/* coverity[uninit_use_in_call : FALSE] */
					if (sizeof(compactErr) != sigwrap_write (fdMFPValPipe, &compactErr, sizeof (compactErr))) {
						TCRIT("writing MFPVAL Pipe fails\n");
					}
					else {
						TDBG("writing MFPVAL Pipe\n");
					}
    			}
    		}
            break;
        case MFP_VAL_STOP:
        	/*
            if (ReqLen != sizeof(pMFPValReq->param) + sizeof(INT8U)) {
            	pMFPValRes->completioncode = CC_REQ_INV_LEN;
                goto END;
            }
            */
            if ( pMFPValReq->reqData.START_STOP.cmd != MFP_VAL_STOP_DATA) {
            	pMFPValRes->completioncode = CC_INV_DATA_FIELD;
            	goto END;
            }
            if ( stat(MFP_VAL_KEY, &info) == 0 ) {
            	remove(MFP_VAL_KEY);
            }
            pMFPValRes->completioncode = CC_SUCCESS;
            break;
        default:
        	pMFPValRes->completioncode = CC_PARAM_NOT_SUPPORTED;
    }

END:
    return ResLen;
}
