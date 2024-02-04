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
* MFPMemFaultCmds.c
* MFP Memory Fault commands
*
* Author: Chang Lee
* Date: 12/29/2021
******************************************************************/
#include <fcntl.h>  /*  open    */
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/errno.h>
#include <dlfcn.h>

#include "Types.h"
#include "Debug.h"
#include "IPMIDefs.h"
#include "unix.h"
#include "EINTR_wrappers.h"
#include "featuredef.h"
#include "IPMI_MFPMemFault.h"

#define PIPE_READ_TIMEOUT	10
static int fdMFPMemFaultPipe = 0;

//#define SIMU_SCI_TRIG_SEND_FAULTS

#ifdef SIMU_SCI_TRIG_SEND_FAULTS
typedef void (*pPDKFunc) (void );
static int simulationTest = 0; 
static int simuFaultNum = 0;

static INT64U	SimuMemFaults[] = {0x18003000, 0x19000000, 0x18005000, 0x18007000, 0x19008000, 0x19002000, 0x19003000};

void *dl_pdkhandle = NULL;
static int pdkSCIFuncsInited = 0;
static int pdkSCIInited = 0;
static pPDKFunc pdkSCIInitFunc = NULL;
static pPDKFunc pdkSCITrigFunc = NULL;
#endif

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


static int checkPipeDataAvail(int fdPipe, struct timeval *pTimeout)
{
    fd_set	fdRead;

    FD_ZERO (&fdRead);
    FD_SET (fdPipe, &fdRead);

    return sigwrap_select (fdPipe + 1, &fdRead, NULL, NULL, pTimeout);
}

#ifdef SIMU_SCI_TRIG_SEND_FAULTS

static int getSCIPDKFuncs()
{	
    dl_pdkhandle = dlopen("/usr/local/lib/libscigen.so",RTLD_NOW);	/* Fortify [Process Control]:: False Positive */
    /* Reason for False Positive - the PDK library is loaded from internal file system only */
    if(!dl_pdkhandle) {
        TCRIT("Error in loading library %s\n",dlerror());
        return -1;
    }

    pdkSCIInitFunc = dlsym(dl_pdkhandle,"PDK_SCI_INIT");
    pdkSCITrigFunc = dlsym(dl_pdkhandle,"PDK_Trigger_SCI");
    if ( (pdkSCIInitFunc!=NULL) && (pdkSCITrigFunc!=NULL)) {
    	pdkSCIFuncsInited  =1;
    }
    else {
    	TCRIT("PDK SCI Function is NULL\n");
    	/* coverity[leaked_storage : FALSE] */
    	dlclose(dl_pdkhandle);
    	return -1;
    }
    /* coverity[leaked_storage : FALSE] */
	return 0;
}

static int triggerSci()
{
	int retVal = 0;
	if (!pdkSCIFuncsInited) {
		retVal = getSCIPDKFuncs();
		if (retVal) {
			TCRIT("PDK SCI Function erros, do not trigger SCI\n");
			return -1;
		}
	}
	
	if (!pdkSCIInited) {
		pdkSCIInitFunc();
		pdkSCIInited = 1;
	}	
	pdkSCITrigFunc();
	return 0;
}
#endif


int MFPMemFault(_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,int BMCInst)
{
    UN_USED(BMCInst);
    //UN_USED(ReqLen);
    MFPMemFaultReq_T  *pMFPMemFaultReq = (MFPMemFaultReq_T *)pReq;
    MFPMemFaultRes_T  *pMFPMemFaultRes = (MFPMemFaultRes_T *)pRes;
    INT8U ResLen = 1; /* Min. 1 (completioncode) */

    INT32U i, counts;
    FILE *fKey2 = NULL;
	struct stat info;
	int retVal, readByte = 0;
	struct timeval readTimeout;
	MFPMemFault_T fault_result[MAX_FAULT_ERR];
	size_t wr_length;
#if defined(DEBUG)
	struct addrTrParam_T addrTr = {0};
	MFPDumpPacket(pReq, ReqLen); 
#endif
	
	if( !pReq ) {
		TCRIT("%s() Request ptr: NULL\n", __FUNCTION__);
        goto END;
	}
	if( !pRes ) {
		TCRIT("%s() Response ptr: NULL\n", __FUNCTION__);
        goto END;
	}

    memset(pMFPMemFaultRes,0,sizeof(MFPMemFaultRes_T));
    pMFPMemFaultRes->completioncode=CC_UNSPECIFIED_ERR;


    switch(pMFPMemFaultReq->param) {
        /* Sub-command to collect fault memory info */
        case GET_MEM_FAULT:
            TINFO("sub-command: GET_MEM_FAULT\n");
            pMFPMemFaultRes->Respdata.faults.entries = 0;
        	ResLen += sizeof(INT8U); /* entries */

#ifdef SIMU_SCI_TRIG_SEND_FAULTS
        	if ( simulationTest ) {
        		TINFO("SIMU_SCI_TRIG_SEND_FAULTS, fault number=%d\n", simuFaultNum);
        		memcpy(pMFPMemFaultRes->Respdata.faults.phyAddr, SimuMemFaults, simuFaultNum*sizeof(INT64U));
        		pMFPMemFaultRes->Respdata.faults.entries = simuFaultNum;
        		ResLen += simuFaultNum*sizeof(INT64U);
        		pMFPMemFaultRes->completioncode=CC_SUCCESS;
        		simulationTest = 0;
        		simuFaultNum = 0;
        		break;
        	}
#endif

			if (fdMFPMemFaultPipe <= 0) {
				if (-1 == (fdMFPMemFaultPipe = sigwrap_open (MFPFAULTQUEUE, O_RDWR))) {
					TCRIT("cannot open MFP Fault Pipe \n");
					break;
				}
			}
			readTimeout.tv_sec = PIPE_READ_TIMEOUT;
			readTimeout.tv_usec = 0;
			retVal = checkPipeDataAvail(fdMFPMemFaultPipe, &readTimeout);
			if (retVal > 0) {
				/* Read a number of faults */
				sigwrap_read(fdMFPMemFaultPipe, &counts, sizeof (INT32U));
				TCRIT("MFP Fault Pipe has %d records\n", counts);
				readByte = sigwrap_read(fdMFPMemFaultPipe, (void *)fault_result, counts*sizeof(MFPMemFault_T));
				if ((int)(counts*sizeof(MFPMemFault_T)) != readByte) {
					TCRIT("Error! read byte(%d) is shorter than expected(%u) \n", readByte, counts*sizeof(MFPMemFault_T));
					counts = 0;
					break ;
				}
				for(i=0; i<counts; ++i){
					memcpy(&pMFPMemFaultRes->Respdata.faults.phyAddr[i], &fault_result[i].phy_addr, sizeof(INT64U));
				}
				ResLen  += counts * sizeof(INT64U);
				pMFPMemFaultRes->Respdata.faults.entries = counts;
			} 
			/* Even no data after timeout, bmc still returns a 0 entry to Bios to prevent ipmi error
			 * Bios will just ignore 0 entry */
			else if (retVal == 0) {
				TCRIT(" read %s time out %u seconds\n", MFPFAULTQUEUE, PIPE_READ_TIMEOUT);
			} else if( retVal < 0 ) {
				TCRIT("Pipe %s error\n", MFPFAULTQUEUE);
			}

            pMFPMemFaultRes->completioncode=CC_SUCCESS;
            break;

        case SET_ADDR_TRANS_PARAMS:
            TINFO("sub-command: SET_ADDR_TRANS_PARAMS\n");
            if (ReqLen != sizeof(pMFPMemFaultReq->param) + sizeof(struct addrTrParam_T)) {
				TCRIT(" Length error: %d != %lu \n",
					ReqLen, sizeof(pMFPMemFaultReq->param) + sizeof(struct addrTrParam_T));
            	pMFPMemFaultRes->completioncode = CC_REQ_INV_LEN;
                break;
            }
            TINFO("\tBase         : 0x%llx", pMFPMemFaultReq->reqData.addrTr.mMmCfgBase);
            TINFO("\tLimit        : 0x%llx", pMFPMemFaultReq->reqData.addrTr.mMmCfgLimit);
            TINFO("\tMmio L Base  : 0x%llx", pMFPMemFaultReq->reqData.addrTr.mPlatGlobalMmiolBase);
            TINFO("\tMmio L Limit : 0x%llx", pMFPMemFaultReq->reqData.addrTr.mPlatGlobalMmiolLimit);
            TINFO("\tMmio H Base  : 0x%llx", pMFPMemFaultReq->reqData.addrTr.mPlatGlobalMmiohBase);
            TINFO("\tMmio H Limit : 0x%llx", pMFPMemFaultReq->reqData.addrTr.mPlatGlobalMmiohLimit);
            TINFO("\tSocket BitMap: 0x%x", pMFPMemFaultReq->reqData.addrTr.mSocketPresentBitMap);
            TINFO("\tmTotCha      : 0x%x", pMFPMemFaultReq->reqData.addrTr.mTotCha);
            
            if( stat(TMP_DIR, &info)==0  && info.st_mode & S_IFDIR) {
        		fKey2 = fopen(ADDR_TRANS_INFO,"w"); /* no binary mode */
        		if(fKey2 == NULL) {
        			TCRIT("Unable to create %s\n", ADDR_TRANS_INFO);
                    break;
        		}
        		wr_length = fwrite(&pMFPMemFaultReq->reqData.addrTr, 1, sizeof(struct addrTrParam_T), fKey2);
        		if(wr_length != sizeof(struct addrTrParam_T)){
					TCRIT("writing %s error (written length %lu)\n",ADDR_TRANS_INFO, wr_length);
					pMFPMemFaultRes->completioncode = CC_UNSPECIFIED_ERR;
					break;
        		} else {
                	TINFO("%lu bytes are written to %s\n", wr_length, ADDR_TRANS_INFO);
        		}
            } else {
        		TCRIT("Error! \"%s\" folder is absent.\n", TMP_DIR);
        		break;
            }
            pMFPMemFaultRes->completioncode=CC_SUCCESS;
            break;
        case SIMULATE_SEND_MEM_FAULTS:
            TINFO("sub-command: SIMULATE_SEND_MEM_FAULTS\n");
            if (ReqLen != sizeof(pMFPMemFaultReq->param) + sizeof(INT8U)) {
            	pMFPMemFaultRes->completioncode = CC_REQ_INV_LEN;
                break;
            }
#ifdef SIMU_SCI_TRIG_SEND_FAULTS
            
            if (pMFPMemFaultReq->reqData.SimuMemFaultNum <= (INT8U)(sizeof(SimuMemFaults)/sizeof(SimuMemFaults[0]))) {
            	simuFaultNum = pMFPMemFaultReq->reqData.SimuMemFaultNum;
            	simulationTest = 1;
            	triggerSci();
            }
            else {
            	TCRIT("requested simulated faults number execeed limit\n");
            	pMFPMemFaultRes->completioncode = CC_INV_DATA_FIELD;
            	break;
            }
#else
            TINFO("SIMU_SCI_TRIG_SEND_FAULTS is disabled, do nothing. \n");
#endif
            pMFPMemFaultRes->completioncode=CC_SUCCESS;
        	break;
        	
        default:
        	pMFPMemFaultRes->completioncode = CC_PARAM_NOT_SUPPORTED;
        	break;
    }

END:
#ifdef DEBUG
if(fKey2) {
    	fclose(fKey2);
		fKey2 = fopen(ADDR_TRANS_INFO,"r");
		if(fKey2 == NULL) {
			printf("Unable to create %s\n", ADDR_TRANS_INFO);
 		}
    	wr_length = fread(&addrTr, 1, sizeof(struct addrTrParam_T), fKey2);
 		if(sizeof(struct addrTrParam_T) == wr_length )
		{
			TINFO("mMmCfgBase = 0x%llx, \t", addrTr.mMmCfgBase);
			TINFO("mMmCfgLimit = 0x%llx\n", addrTr.mMmCfgLimit);
			TINFO("mPlatGlobalMmiolBase = 0x%llx, \t", addrTr.mPlatGlobalMmiolBase);
			TINFO("mPlatGlobalMmiolLimit = 0x%llx\n", addrTr.mPlatGlobalMmiolLimit);
			TINFO("mPlatGlobalMmiohBase = 0x%llx, \t", addrTr.mPlatGlobalMmiohBase);
			TINFO("mPlatGlobalMmiohLimit = 0x%llx\n", addrTr.mPlatGlobalMmiohLimit);
			TINFO("mSocketPresentBitMap = 0x%x\n", addrTr.mSocketPresentBitMap);
			TINFO("mTotCha = 0x%x\n", addrTr.mTotCha);
		} else {
			TCRIT("Unable to read %s (read length = %lu)", ADDR_TRANS_INFO, wr_length);
			TINFO ("ls -l /var/tmp/addr_trans_params");
			system("ls -l /var/tmp/addr_trans_params");
		}
}
#endif

    if(fKey2) {
        fclose(fKey2);
    }

    return ResLen;
}
