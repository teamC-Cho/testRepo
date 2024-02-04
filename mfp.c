/******************************************************************
 ******************************************************************
 ***                                                             **
 ***    (C)Copyright 2020, American Megatrends Inc.             **
 ***                                                             **
 ***    All Rights Reserved.                                     **
 ***                                                             **
 ***    5555 , Oakbrook Pkwy, Norcross,                          **
 ***                                                             **
 ***    Georgia - 30093, USA. Phone-(770)-246-8600.              **
 ***                                                             **
 ******************************************************************
 ******************************************************************
 ******************************************************************
 *
 * mfp.c
 * memory failure prediction main function
 *
 * Author: Changjun Kang <changjunk@ami.com>
 ******************************************************************/

/******************************************************************
 1.	MRT 1.0
  *	All memory errors
	-	BIOS -> IPMI(libipmioemmfp) -> BMC MFP main()
	-	Used pipe: MFPQUEUE "/var/MFPQUEUE"
	-	BIOS collects all memory error and sends it to BMC computeMFPThread() thread via IPMI

 2.	MRT 2.0
  * Include MFP 1.0 features
  *	Uncorrectable error
	-	BIOS -> IPMI (libipmiamioemmfpvalid) -> BMC MFP main()
	-	Used pipe: MFPVALQUEUE "/var/MFPVALQUEUE"
	-	BIOS collects uncorrectable error and sends it to BMC computeMFPThread() thread via IPMI
  *	Correctable error
	-	BMC mfp2Thread() thread collects correctable memory error data
	-	BIOS is not involved

 3.	MRT 2.1 & MRT 3.0
  * Include MFP 1.0 features
  * Include MFP 2.0 features
  *	Page-offlining Scenario:
    3.1) BMC colMemFaultThread() monitors DIMM fault info by calling mfp_stat()
	3.2) BMC colMemFaultThread() converts DIMM info to physical address when a fault is detected
	3.3) BMC colMemFaultThread() triggers a SCI to indicates new memory fault is available
	3.4) BIOS sends the IPMI request to get the memory fault info
	3.5) BMC main() monitors a FIFO pipe (MFPFAULTQUEUE) &&
	     BMC sends the collected info to the IPMI
 	-	BMC SCI -> BIOS
	-	BIOS -> IPMI(libipmiamioemmfpfault) -> BMC colMemFaultThread

 4.	MRT 3.1
  * Include MFP 1.0 features
  * Include MFP 2.0 features
  * Include MRT 2.1 & MRT 3.0 feature
  *	High Bandwidth Memory (HBM) Memory-Error-Collector is added

 5. Channel number index
  * Channel number can be socket based or imc based
  * Hereafter CPUs is assumed as ICX. Keep in mind, different CPU has 
  * different topology.
  * For ICX topology, each socket has 4 IMCs, each of which has 2 channels
  * Each channel has 2 DIMM slots.
  * 									              ICX CPU
  * 										             |
  *                 ---------------------------------------------------------------------------------
  *                 IMC0                   IMC1                     IMC2                         IMC3
  *          -----------------       ------------------         ------------------          -------------------
  *         CH0            CH1       CH2            CH3         CH4            CH5          CH6             CH7
  *    -----------    ----------   ---------   -------------  ---------     ------------  -----------    -----------        
  *    DI0     DI1    DI2    DI3   DI4   DI5   DI6       DI7  DI8   DI9     DI10    DI11  DI12   DI13    DI14   DI15  
  *
  * Therefore, socket based channel number ranges 0 ~ 7,
  * IMC based channel number ranges 0 ~ 1
  * 
  * Socket-Based-Channel-Number = IMC*2 + IMC-Based-Channel-Number
  * IMC-Based-Channel-Number = Socket-Based-Channel-Number %2 
  * 
  * The channel number from host inventory and UCE ipmi mfp error data
  * is socket based
  * 
  * MFP is using IMC based channel number
  * The channel number of CE collection is IMC based
******************************************************************/

#include <fcntl.h>  /*  open    */
#include <dirent.h>
#include <dlfcn.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/errno.h>
#include <signal.h>
#include <sys/time.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <sys/select.h>
#include "Types.h"
#include "dbgout.h"
#include "unix.h"
#include "procreg.h"
#include "EINTR_wrappers.h"
#include "mfp.h"
#include "mfp_ami.h"
#include "hiredis.h"
#include<sys/prctl.h>
#if defined (CONFIG_SPX_FEATURE_MFP_2)
#include "peciifc.h"
#include "AddressDecodeLib.h"
#endif
#include "cpu.h"
#include "channel.h"
#include "libipmi_session.h"
#include "libipmi_StorDevice.h"
#include "IPMI_SEL.h"
#include "SEL_OEMRcdType.h"
#include "featuredef.h"
#if defined (CONFIG_SPX_FEATURE_MFP_3)
#include "AddressDecodeLib-egs.h"
#include "AddressDecodeInternal.h"
#include "libpeci4.h"
#include "peci-ioctl.h"
#include "peci.h"
#endif

/*** test purpose defs ***/
/* Define EVB_DEBUG in case local evaluation board is used instead of a real Wilsoncity host */
//#define	EVB_DEBUG		1

/*
 * If defined, a fake error can be injected by IPMItool
 * e.g. ipmitool -U admin -P admin123 -I lanplus -H 172.31.8.151 raw
 *      0x30 0x45 1 2 1 0 1 1 1  0 1 0 0  0 1 0 0 1 0 0 0 1
 */

/***********/
#define	MAX_NEWERR		256
#define SLEEP_THRESH	16
#define ELAPSE_LIMIT	3
#if !defined(DEBUG)
#define SNAPSHOT_SAVE_INTERVAL  (24*60*60)
#else
#define SNAPSHOT_SAVE_INTERVAL  200
#endif
#define DATA_PROC_DEFER_TIME	180
#define DATA_REC_SLEEP			5
#define DATA_MEMORY_FAULT_TRANSFER_SLEEP  5
#define DATA_MEMORY_FAULT_COLLECT_SLEEP   5
#define CPU_DETECT_INTERVAL     300

#define PIPE_READ_TIMEOUT		10
#define PIPE_WRITE_TIMEOUT		10

#define REDIS_SOCK		"/run/redis/redis.sock"
#define REDIS_LENGTH 100
#define MEM_ENTRY_LEN	32

/* Enumerate the column 128 times: 0 to (1024-8) */

#define ADDR_TRANS_MASK_4K       ((unsigned long long) (~0x0FFF))
#define ADDR_TRANS_COL_ADD_MIN   ((unsigned long long) (0x0000))
#define ADDR_TRANS_COL_ADD_MAX   ((unsigned long long) (0x03FF))

#define wakePECIAfterHostReset	1
#define WaitSecondsPeriodBetweenErrorPooling	1

#if 0
static INT32	gSigUSR1=0;
static struct timeval tReportStart, tReportEnd;
#endif

static struct timeval tlastErr, tCur, tlastSave;
pthread_mutex_t	mfpDataMutex = PTHREAD_MUTEX_INITIALIZER;
static INT32	newErrNum = 0;
static INT32	newValErrNum = 0;
static INT32	newShadowErrNum = 0;
struct mfp_error	newErr[MAX_NEWERR];
mfpval_error	newValErr[MAX_NEWERR];

static int fdFaultFifo = 0;

typedef void (*pPDKFunc) (void );
void *dl_pdkhandle = NULL;
static int pdkSCIFuncsInited = 0;
static int pdkSCIInited = 0;
static pPDKFunc pdkSCIInitFunc = NULL;
static pPDKFunc pdkSCITrigFunc = NULL;

static size_t	dimmCount = 0;
static INT32	memEntryCount = 0;		//SLOT Count
static struct mfp_dimm_entry dimmArray[MAX_DIMM_COUNT];

static struct mfp_dimm_entry dimmArrayVal[MAX_DIMM_COUNT];

struct mfp_evaluate_result *results = NULL;
static UINT16	dimmID[MAX_DIMM_COUNT];
static char		memEntry[MAX_DIMM_COUNT][MEM_ENTRY_LEN] =  {{0}};
static FILE 	*fp = NULL;
static FILE		*fpSave	= NULL;
static FILE		*fpStat = NULL;
static char env_systems_name[REDIS_LENGTH] = {0};
static INT32	redfishReportInit = 0;

static INT8U	nrCPU = 0;
static CpuTypes type[MAX_AMOUNT_OF_CPUS];
static INT8U	bus[MAX_AMOUNT_OF_CPUS];

#ifdef CONFIG_SPX_FEATURE_MFP_3
static int eccMode = ECC_MODE_UNKNOWN;
static int DDR5ColWidth = 0;
#endif

unsigned long long *rowOffLinedPagesSysAddr = NULL;
unsigned long long *cellOffLinedPagesSysAddr = NULL;
FILE *pRowFaultRec = NULL;
FILE *pCellFaultRec = NULL;

#if defined (TRACK_DETECTED_CORR_ERROR) && defined (MRT_DEBUG_TIME_STAMP)
struct timeval mrt_t0, mrt_t1;
#endif

int genMFPReport(UINT16 *pDimmID, struct mfp_evaluate_result *pResult, UINT32 count)
{
	size_t i;
	ami_mfp_evaluate_result mfpResult;
	FILE *fpRep;

	/* No need to call fclose() in case of failure */
	if ( count == 0 || pResult == NULL) {
		TWARN("no dimm is found or pResult is NULL\n");
		return 0;
	}

	fpRep = fopen(MFP_REPORT,"wb");
	if(fpRep == NULL) {
		TCRIT("Unable to open %s file\n", MFP_REPORT);
		return -1;
	}
	
	for (i=0; i<count; i++)  {
		mfpResult.dimmID = pDimmID[i];
		memcpy(&mfpResult.result, &pResult[i], sizeof(struct mfp_evaluate_result));
		if ( 1 != fwrite(&mfpResult, sizeof(ami_mfp_evaluate_result), 1, fpRep)) {
			TCRIT("writing MFP Report error\n");
			fclose(fpRep);
			return -1;
		}
	}
	fclose(fpRep);

	return 0;
}

int genMFPRedfishReport(UINT16 *pDimmID, struct mfp_evaluate_result *pResult, UINT32 count)
{
	UINT32 i=0;
	UINT32 j=0;
	redisContext *c = NULL;
	redisReply *reply = NULL;
    
	c = redisConnectUnix(REDIS_SOCK);
	if (c->err)
	{
		redisFree(c);
		return -1;
	} 
    
	for (i=0, j=0; i<(UINT32)memEntryCount; i++) {
		if ( i==pDimmID[j] && j<count) {
			if (redfishReportInit == 0 ) {
				TDBG("Set attributes for  %s:Memory:%s:MemoryMetrics\n",  env_systems_name, memEntry[i]);
				reply = redisCommand(c,"SET Redfish:Systems:%s:Memory:%s:MemoryMetrics:Id %s",env_systems_name, memEntry[i], memEntry[i]);
				if (reply == NULL ) {
					TCRIT("redis set id fails\n");
				}
				reply = redisCommand(c,"SET Redfish:Systems:%s:Memory:%s:MemoryMetrics:Name %s_Metric",env_systems_name, memEntry[i], memEntry[i]);
				if (reply == NULL ) {
					TCRIT("redis set name fails\n");
				}
			}
		    TDBG("result[%d] score = %d \n", j, pResult[j].score);
			reply = redisCommand(c,"SET Redfish:Systems:%s:Memory:%s:MemoryMetrics:dimm_score %d",env_systems_name, memEntry[i], pResult[j].score);
		    if (reply == NULL ) {
		    	TCRIT("redis set score fails\n");
		    }
		    j++;
		}
		else {
			if (redfishReportInit == 0 ) {
				TDBG("Del attributes for  %s:Memory:%s:MemoryMetrics\n",  env_systems_name, memEntry[i]);
				reply = redisCommand(c,"GET Redfish:Systems:%s:Memory:%s:MemoryMetrics:Id", env_systems_name, memEntry[i]);
				if (reply != NULL) {
					redisCommand(c,"DEL Redfish:Systems:%s:Memory:%s:MemoryMetrics:Id", env_systems_name, memEntry[i]);
				}
				reply = redisCommand(c,"GET Redfish:Systems:%s:Memory:%s:MemoryMetrics:Name",env_systems_name, memEntry[i]);
				if (reply != NULL ) {
					redisCommand(c,"DEL Redfish:Systems:%s:Memory:%s:MemoryMetrics:Name", env_systems_name, memEntry[i]);
				}
				reply = redisCommand(c,"GET Redfish:Systems:%s:Memory:%s:MemoryMetrics:dimm_score",env_systems_name, memEntry[i]);
				if (reply != NULL ) {
					redisCommand(c,"DEL Redfish:Systems:%s:Memory:%s:MemoryMetrics:dimm_score", env_systems_name, memEntry[i]);
				}
			}
		}
	}

	redfishReportInit = 1;
	freeReplyObject(reply);
	redisFree(c);
	return 0;
}

#if 0
int printMFPReport()
{
	FILE *fpRep;
	ami_mfp_evaluate_result mfpResult;
	fpRep = fopen(MFP_REPORT,"rb");
	if(fpRep == NULL) {
		TCRIT("Unable to open %s file\n", MFP_REPORT);
		return -1;
	}
	printf("\n%s\t%s\t%s\t%s\t%s:\t%s\n", "DIMM_ID", "SOCKET", "IMC", "CHANNEL", "SLOT", "SCORE");
	while (fread(&mfpResult, sizeof(ami_mfp_evaluate_result), 1, fpRep) == 1) {
		 printf("%u\t%d\t%d\t%d\t%d:\t%d\n", mfpResult.dimmID, mfpResult.result.loc.socket, mfpResult.result.loc.imc, 
				 mfpResult.result.loc.channel, mfpResult.result.loc.dimm, mfpResult.result.score);
	}
	fclose(fpRep);

	return 0;
}
#endif 

void mfp_signal_handler(int signum)
{
	UN_USED(signum);
	FILE *f = NULL;
	
	if ( access( MFP_VAL_KEY, F_OK ) != 0 ) {
		f = fopen(MFP_SNAPSHOT,"wb");	
		if(f != NULL) {
			mfp_save(f);
			fclose(f);
		}		
	}
	mfp_fin();

    ProcMonitorDeRegister("/usr/local/bin/mfp");
    unlink("/var/run/mfp.pid" );
    exit(0);
}

#if 0
void mfp_sigusr1_handler(int signum)
{
	UN_USED(signum);
	time_t deltaTime;

	if (gSigUSR1 == 0) {
		gettimeofday(&tReportStart, NULL);
		gSigUSR1++;
		return;
	}
	else if (gSigUSR1 == 1) {
		gettimeofday(&tReportEnd, NULL); 
		gSigUSR1++;
	}
	else {
		TCRIT("gSigUSR1 should not > 1 \n");
		gSigUSR1 = 0;
		return;
	}
	
	if ( tReportEnd.tv_sec >= tReportStart.tv_sec) {
		deltaTime = tReportEnd.tv_sec - tReportStart.tv_sec;
		if (deltaTime < ELAPSE_LIMIT ) {
			printMFPReport();
		}
		else {
			TDBG("Ignore the SIGUSR1 because the time gap is out of defined range\n");
		}
	}
	else {
		TINFO("tStart should not be greater than tEnd, clear state\n");
	}
	gSigUSR1 = 0;
	return;
}
#endif

int AddMFPSELEntries(struct mfp_error *err)
{
	IPMI20_SESSION_T pSession;
	uint8_t byPrivLevel = PRIV_LEVEL_ADMIN;
	int wRet;
	SELOEM1Record_T  OEMSELRec ;
	AddSELRes_T AddSELRes;

	wRet = LIBIPMI_Create_IPMI_Local_Session(&pSession,"","",&byPrivLevel,NULL,AUTH_BYPASS_FLAG,3);
	if(wRet != LIBIPMI_E_SUCCESS) {
		TCRIT("Cannot Establish IPMI Local Session\n");
		return -1;
	}

    /*
     * 	OEM Rec Type 0xC4
     *	OEMData[0] : SOCKET, Bit7-5; IMC, Bit4-3; CHANNEL, Bit2-1 
     *	OEMData[1] : SLOT, Bit7-6; RANK, Bit5-2;								 
     *	OEMData[2] : BANK GROUP, Bit7-5; BANK, Bit4-3; ERROR TYPE, Bit2
     *	OEMData[3] : most significant 8 bits of Column
     *	OEMData[4] : least significant 2 bits of Column for ddr4 on MRT2
     *	OEMData[4] : least significant 4 bits of Column for ddr5 for MRT3
     */
    
	OEMSELRec.ID = 0x00;
	OEMSELRec.Type = MEMORYFAILURE_OEMRECTYPE;
	OEMSELRec.TimeStamp = time(NULL);
	OEMSELRec.OEMData[0] = (err->socket<<SEL_SOCKET_SHFT) | (err->imc<<SEL_IMC_SHFT) | (err->channel<<SEL_CHAN_SHFT);
	OEMSELRec.OEMData[1] = (err->dimm<<SEL_DIMM_SHFT) | (err->rank<<SEL_RANK_SHFT);
	OEMSELRec.OEMData[2] = (err->bank_group<<SEL_BG_SHFT) | (err->bank<<SEL_BANK_SHFT)|(err->error_type<<SEL_ERRT_SHFT);
	OEMSELRec.OEMData[3] = (err->col >>SEL_COL_RSHFT ) & 0xFF;
	OEMSELRec.OEMData[4] = (err->col & ( ~(0xFF<<SEL_COL_RSHFT))) << (8-SEL_COL_RSHFT);
	//add the SEL entry
	wRet = IPMICMD_AddSELEntry (&pSession, (SELEventRecord_T *) &OEMSELRec, &AddSELRes, 3 );
    if (( wRet == LIBIPMI_E_SUCCESS ) && ( AddSELRes.CompletionCode == CC_SUCCESS ))  {
    	TDBG ("MFP EVENT Logged successfully\n");
	}
    LIBIPMI_CloseSession(&pSession);
    return 0;
}

void *computeMFPThread(void *pArg) 
{ 
	int retVal = 0;
	sigset_t   mask;
	int inited = 0;
	int valInited = 0;
	uint32_t timeTmp;
	int32 errNumTmp = 0;
	int32 errTotal = 0;
	int i = 0;
	
	time_t evalSec = 0;
	suseconds_t evaluSec=0;
	struct timeval	tEval;

	FILE *fKey;
	int c;
	int pnLen = 0;
	struct mfp_part_number pnVal;
	struct mfp_stat_result mfpStatResult;

	UN_USED(pArg);
	sigfillset(&mask);
	sigprocmask(SIG_SETMASK, &mask, NULL);
	prctl(PR_SET_NAME,__FUNCTION__,0,0,0);

DATA_PROC:
	while (1) {
		if ( access( MFP_VAL_KEY, F_OK ) == 0 ) {
			TINFO("Found %s, Exit MFP Data Process Loop\n", MFP_VAL_KEY);
			errno = 0;
			if (inited != 0) {
				fpSave = fopen(MFP_SNAPSHOT,"wb");

				if(fpSave != NULL) {
					retVal = mfp_save(fpSave);
					if(retVal != MFP_OK) {
						TCRIT("mfp_save error, retVal=%d\n", retVal);
						perror("mfp_save system error number : ");	
					}
					else {
						TINFO("%s is saved \n", MFP_SNAPSHOT);
					}
					fclose(fpSave);
				}
				
				retVal = mfp_fin();
				if(retVal != MFP_OK) {
					TCRIT("mfp_fin meet error, retVal=%d\n", retVal);
					if ( retVal == MFP_SYS_ERR) {
						perror("mfp_fin system error number : ");
					}
				}
				inited = 0; 
			}
			TINFO("Go to MFP Validation Data Process loop\n");
			break;
		}
		
		errno = 0;

		if (inited == 0 ) {
			fp = fopen(MFP_SNAPSHOT,"rb");
			if(fp == NULL) {
				TCRIT("Unable to open %s file, not necessarily error because snapshot may not exist in the beginning\n", MFP_SNAPSHOT);
			}
			//file size is 0, close it because mfp_init doesn't take 0 byte file
			if (fp != NULL ) {
				fseek (fp, 0, SEEK_END);
				if ( 0 == ftell (fp) ) {
					TINFO("%s size 0 byte\n", MFP_SNAPSHOT);
					fclose(fp);
					fp = NULL;
				}
			}
#if defined (CONFIG_SPX_FEATURE_MFP_2)
			retVal = mfp_init(time(0), fp, dimmCount, dimmArray);
#elif defined (CONFIG_SPX_FEATURE_MFP_3)
			retVal = mfp_init(time(0), fp, dimmCount, dimmArray, eccMode);
#endif
		    if(retVal != MFP_OK) {
		        TCRIT("\tStep 1: mfp_init meet error, ret=%d\n", retVal);
		        free(results);
		        return NULL;
		    }
		    
            retVal = mfp_evaluate_dimm(time(0), 0, newErr, dimmCount, results);
            if(retVal != MFP_OK) {
                TCRIT("mfp_evaluate_dimm meet error, retVal=%d\n", retVal);
            }
			genMFPReport(dimmID, results, dimmCount);
			genMFPRedfishReport(dimmID, results, dimmCount);
		    inited = 1;
		    TINFO("MFP Engine Initialized, MFP report generated for %u DIMMs\n", dimmCount);
		}
		
		if (newErrNum > 0 ) {
			TDBG("newErrNum = %d \n", newErrNum);
			gettimeofday(&tCur, NULL);

			if ( ((tCur.tv_sec-tlastErr.tv_sec) > DATA_PROC_DEFER_TIME)  || (newErrNum >= MAX_NEWERR)) {
				TDBG(" tCur.tv_sec is %u tlastErr.tv_sec %u \n", (unsigned int)tCur.tv_sec, (unsigned int)tlastErr.tv_sec);

				TINFO("process %d mfp data in single evaluation\n", newErrNum);
				pthread_mutex_lock(&mfpDataMutex);
	            retVal = mfp_evaluate_dimm(time(0), newErrNum, newErr, dimmCount, results);
	            if(retVal != MFP_OK) {
	                TCRIT("mfp_evaluate_dimm meet error, retVal=%d\n", retVal);
	            }
	            
	            newShadowErrNum = newErrNum;
				newErrNum = 0;
				pthread_mutex_unlock(&mfpDataMutex);
				TDBG("get newShadowErrNum %d\n", newShadowErrNum);
				
#if defined(DEBUG)
				TDBG("after results0 =%u result1=%u\n", results[0].score, results[1].score);
				gettimeofday(&tEval, NULL);
				if ( tEval.tv_usec > tCur.tv_usec) {
					evaluSec = tEval.tv_usec - tCur.tv_usec;
					evalSec = tEval.tv_sec - tCur.tv_sec;
				}
				else {
					evaluSec = tEval.tv_usec + (1000000- tCur.tv_usec);
					evalSec = tEval.tv_sec - tCur.tv_sec -1;
				}
				TDBG(" MFP evaluation time is %ld.%ld seconds \n", evalSec, evaluSec);
#endif
				genMFPReport(dimmID, results, dimmCount);
				genMFPRedfishReport(dimmID, results, dimmCount);
			}			
		}

		gettimeofday(&tCur, NULL);

		if ((tCur.tv_sec-tlastSave.tv_sec) > SNAPSHOT_SAVE_INTERVAL) {
			tlastSave.tv_sec = tCur.tv_sec;
	        fpSave = fopen(MFP_SNAPSHOT,"wb");

	        if(fpSave == NULL) {
        		TCRIT("Unable to open %s file\n", MFP_SNAPSHOT);
        	}
	        else {
				retVal = mfp_save(fpSave);
				if(retVal != MFP_OK) {
					TCRIT("mfp_save error, retVal=%d\n", retVal);
				}
				else {
					TDBG("%s is saved \n", MFP_SNAPSHOT);
				}
				fclose(fpSave);
	        }
	        for ( i=0; i< (int)dimmCount; i++ ) {
	        	mfp_stat(dimmArray[i].loc, &mfpStatResult);
	        	updateStatResult(dimmArray[i].loc, &mfpStatResult);
	        }
		}

		if ( newErrNum < SLEEP_THRESH ) {
			sleep(DATA_REC_SLEEP);
		}
	}
	
	while (1)
	{
		if ( access( MFP_VAL_KEY, F_OK ) != 0 ) {
			TINFO("Not Found %s, Exit MFP Validation Data Process Loop\n", MFP_VAL_KEY);
			errno = 0;
			if (valInited != 0 ) {			
				retVal = mfp_fin();
				if(retVal != MFP_OK) {
					TCRIT("mfp_fin meet error, retVal=%d\n", retVal);
					if ( retVal == MFP_SYS_ERR) {
						perror("mfp_fin system error number : ");
					}
				}
				valInited = 0;
			}
			TINFO("Go to MFP Data Process loop\n");
			goto DATA_PROC;
		}
		
		if (valInited == 0 ) {
			memset(pnVal.s, 0, sizeof(pnVal.s));
			fKey = fopen(MFP_VAL_KEY, "r");
			if (fKey == NULL) {
				TCRIT("Unable to open %s for read \n", MFP_VAL_KEY);
		        if(results != NULL) {
		        	free(results);
		        }
				return NULL;
			}
			i = 0;
			do {
				c = fgetc(fKey);
				if (feof(fKey)) {
					break;
				}
				else {
					pnVal.s[i++] = c;
				}
			} while (1);
			pnLen = i;
			TINFO("pnLen = %d, pn: %s\n", pnLen, pnVal.s);
			fclose(fKey);
			memcpy((void *)dimmArrayVal, (void *)dimmArray, sizeof(dimmArray));
			for (i=0; i<(int)dimmCount; i++) {
				memcpy(dimmArrayVal[i].pn.s, pnVal.s, pnLen);
			}
#ifdef CONFIG_SPX_FEATURE_MFP_2
			retVal = mfp_init(0, NULL, dimmCount, dimmArrayVal);
#elif defined (CONFIG_SPX_FEATURE_MFP_3)
			retVal = mfp_init(0, NULL, dimmCount, dimmArrayVal, eccMode);
#endif
		    if(retVal != MFP_OK) {
		        TCRIT("\tStep 1: mfp_init meet error, ret=%d\n", retVal);
		        if(results != NULL) {
		        	free(results);
		        }
		        return NULL;
		    }
		    errTotal = 0;
		    valInited = 1;
		}
		
		TDBG("newValErrNum = %d \n", newValErrNum);
		if (newValErrNum > 0 ) {
			gettimeofday(&tCur, NULL);

			if ( ((tCur.tv_sec-tlastErr.tv_sec) > DATA_PROC_DEFER_TIME)  || (newValErrNum >= MAX_NEWERR)) {

				TDBG(" tCur.tv_sec is %u tlastErr.tv_sec %u \n", (unsigned int)tCur.tv_sec, (unsigned int)tlastErr.tv_sec);
				TDBG("process %d mfp validation error data one by one\n", newValErrNum);
				pthread_mutex_lock(&mfpDataMutex);
				errNumTmp = newValErrNum;
				errTotal += errNumTmp;
				for ( i=0; i< newValErrNum; i++ ) {
					timeTmp = newValErr[i].timestamp;
					retVal = mfp_evaluate_dimm(timeTmp, 1, &newValErr[i].valerr, dimmCount, results);
					if(retVal != MFP_OK) {
						TCRIT("mfp_evaluate_dimm meet error, retVal=%d\n", retVal);
					}
				}
				newValErrNum = 0;
				pthread_mutex_unlock(&mfpDataMutex);
				
				printf("After %d errors:\n", errTotal);
				for ( i = 0; i< (int)dimmCount; i++) {
					if(results[i].score != 100){
						printf("DIMM Health Score:: %u-%u-%u-%u:\t%u\n",results[i].loc.socket, results[i].loc.imc, results[i].loc.channel, results[i].loc.dimm, results[i].score);
					}
				}

				gettimeofday(&tEval, NULL);
				if ( tEval.tv_usec > tCur.tv_usec) {
					evaluSec = tEval.tv_usec - tCur.tv_usec;
					evalSec = tEval.tv_sec - tCur.tv_sec;
				}
				else {
					evaluSec = tEval.tv_usec + (1000000- tCur.tv_usec);
					evalSec = tEval.tv_sec - tCur.tv_sec -1;
				}
				TINFO(" MFP Validation evaluation time of %d entries is %d.%d seconds \n", errNumTmp, evalSec, evaluSec);
			}
		}

		if ( newValErrNum < SLEEP_THRESH ) {
			sleep(DATA_REC_SLEEP);
		}
	} /* while(1) */

    return NULL;
}

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
    	return -1;
    }
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

/* ***************************************************************
 * Only run mfp_stat on those dimms on which new errors occur
 * besides 1st time mfp_stat on all dimms after boot
 * remove the duplicate dimms for mfp_stat
 * return : nubmer of dimms that should go through mfp_stat
 *****************************************************************/
int getDimmsForStat(struct mfp_dimm *pDimm, int lastDimmForStatNum, struct mfp_error *pError, int errorNumber)
{
	int i=0;
	int j=lastDimmForStatNum;
	int k=0;
	if (errorNumber <= 0) {
		TCRIT("error nubmer is %d <=0, just ignore\n", errorNumber);
		return 0;
	}
	TDBG("lastDimmForStatNum =%d\n", lastDimmForStatNum);
	for (i=0; i<errorNumber; i++) {

		k=0;
		while (k<j) {
			if ( ((pError+i)->socket == (pDimm+k)->socket) && ((pError+i)->imc == (pDimm+k)->imc) && ((pError+i)->channel == (pDimm+k)->channel) 
					&& ((pError+i)->dimm == (pDimm+k)->dimm) ) {
				TDBG("dimm associated with error[%d] already exists", i);
				TDBG("i=%d, k=%d, socket %u, imc %u, channel %u, dimm %u", i, k, (pError+i)->socket, (pError+i)->imc, (pError+i)->channel, (pError+i)->dimm );
				break;
			}
			k++;
		}
		if (k==j) {
			TINFO("dimm associated with error[%d] does not exist,add a new dimm for mfp_stat", i);
			TDBG("__i=%d, k=%d, socket %u, imc %u, channel %u, dimm %u", i, k, (pError+i)->socket, (pError+i)->imc, (pError+i)->channel, (pError+i)->dimm );
			(pDimm+j)->socket = (pError+i)->socket;
			(pDimm+j)->imc = (pError+i)->imc;
			(pDimm+j)->channel = (pError+i)->channel;
			(pDimm+j)->dimm = (pError+i)->dimm;
			j++;
		}

	}
	if ( j > MAX_DIMM_COUNT ) {
		TCRIT("total dimms for stat can not exceed available dimms, something is wrong, ignore this time\n");
		return 0;
	}
	return j;
}


int filterNewFaultByMemErr(struct mfp_error *pShError, int errorShNum, struct mfp_faults *recentFaults, struct mfp_component *newFault, int *newFaultNum, faultType fType)
{
	int i=0, j=0;
	*newFaultNum = 0;
	printf("recent rows size is %u, row[0] size %u\n", sizeof(recentFaults->rows), sizeof(recentFaults->rows[0]) );

	for ( j=0; j<(int)(sizeof(recentFaults->rows)/sizeof(recentFaults->rows[0])); j++) {
		for ( i=0; i<errorShNum;i++ ) {
			printf("%d: error socket %u, imc %u, channel %u, slot %u\n", i, pShError[i].socket,pShError[i].imc,pShError[i].channel,pShError[i].dimm);
			printf("%d: error rank %u, device %u, bank_group %u, bank %u\n", i, pShError[i].rank,pShError[i].device,pShError[i].bank_group,pShError[i].bank);
			printf("%d: error row 0x%5x, col 0x%3x\n", i, pShError[i].row,pShError[i].col);
			switch (fType) {
			case ROWFAULT:
				if (pShError[i].socket==recentFaults->rows[j].socket
						&& pShError[i].imc==recentFaults->rows[j].imc
						&& pShError[i].channel==recentFaults->rows[j].channel
						&& pShError[i].dimm==recentFaults->rows[j].dimm
						&& pShError[i].rank==recentFaults->rows[j].rank
						&& pShError[i].device==recentFaults->rows[j].device
						&& pShError[i].bank_group==recentFaults->rows[j].bank_group
						&& pShError[i].bank==recentFaults->rows[j].bank
						&& pShError[i].row==recentFaults->rows[j].row
						&& recentFaults->rows[j].valid) {
					memcpy((void *)(&newFault[*newFaultNum]), (void *)(&recentFaults->rows[j]), sizeof(struct mfp_component));
					*newFaultNum +=1;
				}
				break;
			
			case CELLFAULT:
				printf("%d: error socket %u, imc %u, channel %u, slot %u, valid=%u\n", j, recentFaults->cells[j].socket,
						recentFaults->cells[j].imc,recentFaults->cells[j].channel,recentFaults->cells[j].dimm, recentFaults->cells[j].valid);
				
				printf("%d: error rank %u, device %u, bank_group %u, bank %u\n", i, recentFaults->cells[j].rank,recentFaults->cells[j].device,recentFaults->cells[j].bank_group,recentFaults->cells[j].bank);
				printf("%d: error row 0x%5x, col 0x%3x\n", i, recentFaults->cells[j].row,recentFaults->cells[j].col);
				if (pShError[i].socket==recentFaults->cells[j].socket
						&& pShError[i].imc==recentFaults->cells[j].imc
						&& pShError[i].channel==recentFaults->cells[j].channel
						&& pShError[i].dimm==recentFaults->cells[j].dimm
						&& pShError[i].rank==recentFaults->cells[j].rank
						&& pShError[i].device==recentFaults->cells[j].device
						&& pShError[i].bank_group==recentFaults->cells[j].bank_group
						&& pShError[i].bank==recentFaults->cells[j].bank
						&& pShError[i].row==recentFaults->cells[j].row
						&& pShError[i].col==recentFaults->cells[j].col
						&& recentFaults->cells[j].valid) {
					memcpy((void *)(&newFault[*newFaultNum]), (void *)(&recentFaults->cells[j]), sizeof(struct mfp_component));
					*newFaultNum +=1;
				}
				break;
			default:
				TCRIT("unrecognizable fault type\n");
		
			}
		}
	}
	printf("new fault number is %d\n", *newFaultNum);
	return 0;
}

/*****************************************************************
 * API mfp_recent_faults() output faults in reverse chronological order
 * fault[0] in last recent faults is pushed down in new recent faults 
 * if there is new faults.
 * fault[0] is used to determine how many new faults
 *****************************************************************/
int getNewFaultsFromRecent(struct mfp_component *anchorFault, struct mfp_component *newFault, int *newFaultNum, struct mfp_component *recentFaultsResult)
{

	int i = 0;
	*newFaultNum = 0;

	while (memcmp(anchorFault, recentFaultsResult + *newFaultNum, sizeof(struct mfp_component))) {		
		memcpy((newFault + *newFaultNum), (recentFaultsResult + *newFaultNum), sizeof(struct mfp_component));
		*newFaultNum += 1;
		if (*newFaultNum>=FAULTN) {
			break;
		}
	}
	
	for ( i=0; i<*newFaultNum; i++) {
		if (!recentFaultsResult[i].valid) {
			TWARN("%dth New fault is invalid, should not happen\n", i);
			*newFaultNum = i;
			break;
		}
	}
	TINFO("%d new faults in recent mfp faults\n", *newFaultNum);
	return 0;
}

/*********************************************************************************
 * This API may be redundant because if API mfp_recent_faults() works as expected, 
 * the fault record should not have a same copy as new fault 
 * This API performs an extra check
 *********************************************************************************/
int filterNewFaultByExistFault(struct mfp_component *faultToDate, int FaultNumToDate, 
		struct mfp_component *outFault, int *outFaultNum,
		struct mfp_component *newFault, int newFaultNum, faultType fType)
{
	int i=0,j=0;
	*outFaultNum = 0;
	int found = 0;
	
	TDBG("newfault num %d, To Date fault number %d\n ", newFaultNum, FaultNumToDate);
	for ( i=0; i<newFaultNum; i++) {
		found = 0;
		for ( j=0; j<FaultNumToDate; j++) {
			if (newFault[i].socket==faultToDate[j].socket
					&& newFault[i].imc==faultToDate[j].imc
					&& newFault[i].channel==faultToDate[j].channel
					&& newFault[i].dimm==faultToDate[j].dimm
					&& newFault[i].rank==faultToDate[j].rank
					&& newFault[i].device==faultToDate[j].device
					&& newFault[i].bank_group==faultToDate[j].bank_group
					&& newFault[i].bank==faultToDate[j].bank
					&& newFault[i].row==faultToDate[j].row) {
				switch (fType) {
				case ROWFAULT:
					found = 1;
					break;
				case CELLFAULT:
					if (newFault[i].col==faultToDate[j].col) {
						found =1;
					}
					break;
				default:
					TCRIT("unrecognizable fault type\n");
				}
			}
			
			if (found) {
				TWARN("Technically we should not find the same saved fault as the new fault\n");
				break;
			}
		}
		if (!found && newFault[i].valid) {
			memcpy((void *)(&outFault[*outFaultNum]), (void *)(&newFault[i]), sizeof(struct mfp_component));
			*outFaultNum +=1;
		}
	}
	TDBG("out fault number is %d\n", *outFaultNum);
	return 0;
}

/* Legacy API from MRT 2.1, not used in MRT 3 */
/* if the row fault is new one, return 1; otherwise return 0 */
int isNewFaultRow(struct mfp_mem_fault_t *newfault, struct mfp_mem_fault_t *faultRec, int recNum)
{
	int retVal = 1;
	int i;
	for (i=0; i< recNum; i++) {
		if ( (newfault->socket==faultRec[i].socket) && (newfault->imc==faultRec[i].imc) && (newfault->channel==faultRec[i].channel)
				&& (newfault->slot==faultRec[i].slot) && (newfault->rank==faultRec[i].rank) && (newfault->device==faultRec[i].device)
				&& (newfault->bankgroup==faultRec[i].bankgroup) && (newfault->bank==faultRec[i].bank) 
				&& (newfault->min_row==faultRec[i].min_row) && (newfault->max_row==faultRec[i].max_row) )  {
			return 0;
		}
	}
	return retVal;
}

/* ******************************************************************************
 * check if the newly translated system address is already in offline page record
 * Use 4KB aligned system address for comparison
 * if the system address is new, return 1. otherwise, return 0
 * ******************************************************************************/
int isNewPageAddress(unsigned long long *offlinePageRecord, int size, unsigned long long sysAddr)
{
	int i;
	int retval = 1;

	for (i=0; i<size; i++)  {
		if ((*(offlinePageRecord+i) & ADDR_TRANS_MASK_4K) == (sysAddr & ADDR_TRANS_MASK_4K)) {
			retval=0;
			break;
		}
	}
	return retval;
}

/* Legacy API from MRT 2.1, not used in MRT 3 */
/* return 1 if the row fault cap on a dimm is reached */
int isRowFaultCapReached(dimmFaultCount *rec, int recNum, struct mfp_dimm *pDimm)
{
	int k;
	for (k=0; k<(int)recNum; k++) {
		if ( (pDimm->socket==rec[k].loc.socket) &&  (pDimm->imc==rec[k].loc.imc) 
				&&  (pDimm->channel==rec[k].loc.channel) &&  (pDimm->dimm==rec[k].loc.dimm)
				&& (rec[k].faultCount>=ROW_FAULT_CAP_PER_DIMM) ) {
			TWARN("socket=%u, imc=%u, channel=%u, dimm=%u reached row fault cap %u\n", pDimm->socket, pDimm->imc,
					pDimm->channel,pDimm->dimm, ROW_FAULT_CAP_PER_DIMM);
			return 1;
		}			
	}
	return 0;
}

/* Legacy API from MRT 2.1, not used in MRT 3 */
int updateRowFaultOnDimm(dimmFaultCount *rec, int recNum, struct mfp_mem_fault_t *newfault)
{
	int i;
	for (i=0; i< recNum; i++) {
		if ( (newfault->socket==rec[i].loc.socket) && (newfault->imc==rec[i].loc.imc) && (newfault->channel==rec[i].loc.channel)
				&& (newfault->slot==rec[i].loc.dimm) )  {
			rec[i].faultCount += 1;
		}
	}
	return 0;
}

int getLastComponentFaultRec(FILE *pFile, struct mfp_component *pCompFault, int *faultCnt, struct mfp_dimm_entry *dimms, int dimmCnt, int *countByDimm, faultType fType)
{
	int size = 0;
	int i=0, j=0, k=0;
	mrtFaultRec rec[MAX_TOTAL_FAULT_NUM] = {0};
	INT32U index = 0;
	size_t nmemb = 0;
	size_t readNum = 0;
	size_t maxInst = 0;
	
	switch (fType)
	{
		case ROWFAULT:
			maxInst = MAX_TOTAL_ROW_FAULT_NUM;
			TINFO("%s%d: record file %s\t", __FUNCTION__, __LINE__, MRT_ROW_FAULT_REC);
			break;
		case CELLFAULT:
			maxInst = MAX_TOTAL_CELL_FAULT_NUM;
			TINFO("%s%d: record file %s\t ", __FUNCTION__, __LINE__, MRT_CELL_FAULT_REC);
			break;
		default:
			TCRIT("unrecognizable fault type\n");
	}

	fseek (pFile, 0, SEEK_END);
	size = ftell(pFile);
	TINFO("size is %d\n", size);
	if ( size < 0 ) {
		TCRIT("get component fault record file size error\n");
		return -1;
	}

	rewind(pFile);
	nmemb = size/sizeof(mrtFaultRec) >= maxInst? maxInst : size/sizeof(mrtFaultRec);
	if (nmemb > 0) {
		readNum = fread(rec, sizeof(mrtFaultRec), nmemb, pFile);
		if (readNum != nmemb) {
			TCRIT("read record number %u != record number in file %u\n", readNum, nmemb);
		}
	}
	TINFO("Read  %u records\n", readNum);
	
	/*
	 * if a dimm is replaced or removed, the fault record of this dimm is also removed.
	 */
	for (i=0; i<(int)(readNum); i++) {
		for (j=0; j<dimmCnt; j++) {
			if (rec[i].dimmInfo.loc.socket == dimms[j].loc.socket &&  rec[i].dimmInfo.loc.imc == dimms[j].loc.imc 
					&& rec[i].dimmInfo.loc.channel == dimms[j].loc.channel && rec[i].dimmInfo.loc.dimm == dimms[j].loc.dimm
					&& rec[i].dimmInfo.sn == dimms[j].sn && !memcmp(rec[i].dimmInfo.pn.s, dimms[j].pn.s, sizeof(dimms[j].pn.s))) {
				memcpy(&pCompFault[k++], &rec[i].compFault,sizeof(rec[i].compFault));
				if ( 0 == getIndexOfDimm(rec[i].dimmInfo.loc.socket, rec[i].dimmInfo.loc.imc, rec[i].dimmInfo.loc.channel, 
						rec[i].dimmInfo.loc.dimm, &index) ) {
					countByDimm[index] += 1;
				}
				else {
					TCRIT("socket=%u, imc=%u, channel=%u, slot=%u, index out of range\n", rec[i].dimmInfo.loc.socket, rec[i].dimmInfo.loc.imc,
							rec[i].dimmInfo.loc.channel, rec[i].dimmInfo.loc.dimm);	
				}
			}				
		}
	}
	*faultCnt = k;
	TINFO("Get %d fault records\n", k);
	return 0;	
}

int writeComponentFaultRec(const char *filePath, struct mfp_component *compFault, int faultCnt, struct mfp_dimm_entry *dimms, int dimmCnt)
{
	mrtFaultRec rec[MAX_TOTAL_FAULT_NUM] = {0};
	int i = 0, j=0;
	FILE *pRec = NULL;
	
	pRec = fopen(filePath,"wb");
	if(pRec == NULL) {
		TCRIT("Unable to open %s\n", filePath);
		return -1;
	}
	
	for ( i=0; i<faultCnt; i++ ) {
		memcpy(&rec[i].compFault, &compFault[i], sizeof(rec[i].compFault));
		for ( j=0; i<dimmCnt; j++ ) {
			if ( compFault[i].socket==dimms[j].loc.socket && compFault[i].imc==dimms[j].loc.imc 
					&& compFault[i].channel==dimms[j].loc.channel && compFault[i].dimm==dimms[j].loc.dimm) {
				
				memcpy(&rec[i].dimmInfo, &dimms[j], sizeof(rec[i].dimmInfo));
				break;
			}
		}
		if ( j == dimmCnt) {
			TCRIT("Error: component fault sees no associated DIMM, however, keep going\n");
		}
	}
	
	if ( (size_t)faultCnt != fwrite(rec, sizeof(mrtFaultRec), (size_t)faultCnt, pRec) ) {
		TCRIT("Write %s error\n", filePath);
		fclose(pRec);
		return -1;
	}
	fclose(pRec);
	TINFO("write %d records to %s\n", faultCnt, filePath);
	return 0;	
}

int updateComponentFaultRec(FILE *pFile, struct mfp_component *compFault, struct mfp_dimm_entry *dimms, int dimmCnt, faultType fType)
{
	mrtFaultRec mrtFaultRecEntry = {0};
	int i = 0;
	int size = 0;	
	size_t maxInst = 0;
	
	for ( i=0; i<dimmCnt; i++) {
		if ( compFault->socket==dimms[i].loc.socket && compFault->imc==dimms[i].loc.imc 
				&& compFault->channel==dimms[i].loc.channel && compFault->dimm==dimms[i].loc.dimm) {
			memcpy(&mrtFaultRecEntry.compFault, compFault, sizeof(mrtFaultRecEntry.compFault));
			memcpy(&mrtFaultRecEntry.dimmInfo, &dimms[i], sizeof(mrtFaultRecEntry.dimmInfo));
			break;
		}
	}

	if (i==dimmCnt) {
		return -1;	//not found
	}

	switch (fType)
	{
		case ROWFAULT:
			maxInst = MAX_TOTAL_ROW_FAULT_NUM;
			TDBG("%s%d: record file %s\t", __FUNCTION__, __LINE__, MRT_ROW_FAULT_REC);
			break;
		case CELLFAULT:
			maxInst = MAX_TOTAL_CELL_FAULT_NUM;
			TDBG("%s%d: record file %s\t", __FUNCTION__, __LINE__, MRT_CELL_FAULT_REC);
			break;
		default:
			TCRIT("unrecognizable fault type\n");
	}
	
	fseek (pFile, 0, SEEK_END);
	size = ftell(pFile);
	if ( size < 0 ) {
		TCRIT("get component fault record file size error\n");
		return -1;
	}
	TDBG("size is %d\n", size);
	
	if ( size/sizeof(mrtFaultRecEntry) < maxInst ) {
		if ( 1 != fwrite(&mrtFaultRecEntry, sizeof(mrtFaultRecEntry), 1, pFile) ) {
			TCRIT("update Component Fault Record Error\n");
			return -1;
		}
	}
	else {
		TWARN("record file reach max items %u\n", maxInst);
	}
	fflush(pFile);
	return 0;
}


int pageOfflineFromFault(struct mfp_component compFault, faultType fType, unsigned long long *offLinePageAddr, int offLinePageStart, int *offLinePagesEnd)
{
	dimmBDFst dimmBdp;
	TRANSLATED_ADDRESS  TranslatedAddress = {0};
	EFI_STATUS          Status;
	int pageCnt = 0;
	int translationErr = 0;
	
	TranslatedAddress.SocketId           = compFault.socket & SOCKET_MASK;
	TranslatedAddress.MemoryControllerId = compFault.imc & IMC_MASK;
	TranslatedAddress.ChannelId          = compFault.channel & IMC_BASE_CHANNEL_MASK;
	TranslatedAddress.DimmSlot           = compFault.dimm & DIMM_MASK;
//	TranslatedAddress.PhysicalRankId     = compFault.rank & RANK_MASK;
	TranslatedAddress.BankGroup          = compFault.bank_group & BG_MASK;
	TranslatedAddress.Bank               = compFault.bank & BANK_MASK;
	TranslatedAddress.Row                = compFault.row & ROW_MASK;
	TranslatedAddress.Col                = compFault.col & COLUMN_MASK;

#if defined (CONFIG_SPX_FEATURE_MFP_2)
	TranslatedAddress.PhysicalRankId     = compFault.rank & 0x03;
#elif defined (CONFIG_SPX_FEATURE_MFP_3)
	TranslatedAddress.ChipSelect         = compFault.rank & RANK_MASK;
	TranslatedAddress.PhysicalRankId     = 0xFF;
#endif

	dimmBdp.cpuType  = (uint8_t) type[compFault.socket & SOCKET_MASK];
	dimmBdp.bus      = bus[compFault.socket & SOCKET_MASK];
	dimmBdp.socket   = compFault.socket & SOCKET_MASK;
	dimmBdp.imc      = compFault.imc & IMC_MASK;
	dimmBdp.channel  = compFault.channel & IMC_BASE_CHANNEL_MASK;

	switch (fType)
	{
		case ROWFAULT:
			
			for (TranslatedAddress.Col=0; TranslatedAddress.Col<(UINT32)(1<<DDR5ColWidth); TranslatedAddress.Col+=16) {
				Status = DimmAddressToSystemAddress(&dimmBdp, &TranslatedAddress);
				if (Status) {
					TCRIT("[col 0x%x]: DimmAddressToSystemAddress() Error: 0x%llx\n", TranslatedAddress.Col, Status);
					TCRIT("skt %d imc %d ch %d dimm %d rank %d, bg %d b %d r %d\n",
					TranslatedAddress.SocketId,
					TranslatedAddress.MemoryControllerId,
					TranslatedAddress.ChannelId,
					TranslatedAddress.DimmSlot,
					TranslatedAddress.PhysicalRankId,
					TranslatedAddress.BankGroup,
					TranslatedAddress.Bank,
					TranslatedAddress.Row);
					translationErr = 1;
					break ;
				} else {
					TDBG("[col 0x%x]: DimmAddressToSystemAddress(): address 0x%llx\n", TranslatedAddress.Col, TranslatedAddress.SystemAddress);
				}
				if (isNewPageAddress(offLinePageAddr, *offLinePagesEnd, TranslatedAddress.SystemAddress)) {
					if (*offLinePagesEnd < MAX_TOTAL_ROW_FAULT_PAGE_NUM) {
						offLinePageAddr[offLinePageStart+pageCnt] = TranslatedAddress.SystemAddress & ADDR_TRANS_MASK_4K;
						TINFO("[col 0x%x]: translated page address 0x%llx\n", TranslatedAddress.Col, offLinePageAddr[offLinePageStart+pageCnt]);					
						*offLinePagesEnd += 1;
						pageCnt++;
					}
					else {
						break;
					}
				}
			}
			if (translationErr) {
				*offLinePagesEnd -= pageCnt;	//rewind
			}
			break;
		case CELLFAULT:
			Status = DimmAddressToSystemAddress(&dimmBdp, &TranslatedAddress);
			if (Status) {
				TCRIT("[col 0x%x]: DimmAddressToSystemAddress() Error: 0x%llx\n", TranslatedAddress.Col, Status);
				TCRIT("skt %d imc %d ch %d dimm %d rank %d, bg %d b %d r %d\n",
				TranslatedAddress.SocketId,
				TranslatedAddress.MemoryControllerId,
				TranslatedAddress.ChannelId,
				TranslatedAddress.DimmSlot,
				TranslatedAddress.PhysicalRankId,
				TranslatedAddress.BankGroup,
				TranslatedAddress.Bank,
				TranslatedAddress.Row);
				translationErr = 1;
				break ;
			} else {
				if (isNewPageAddress(offLinePageAddr, *offLinePagesEnd, TranslatedAddress.SystemAddress)) {
					if (*offLinePagesEnd < MAX_TOTAL_CELL_FAULT_PAGE_NUM) {
						TINFO("[col 0x%x]: translated page address 0x%llx\n", TranslatedAddress.Col, TranslatedAddress.SystemAddress);
						offLinePageAddr[offLinePageStart] = TranslatedAddress.SystemAddress;
						*offLinePagesEnd +=1;
					}
				}
			}
			break;
		default:
			TCRIT("unsupported fault type\n");
	}
	
	return translationErr? -1 : 0;
}

int writeOffLinePages(unsigned long long *pSysAddr, int *start, int *end)
{
	int pageCnt = 0;
	int tx_count = 0;
	int writtenByte = 0, writeByte;
	
	
	if (*end > *start) {
		pageCnt = *end - *start;
	}
	else if ( *end < *start ) {
		TCRIT("end = %d < start %d, Should not happen, Error\n", *end, *start);
		return -1;
	}
	else {
		return 0;
	}
	
	while ( pageCnt > 0) {

   	    tx_count = pageCnt >= MAX_FAULT_ERR? MAX_FAULT_ERR : pageCnt;
		/*
		 * Transmit the page count followed by page addresses
		 */
   	    
   	    /* Tx 1: record size */
		writtenByte = sigwrap_write(fdFaultFifo, (void *)&tx_count, sizeof(tx_count));
		/* Tx 2: records */
		writeByte   = tx_count * sizeof(unsigned long long);
		writtenByte = sigwrap_write(fdFaultFifo, (void *)&pSysAddr[*start], writeByte);
		if (writeByte == writtenByte) {
			TINFO("mfp fault data was written: %d records\n", tx_count);
			/*
			 * Trigger SCI pin
			 */
			TDBG("Trigger the SCI pin to notify BIOS of memory fault\n");
			triggerSci();
			sleep(2);	//Give host and bios 2 second to handle SCI
		} else {
			if (writtenByte == 0) {
				TCRIT("no mfp fault data is written\n");
			}
			if(errno == EINTR || errno == EAGAIN) {
				TCRIT("writing mfp fault pipe gets error %d, writtenByte= %d", errno, writtenByte);
			}
			TCRIT("Not get right size of data\n");
		}
		pageCnt -= tx_count;
		*start  += tx_count;
	}

	return 0;
}

int isCapReached(struct mfp_component *compf, int *faultByDimm, faultType fType)
{
	INT32U index = 0;
	int retVal = 0;
	if ( 0 != getIndexOfDimm(compf->socket, compf->imc, compf->channel, compf->dimm, &index) ) {
		return 1;
	}
	
	switch (fType)
	{
		case ROWFAULT:
			retVal = faultByDimm[index]>=ROW_FAULT_CAP_PER_DIMM? 1 : 0;
			break;
		case CELLFAULT:
			retVal = faultByDimm[index]>=CELL_FAULT_CAP_PER_DIMM? 1 : 0;
			break;
		default:
			TCRIT("unrecognizable fault type\n");
	}
	return retVal;
}

void *colMemFaultThread(void *pArg)
{
	sigset_t   mask;
	int cpu_wait = 0;

	int i;
	int rowOffLinedPageStart = 0;
	int cellOffLinedPageStart = 0;
	int rowOffLinedPageEnd = 0;
	int cellOffLinedPageEnd = 0;
	int rowOffLinedPageCurStart = 0;
	int cellOffLinedPageCurStart = 0;
	int row_fault_count = 0;
	int cell_fault_count = 0;

	struct mfp_component	rowFault[MAX_TOTAL_ROW_FAULT_NUM]={0};
	struct mfp_component	cellFault[MAX_TOTAL_CELL_FAULT_NUM] = {0};
	struct mfp_faults	recentFaults;
	
	struct mfp_component	rowAnchor = {0};
	struct mfp_component	cellAnchor = {0};
	
	struct mfp_component	rowFaultFromRecent[FAULTN]={0};
	int rowFaultNumFromRecent = 0;
	struct mfp_component	cellFaultFromRecent[FAULTN] = {0};
	int cellFaultNumFromRecent = 0;
	
	struct mfp_component	rowFaultFilterByRec[FAULTN]={0};
	int rowFaultNumFilterByRec = 0;
	struct mfp_component	cellFaultFilterByRec[FAULTN] = {0};
	int cellFaultNumFilterByRec = 0;

	int rowFaultByDimm[MAX_DIMM_COUNT] = {0};
	int cellFaultByDimm[MAX_DIMM_COUNT] = {0};
	INT32U	indexByDimm = 0;
	struct mfp_stat_result statResult;
	
	UN_USED(pArg);
	sigfillset(&mask);
	sigprocmask(SIG_SETMASK, &mask, NULL);
	prctl(PR_SET_NAME,__FUNCTION__,0,0,0);

	/*
	 * It is possible that this thread starts running
	 * before DetectCpuType() is called
	 */
	sleep(180);        //Wait until host OS has started up, 3 minutes should be long enough
	printf("Thread %s starts\n", __FUNCTION__);
	while(nrCPU == 0) {
		cpu_wait += 2;
		sleep(2);
		if(cpu_wait > CPU_DETECT_INTERVAL)
			TCRIT("ERROR: The number of CPU = 0 after %ds.\n", cpu_wait);
		else
			TINFO("mfp: The number of CPU = 0. Wait for %ds.\n", cpu_wait);
	}
    TDBG("mfp: The number of CPU = %d\n", nrCPU);
    
	/* Initialization of ADDRESS_TRANSLATION */
	
#if defined (CONFIG_SPX_FEATURE_MFP_2)
	EFI_STATUS eresult = InitAddressDecodeLib(nrCPU);
#elif defined (CONFIG_SPX_FEATURE_MFP_3)
	EFI_STATUS eresult = InitAddressDecodeLib(bus, nrCPU);
#endif
	if( EFI_SUCCESS !=  eresult) {
		TCRIT("InitAddressDecodeLib() fails: 0x%llx\n", eresult);
	}

	getLastComponentFaultRec(pRowFaultRec, rowFault, &row_fault_count, dimmArray, dimmCount, rowFaultByDimm, ROWFAULT);
	for ( i=0; i<row_fault_count;i++ ) {
		rowOffLinedPageCurStart = rowOffLinedPageEnd;
		pageOfflineFromFault(rowFault[i], ROWFAULT, rowOffLinedPagesSysAddr, rowOffLinedPageCurStart, &rowOffLinedPageEnd);
	}	
	writeOffLinePages(rowOffLinedPagesSysAddr, &rowOffLinedPageStart, &rowOffLinedPageEnd);
	fclose(pRowFaultRec);
	
	writeComponentFaultRec(MRT_ROW_FAULT_REC, rowFault, row_fault_count, dimmArray, dimmCount);

	pRowFaultRec = fopen(MRT_ROW_FAULT_REC,"a+b");
	if(pRowFaultRec == NULL) {
		TCRIT("Unable to open %s\n", MRT_ROW_FAULT_REC);
		return NULL;
	}
	
	getLastComponentFaultRec(pCellFaultRec, cellFault, &cell_fault_count, dimmArray, dimmCount, cellFaultByDimm, CELLFAULT);
	for ( i=0; i<cell_fault_count;i++ ) {
		cellOffLinedPageCurStart = cellOffLinedPageEnd;
		pageOfflineFromFault(cellFault[i], CELLFAULT, cellOffLinedPagesSysAddr, cellOffLinedPageCurStart, &cellOffLinedPageEnd);
	}
	writeOffLinePages(cellOffLinedPagesSysAddr, &cellOffLinedPageStart, &cellOffLinedPageEnd);
	fclose(pCellFaultRec);
	
	writeComponentFaultRec(MRT_CELL_FAULT_REC, cellFault, cell_fault_count, dimmArray, dimmCount);
	
	pCellFaultRec = fopen(MRT_CELL_FAULT_REC,"a+b");
	if(pCellFaultRec == NULL) {
		TCRIT("Unable to open %s\n", MRT_CELL_FAULT_REC);
		return NULL;
	}
	
    for ( i=0; i< (int)dimmCount; i++ ) {
    	mfp_stat(dimmArray[i].loc, &statResult);
    	updateStatResult(dimmArray[i].loc, &statResult);
    }
	
    mfp_recent_faults(&recentFaults);
    memcpy(&rowAnchor, &recentFaults.rows[0],sizeof(rowAnchor));
    memcpy(&cellAnchor, &recentFaults.cells[0],sizeof(cellAnchor));
    
	while (1)
	{
		if (newShadowErrNum) {
			TDBG("newShadowErrNum is %d\n", newShadowErrNum);
			pthread_mutex_lock(&mfpDataMutex);
			newShadowErrNum = 0;
			pthread_mutex_unlock(&mfpDataMutex);
			mfp_recent_faults(&recentFaults);
#ifdef DEBUG
			print_mfp_faults(recentFaults);
		    TINFO("%d : rowAnchor ", __LINE__);
		    print_mfp_component(rowAnchor, 1);		    
		    TINFO("%d :cellAnchor ",__LINE__);
		    print_mfp_component(cellAnchor, 1);
#endif
			if ( rowOffLinedPageEnd < MAX_TOTAL_ROW_FAULT_PAGE_NUM ) {
				getNewFaultsFromRecent(&rowAnchor, rowFaultFromRecent, &rowFaultNumFromRecent, recentFaults.rows);
				memcpy(&rowAnchor, &recentFaults.rows[0], sizeof(rowAnchor));
				if (rowFaultNumFromRecent) {
					filterNewFaultByExistFault(rowFault, row_fault_count, 
						rowFaultFilterByRec, &rowFaultNumFilterByRec,
						rowFaultFromRecent, rowFaultNumFromRecent, ROWFAULT);
				}
				else {
					rowFaultNumFilterByRec = 0;
				}
				
				TINFO("rowFaultNumFilterByRec is %d\n", rowFaultNumFilterByRec);

				rowOffLinedPageStart = rowOffLinedPageEnd;
				for ( i=0; i<rowFaultNumFilterByRec; i++ ) {
					// reach cap per dimm?
					if ( !isCapReached(&rowFaultFilterByRec[i], rowFaultByDimm, ROWFAULT) ) {
						//addr trans
						rowOffLinedPageCurStart = rowOffLinedPageEnd;
						if ( !pageOfflineFromFault(rowFaultFilterByRec[i], ROWFAULT, rowOffLinedPagesSysAddr, rowOffLinedPageCurStart, &rowOffLinedPageEnd) ) {
							if ( row_fault_count < MAX_TOTAL_ROW_FAULT_NUM) {
								memcpy(&rowFault[row_fault_count++], &rowFaultFilterByRec[i], sizeof(rowFaultFilterByRec[i]));
								updateComponentFaultRec(pRowFaultRec, &rowFaultFilterByRec[i], dimmArray, dimmCount, ROWFAULT);

								//udpate rowFaultByDimm
								if ( 0 == getIndexOfDimm(rowFaultFilterByRec[i].socket, rowFaultFilterByRec[i].imc, rowFaultFilterByRec[i].channel, 
										rowFaultFilterByRec[i].dimm, &indexByDimm) ) {
									rowFaultByDimm[indexByDimm] += 1;
								}
								updateStatResultByFault(&rowFaultFilterByRec[i]);
							}
						}
					}
				}
				//page offline
				writeOffLinePages(rowOffLinedPagesSysAddr, &rowOffLinedPageStart, &rowOffLinedPageEnd);
			}
			else {
				TINFO("page offlining number for row fault %d, reach max, no more offlining\n", rowOffLinedPageEnd);
			}
			
			if ( cellOffLinedPageEnd < MAX_TOTAL_CELL_FAULT_PAGE_NUM ) {
				getNewFaultsFromRecent(&cellAnchor, cellFaultFromRecent, &cellFaultNumFromRecent, recentFaults.cells);
				memcpy(&cellAnchor, &recentFaults.cells[0], sizeof(cellAnchor));
				if (cellFaultNumFromRecent) {
					filterNewFaultByExistFault(cellFault, cell_fault_count, 
						cellFaultFilterByRec, &cellFaultNumFilterByRec,
						cellFaultFromRecent, cellFaultNumFromRecent, CELLFAULT);
				}
				else {
					cellFaultNumFilterByRec = 0;
				}
				
				TINFO("cellFaultNumFilterByRec is %d\n", cellFaultNumFilterByRec);
				// reach cap per dimm?
				cellOffLinedPageStart = cellOffLinedPageEnd;
				for ( i=0; i<cellFaultNumFilterByRec; i++ ) {
					if ( !isCapReached(&cellFaultFilterByRec[i], cellFaultByDimm, CELLFAULT) ) {
						//addr trans
						cellOffLinedPageCurStart = cellOffLinedPageEnd;
						if ( !pageOfflineFromFault(cellFaultFilterByRec[i], CELLFAULT, cellOffLinedPagesSysAddr, cellOffLinedPageCurStart, &cellOffLinedPageEnd) ) {
							if ( cell_fault_count < MAX_TOTAL_CELL_FAULT_NUM) {
								memcpy(&cellFault[cell_fault_count++], &cellFaultFilterByRec[i], sizeof(cellFaultFilterByRec[i]));
								updateComponentFaultRec(pCellFaultRec, &cellFaultFilterByRec[i], dimmArray, dimmCount, CELLFAULT);

								//udpate cellFaultByDimm
								if ( 0 == getIndexOfDimm(cellFaultFilterByRec[i].socket, cellFaultFilterByRec[i].imc, cellFaultFilterByRec[i].channel, 
										cellFaultFilterByRec[i].dimm, &indexByDimm) ) {
									cellFaultByDimm[indexByDimm] += 1;
								}
								updateStatResultByFault(&cellFaultFilterByRec[i]);
							}
						}
					}
				}
				//page offline
				writeOffLinePages(cellOffLinedPagesSysAddr, &cellOffLinedPageStart, &cellOffLinedPageEnd);
			}
			else {
				TINFO("page offlining number for cell fault %d, reach max, no more offlining\n", cellOffLinedPageEnd);
			}
			
#ifdef DEBUG
			printStatResult();
#endif			
		}
		else {
//			TDBG("No memory errors, sleep for %u seconds \n", DATA_MEMORY_FAULT_COLLECT_SLEEP);
			sleep(DATA_MEMORY_FAULT_COLLECT_SLEEP);
		}
	}

	return NULL;
}

int setDimm()
{
    redisContext *c = NULL;
    redisReply *reply;
    
    c = redisConnectUnix(REDIS_SOCK);
    if (c->err)
    {
    	TCRIT("redis connect unix fails\n");
        redisFree(c);
        return -1;
    }
    reply = redisCommand(c,"SET Redfish:Systems:Self:Memory:DevType2_DIMM0:Name %s","DevType2_DIMM0");   
    if (reply == NULL ) {
    	TCRIT("redis set fails\n");
    	return -1;
    }
    	
    reply = redisCommand(c,"SET Redfish:Systems:Self:Memory:DevType2_DIMM0:MemoryLocation:Socket %s","0");
    reply = redisCommand(c,"SET Redfish:Systems:Self:Memory:DevType2_DIMM0:MemoryLocation:MemoryController %s","0");
    reply = redisCommand(c,"SET Redfish:Systems:Self:Memory:DevType2_DIMM0:MemoryLocation:Channel %s","0");
    reply = redisCommand(c,"SET Redfish:Systems:Self:Memory:DevType2_DIMM0:MemoryLocation:Slot %s","0");
    reply = redisCommand(c,"SET Redfish:Systems:Self:Memory:DevType2_DIMM0:SerialNumber %s","00000001");
    reply = redisCommand(c,"SET Redfish:Systems:Self:Memory:DevType2_DIMM0:Status:State %s","Enabled");
    
    reply = redisCommand(c,"SET Redfish:Systems:Self:Memory:DevType2_DIMM1:Name %s","DevType2_DIMM1");   
    reply = redisCommand(c,"SET Redfish:Systems:Self:Memory:DevType2_DIMM1:MemoryLocation:Socket %s","0");
    reply = redisCommand(c,"SET Redfish:Systems:Self:Memory:DevType2_DIMM1:MemoryLocation:MemoryController %s","0");
    reply = redisCommand(c,"SET Redfish:Systems:Self:Memory:DevType2_DIMM1:MemoryLocation:Channel %s","0");
    reply = redisCommand(c,"SET Redfish:Systems:Self:Memory:DevType2_DIMM1:MemoryLocation:Slot %s","1");
    reply = redisCommand(c,"SET Redfish:Systems:Self:Memory:DevType2_DIMM1:SerialNumber %s","00000002");
    reply = redisCommand(c,"SET Redfish:Systems:Self:Memory:DevType2_DIMM1:Status:State %s","Enabled");
    
    reply = redisCommand(c,"SET Redfish:InventoryData:PostStatus:Status %s","Completed");
    freeReplyObject(reply);
    redisFree(c);
    
	return 0;
}

int getDimm(size_t *dimm_count, struct mfp_dimm_entry *dimm_arr, UINT16 *pDimmID)
{
	int i = 0;
	int j = 0;
    redisContext *c = NULL;
    redisReply *reply = NULL;
#ifdef CONFIG_SPX_FEATURE_MFP_2
    char * pEnd;
#endif
    
    c = redisConnectUnix(REDIS_SOCK);
    if (c->err)
    {
        redisFree(c);
        return -1;
    }
    
    for (i=0,j=0; i<memEntryCount; i++) {
        reply = redisCommand(c,"GET Redfish:Systems:%s:Memory:%s:Status:State", env_systems_name, memEntry[i]);       
        //TDBG("reply->str = %s, reply->type = %d \n", reply->str, reply->type);
        if (reply != NULL && reply->str != NULL) {
			if ( !strcmp(reply->str, "Enabled")) {
				pDimmID[j] = (UINT16)i;
				reply = redisCommand(c,"GET Redfish:Systems:%s:Memory:%s:MemoryLocation:Socket", env_systems_name, memEntry[i]);
				if ( reply != NULL ) {
					if (reply->str != NULL) {
						dimm_arr[j].loc.socket = ((UINT16)strtol((reply->str), NULL, 10)) & SOCKET_MASK;
					}
					else {
						if (reply->type == REDIS_REPLY_NIL) {
							TCRIT("Key Redfish:Systems:%s:Memory:%s:MemoryLocation:Socket not exist", env_systems_name, memEntry[i]);
						}
						return -1;
					}
				}
				else {
					TCRIT("ERROR, GET Redfish:Systems:%s:Memory:%s:MemoryLocation:Socket, Exit \n", env_systems_name, memEntry[i]);
					return -1;
				}
				
				reply = redisCommand(c,"GET Redfish:Systems:%s:Memory:%s:MemoryLocation:MemoryController", env_systems_name, memEntry[i]);
				if ( reply != NULL ) {
					if (reply->str != NULL) {
						dimm_arr[j].loc.imc = ((UINT16)strtol((reply->str), NULL, 10)) & IMC_MASK;
					}
					else {
						if (reply->type == REDIS_REPLY_NIL) {
							TCRIT("Key Redfish:Systems:%s:Memory:%s:MemoryLocation:MemoryController not exist", env_systems_name, memEntry[i]);
						}
						return -1;
					}
				}
				else {
					TCRIT("ERROR, GET Redfish:Systems:%s:Memory:%s:MemoryLocation:MemoryController, Exit \n", env_systems_name, memEntry[i]);
					return -1;
				}
				
				reply = redisCommand(c,"GET Redfish:Systems:%s:Memory:%s:MemoryLocation:Channel", env_systems_name, memEntry[i]);
				if ( reply != NULL ) {
					if (reply->str != NULL) {
						dimm_arr[j].loc.channel = ((UINT16)strtol((reply->str), NULL, 10))%2;    //Convert socket-based chan number to imc-based number
					}
					else {
						if (reply->type == REDIS_REPLY_NIL) {
							TCRIT("Key Redfish:Systems:%s:Memory:%s:MemoryLocation:Channel not exist", env_systems_name, memEntry[i]);
						}
						return -1;
					}			
				}
				else {
					TCRIT("ERROR, GET Redfish:Systems:%s:Memory:%s:MemoryLocation:Channel, Exit \n", env_systems_name, memEntry[i]);
					return -1;
				}
				
				reply = redisCommand(c,"GET Redfish:Systems:%s:Memory:%s:MemoryLocation:Slot", env_systems_name, memEntry[i]);
				if ( reply != NULL ) {
					if (reply->str != NULL) {
						dimm_arr[j].loc.dimm = ((UINT16)strtol((reply->str), NULL, 10)) & DIMM_MASK;
					}
					else {
						if (reply->type == REDIS_REPLY_NIL) {
							TCRIT("Key Redfish:Systems:%s:Memory:%s:MemoryLocation:Slot not exist", env_systems_name, memEntry[i]);
						}
						return -1;
					}
				}
				else {
					TCRIT("ERROR, GET Redfish:Systems:%s:Memory:%s:MemoryLocation:Slot, Exit \n", env_systems_name, memEntry[i]);
					return -1;
				}
				
				reply = redisCommand(c,"GET Redfish:Systems:%s:Memory:%s:SerialNumber", env_systems_name, memEntry[i]);
				if ( reply != NULL ) {
					if (reply->str != NULL) {
#ifdef CONFIG_SPX_FEATURE_MFP_2
						strtol((reply->str), &pEnd, 16);
						if(*pEnd == '-'){
                        	imm_arr[j].sn = strtoul(pEnd+1, NULL,16);						
						}
						else{
							TCRIT("Serial Number string %s is not according to SMBIOS Type 17 format. Eg xxxx-xxxxxxxx \n", reply->str);
						}
#elif defined (CONFIG_SPX_FEATURE_MFP_3)
						//EGS Bios Redfish just give serial number in xxxxxxxx for DDR5
						dimm_arr[j].sn = strtoul((reply->str), NULL, 16);
						TDBG("Key Redfish:Systems:%s:Memory:%s:MemoryLocation:SerialNumber is %x\n", env_systems_name, memEntry[i], dimm_arr[j].sn);
#endif
					}
					else {
						if (reply->type == REDIS_REPLY_NIL) {
							TCRIT("Key Redfish:Systems:%s:Memory:%s:MemoryLocation:SerialNumber not exist", env_systems_name, memEntry[i]);
						}
						dimm_arr[j].sn = 0xAAAAAAAA;  //default sn
					}

				}
				else {
					TCRIT("ERROR, GET Redfish:Systems:%s:Memory:%s:SerialNumber, Exit\n", env_systems_name, memEntry[i]);
					return -1;
				}
				
				reply = redisCommand(c,"GET Redfish:Systems:%s:Memory:%s:PartNumber", env_systems_name, memEntry[i]);
				if ( reply != NULL ) {
					if (reply->str != NULL) {
						TINFO("DIMM PartNumber is %s\n", reply->str);
						memset(dimm_arr[j].pn.s, 0, sizeof(dimm_arr[j].pn.s));
						if ((size_t)reply->len <= sizeof(dimm_arr[j].pn.s)) {
							strncpy(dimm_arr[j].pn.s, reply->str, (size_t)reply->len);
						}
						else{
							TCRIT("Key Redfish:Systems:%s:Memory:%s:PartNumber: %d exceed allowable length %u\n", env_systems_name, memEntry[i],reply->len,sizeof(dimm_arr[j].pn.s));
						}
					}
					else {
						if (reply->type == REDIS_REPLY_NIL) {
							TCRIT("Key Redfish:Systems:%s:Memory:%s:PartNumber: not exist", env_systems_name, memEntry[i]);
						}
					}
				}
				else {
					TCRIT("ERROR, GET Redfish:Systems:%s:Memory:%s:PartNumber, Exit\n", env_systems_name, memEntry[i]);
					return -1;
				}
				
#if defined(DEBUG)
				TDBG("Found DIMM %i, socket=%u, imc=%u, channel=%u, dimm=%u, sn=0x%x \n", i, dimm_arr[i].loc.socket, dimm_arr[i].loc.imc,
    			dimm_arr[j].loc.channel,dimm_arr[j].loc.dimm,dimm_arr[j].sn);
#endif

				j++;
			}
			else if ( !strcmp(reply->str, "Absent")){
#if defined(DEBUG)
				TDBG(" DIMM %i is Absent\n", i);
#endif
			}
        }
    }
    freeReplyObject(reply);
    redisFree(c);
    *dimm_count = j;
    TINFO("Found %d DIMMs \n", j);
    
#if defined(DEBUG)
    for (i=0; i<(int)*dimm_count; i++) {
    	TDBG("%d:	socket=%u, imc=%u, channel=%u, dimm=%u, sn=0x%x \n", i, dimm_arr[i].loc.socket, dimm_arr[i].loc.imc,
    			dimm_arr[i].loc.channel,dimm_arr[i].loc.dimm,dimm_arr[i].sn);
    }
#endif
	return 0;
}


int getMemEntries()
{
	redisContext *c = NULL;
	redisReply *reply = NULL;
	redisReply *rediselement = NULL;
	char *leading = "Redfish:Systems:Self:Memory:";
	char *p;
	unsigned int i;
	int ret = 0;
    
	c = redisConnectUnix(REDIS_SOCK);
	if (c->err)
	{
		redisFree(c);
		return -1;
	}

	reply = redisCommand(c,"zrange Redfish:Systems:%s:Memory:SortedIDs 0 -1", env_systems_name);
	if ( reply != NULL ) {
		TDBG("reply type = %d \n", reply->type );
		
		if (reply->type == REDIS_REPLY_ARRAY) {
			memEntryCount = reply->elements;
			for (i=0 ; i<reply->elements; i++) {
				rediselement = reply->element[i];
				TDBG("element reply type %d \n", rediselement->type);
				if (rediselement->type == REDIS_REPLY_STRING) {
					p = rediselement->str;
					if ( 0 == strncmp(p , leading, strlen(leading))) {
						p += strlen(leading);
						if ( strlen(p) < MEM_ENTRY_LEN ) {
							strcpy(memEntry[i], p);
							TDBG("extracted memory entry %s\n", (char *)memEntry[i]);
						}
						else {
							TCRIT("memory entry length is greater than allowed length\n");
							ret = -1;
						}
					}
					else {
						TCRIT("Not matching leading string\n");
						ret = -1;
					}
					TDBG("...%s...\n", rediselement->str);
				}
				else {
					TCRIT("rediselement->type is not string\n");
					ret = -1;
				}
			}
		}
		else {			
			TCRIT("zrange reply type is not array\n");			
			ret = -1;
		}			
	}
	else {
		TCRIT(" sorted id reply is NULL\n");
		ret = -1;
	}
    
    freeReplyObject(reply);
    redisFree(c);

    return ret;
}

int checkInventoryDataReady()
{
#define MAX_WAITTIME_REDIS		1200
#define MAX_WAITTIME_INVENTORY	3000 // 100 is too short. 10 sec * No. of "reply->str = true"

	redisContext *c = NULL;
    redisReply *reply;
	
    int	waitTime = 0;
    int retVal=0;

	while ( access( REDIS_SOCK, F_OK ) != 0 ) {
	    sleep(5);
	    waitTime += 5;
	    if ( waitTime > MAX_WAITTIME_REDIS) {
	    	TCRIT("Redis Sock is not ready after %d seconds, MFP exit\n", MAX_WAITTIME_REDIS);
	    	return -1;
	    }
    	TDBG("[MFP] Redis Sock is ready after %d seconds, MFP exit\n", waitTime);
	}
    
    c = redisConnectUnix(REDIS_SOCK);
    if (c->err)
    {
        redisFree(c);
    	TDBG("[MFP] redisConnectUnix() failed\n");
        return -1;
    }
    TDBG("[MFP] redisConnectUnix() done\n");
    
    while (1) {
    	reply = redisCommand(c,"GET Redfish:HostBooting:Status");
		if(reply != NULL && reply->str != NULL) {
			if (strcmp(reply->str, "false") == 0) {
				TINFO("BIOS booting is complete\n");
				break;
			}
			else {
				TINFO("Redfish:HostBooting:Status %s\n", reply->str);
				sleep(10);
				waitTime +=10;
			}
		}
		else {
			sleep(5);
			waitTime +=5;
		}
		if (waitTime > MAX_WAITTIME_INVENTORY) {
			retVal = -1;
			break;
		}
    }
    
    /* 
     * because Redfish:InventoryData:PostStatus:Status maybe
     * initialized Completed when HostBooting:Status is false after power cycle.
     * However Redfish:InventoryData:PostStatus:Status changes to BootInProgress 
     * if dimms are added or removed between power cycles.
     * The waiting time and the subsequent checking Redfish:InventoryData:PostStatus:Status 
     * ensure host inventory is really updated.
     */
    sleep(60);
    
    while (1) {
    	reply = redisCommand(c,"GET Redfish:InventoryData:PostStatus:Status");
		if(reply != NULL && reply->str != NULL) {
			if (strcmp(reply->str, "Completed") == 0) {
				TINFO("BIOS Inventory Data is ready\n");
				break;
			}
			else {
				TWARN("Redfish:InventoryData:PostStatus:Status %s\n", reply->str);
				sleep(10);
				waitTime +=10;
			}
		}
		else {
			sleep(5);
			waitTime +=5;
		}
		if (waitTime > MAX_WAITTIME_INVENTORY) {
			retVal = -1;
			break;
		}
    }
    
	freeReplyObject(reply);
	redisFree(c);

    return retVal;
}

int checkPipeDataAvail(int fdPipe, struct timeval *pTimeout)
{
    fd_set	fdRead;
    
    FD_ZERO (&fdRead);
    FD_SET (fdPipe, &fdRead);

    return sigwrap_select (fdPipe + 1, &fdRead, NULL, NULL, pTimeout);   
}

int getRedfishEnv()
{
	redisContext *c = NULL;
	redisReply *reply;
	
	c = redisConnectUnix(REDIS_SOCK);
	if (c->err)
	{
		redisFree(c);
		return -1;
	}
	
	reply = redisCommand(c,"GET ENV:SystemSelf");
	if(reply->str != NULL) {
		memcpy(env_systems_name, reply->str, strlen(reply->str));
	}
	else {
		//Only Self is allowed RTPVersion <=RTP1.5 
		memcpy(env_systems_name, "Self", strlen("Self"));
	}
	TDBG(" SystemSelf is %s \n", env_systems_name);

	freeReplyObject(reply);
	redisFree(c);	
	return 0;
}

int MemErrorStructToMFPError(MemErrorStruct  *pIPMIErr, struct mfp_error *pMFPErr)
{
	if ( (pIPMIErr == NULL) || (pMFPErr == NULL) ) {
		TCRIT(" pIPMIErr == NULL) or pMFPError == NULL");
		return -1;
	}
	pMFPErr->socket = pIPMIErr->socket & SOCKET_MASK;
	pMFPErr->imc = pIPMIErr->imc & IMC_MASK;
	pMFPErr->channel = pIPMIErr->channel & IMC_BASE_CHANNEL_MASK;
	pMFPErr->dimm = pIPMIErr->slot & DIMM_MASK;
	pMFPErr->rank = pIPMIErr->rank & RANK_MASK;
	pMFPErr->device = pIPMIErr->device & DEVICE_MASK;
	pMFPErr->bank_group = pIPMIErr->bankGroup & BG_MASK;
	pMFPErr->bank = pIPMIErr->bank & BANK_MASK;
	pMFPErr->row = pIPMIErr->row & ROW_MASK;
	pMFPErr->col = pIPMIErr->col & COLUMN_MASK;
	pMFPErr->error_type = pIPMIErr->errorType & UE_MASK;
	pMFPErr->par_syn = pIPMIErr->paritySyndrome;
	pMFPErr->mode = pIPMIErr->mode & MODE_MASK;
	return 0;
}

int getCPUNrTypeAndBus(INT8U *CPUCount, CpuTypes *cpu_type, INT8U *busNumber)
{
	INT8U i=0;
	INT8U CPUs=0;
	
	for (CPUs=0; CPUs<MAX_AMOUNT_OF_CPUS;CPUs++) {
		if( 0 == peci_Ping(MIN_CPU_ADDRESS + CPUs) )  {
			TINFO("Ping address 0x%x Success\n", (MIN_CPU_ADDRESS + CPUs));
		}
		else {
			break;
		}
	}
	if ( CPUs == 0 ) {
		TCRIT("No CPU is detected by PECI PING\n");
		return -1;;
	}
	
	*CPUCount = CPUs;
	
	for (i=0; i<CPUs;i++)  {
	    if ( 0 != ValidatePECIBus(i) || 0 !=  DetectCpuType(i, &cpu_type[i]) || 0 != RetrievePECIBus(i, &busNumber[i])) {
	    	TCRIT("CPU %d: Bus validation & CPU detection failed\n", i);
	    	return -1;
	    }
	
#ifdef wakePECIAfterHostReset
		if ( 0 != WakePECI(i) ) {
			return -1;
		}
#endif
	}

	return 0;
}

void *mfp2Thread(void *pArg) 
{
	UN_USED(pArg);
	INT8U	i, iCPU, iIMC, iChan, iSet;
	MemErrorStruct memErr[NUMBER_OF_MMIO_REGISTERS_SETS];
	bool validError[NUMBER_OF_MMIO_REGISTERS_SETS] = {false};
	struct mfp_error err = {0};
	INT32U	peciEn = 0;
	INT8U	j;
	INT32	retVal = 0;
	
	prctl(PR_SET_NAME,__FUNCTION__,0,0,0);

	TINFO("%s() Line %d: Pass PECI CPU Commands\n",__FUNCTION__, __LINE__);
	for (iCPU=0; iCPU<nrCPU; iCPU++) {
		for (iIMC=0; iIMC<NUMBER_OF_IMCS; iIMC++) {
			for (iChan=0; iChan<NUMBER_OF_CHANNELS; iChan++) {
				retVal = RecognizeTypeOfErrorsStoredInRetryLogRegisters(iCPU, bus[iCPU], iIMC, iChan);
#if defined CONFIG_SPX_FEATURE_MFP_3_1
				retVal = InitializeRetryRdErrLogValues(iCPU, bus[iCPU], iIMC, iChan);
				if(retVal == -1) {
					TCRIT("InitializeRetryRdErrLogValues(cpu %d imc %d chan %d) failed\n",iCPU, iIMC, iChan);
				}
				else
				{
					TINFO("InitializeRetryRdErrLogValues(cpu %d imc %d chan %d) is done\n",iCPU, iIMC, iChan);
				}
				TryToInitializeErrorHandlingForDimm(iCPU, bus[iCPU], iIMC, iChan);
#endif
			}
		}
	}

	while (1) {
		for (i=0; i<dimmCount; i++) {
			if (newErrNum < MAX_NEWERR-1) {
#if defined (TRACK_DETECTED_CORR_ERROR) && defined (MRT_DEBUG_TIME_STAMP)
				gettimeofday(&mrt_t0, 0);
#endif
				pthread_mutex_lock(&mfpDataMutex);
				iCPU = (INT8U)dimmArray[i].loc.socket;
				iIMC = (INT8U)dimmArray[i].loc.imc;
				iChan = (INT8U)dimmArray[i].loc.channel;
				/* coverity[sleep : FALSE] */
				LookForErrors(bus[iCPU], type[iCPU], iCPU, iIMC, iChan, memErr, validError);
				for (iSet=0; iSet<NUMBER_OF_MMIO_REGISTERS_SETS; iSet++) {
					/*******************************************************************************
					 * CE is collected; If non-fatal UCE that is not handled by host, we will collect it here
					 * Fatal UCE is handled by host, and is sent via ipmi oem command by BIOS
					 ******************************************************************************/
					if ( validError[iSet] ) {
						TDBG("validError[%u] true", iSet);
						MemErrorStructToMFPError(&memErr[iSet], &err);						
						memcpy(&newErr[newErrNum], &err, sizeof(err));
						newErrNum++;
						validError[iSet]=false;
						gettimeofday(&tlastErr, NULL);
						AddMFPSELEntries(&err);						
					}
				}
				pthread_mutex_unlock(&mfpDataMutex);
#if defined (TRACK_DETECTED_CORR_ERROR) && defined (MRT_DEBUG_TIME_STAMP)
				gettimeofday(&mrt_t1, 0);
				long elapsed = (mrt_t1.tv_sec-mrt_t0.tv_sec)*1000000 + mrt_t1.tv_usec-mrt_t0.tv_usec;
				printf("Elapsed LookForErrors() time %ld sec, %ld usec\n", elapsed/1000000, elapsed);
#endif
			}
			else {
				TDBG("Wait for MFP Data finish processing\n");
				sleep(1);
			}	
		}

		/*
		 * The sleep prevents the BMC from detecting many CE bursts..
		 * MFP thread doesn't use much CPU resource, so remove the sleep.
		 */
		/*sleep(WaitSecondsPeriodBetweenErrorPooling);*/

		for (j=0; j<nrCPU;j++)  {
			retVal = isPECIEnabled(j, &peciEn);
			if (retVal==0 && peciEn==0) {
				WakePECI(j);
			}
		}
	}
	return NULL;
}

#if defined CONFIG_SPX_FEATURE_MFP_3_1 && defined (MRT_CPU_HBM)
/* MRT3.1 specific to HBM*/
void *mfp2ThreadHbm(void *pArg)
{
	UN_USED(pArg);
	INT8U	i, iCPU, iIMC, iChan, iSet;
	MemErrorStruct memErr[NUMBER_OF_MMIO_REGISTERS_SETS];
	bool validError[NUMBER_OF_MMIO_REGISTERS_SETS] = {false};
	struct mfp_error err = {0};
	INT32U	peciEn = 0;
	INT8U	j;
	INT32	retVal;
	
	prctl(PR_SET_NAME,__FUNCTION__,0,0,0);

	for (iCPU=0; iCPU<nrCPU; iCPU++) {
		for (iIMC=0; iIMC<NUMBER_OF_IMCS; iIMC++) {
			for (iChan=0; iChan<NUMBER_OF_CHANNELS; iChan++) {
				retVal  = RecognizeTypeOfErrorsStoredInRetryLogRegistersHbm(iCPU, bus[iCPU], iIMC, iChan);
				retVal += InitializeRetryRdErrLogValuesHbm(iCPU, bus[iCPU], iIMC, iChan);
				if(retVal)
				{
					TCRIT("HBM cpu %d imc %d ch %d initialization failed - return %d\n",
							iCPU, iIMC, iChan, retVal);
				}
			}
		}
	}

	while (1) {
		for (i=0; i<dimmCount; i++) {
			if (newErrNum < MAX_NEWERR-1) {
				pthread_mutex_lock(&mfpDataMutex);
				iCPU = (INT8U)dimmArray[i].loc.socket;
				iIMC = (INT8U)dimmArray[i].loc.imc;
				iChan = (INT8U)dimmArray[i].loc.channel;
				/* coverity[sleep : FALSE] */
				LookForErrorsHbm(bus[iCPU], type[iCPU], iCPU, iIMC, iChan, memErr, validError);
				for (iSet=0; iSet<NUMBER_OF_MMIO_REGISTERS_SETS; iSet++) {
					/*******************************************************************************
					 * CE is collected; If non-fatal UCE that is not handled by host, we will collect it here
					 * Fatal UCE is handled by host, and is sent via ipmi oem command by BIOS
					 ******************************************************************************/
					if ( validError[iSet] ) {
						TDBG("HBM validError[%u] true", iSet);
						MemErrorStructToMFPError(&memErr[iSet], &err);						
						memcpy(&newErr[newErrNum], &err, sizeof(err));
						newErrNum++;
						validError[iSet]=false;
						gettimeofday(&tlastErr, NULL);
						AddMFPSELEntries(&err);						
					}
				}
				pthread_mutex_unlock(&mfpDataMutex);
			}
			else {
				TDBG("HBM Wait for MFP Data finish processing\n");
				sleep(1);
			}	
		}

		/*
		 * The sleep prevents the BMC from detecting many CE bursts..
		 * MFP thread doesn't use much CPU resource, so remove the sleep.
		 */
		/*sleep(WaitSecondsPeriodBetweenErrorPooling);*/
		for (j=0; j<nrCPU;j++)  {
			retVal = isPECIEnabled(j, &peciEn);
			if (retVal==0 && peciEn==0) {
				WakePECI(j);
			}
		}
	}
	return NULL;
}
#endif

int main(int argc, char* argv[])
{
	UN_USED(argc);
	UN_USED(argv);
	int fdFifo = 0;
	int fdValFifo = 0;
	pthread_t mfpMemFault;
	int readByte = 0;
	struct mfp_error err;
	mfpval_error	valerr;
	pthread_t mfpCompute;

	pthread_t mfp2ErrCollect;
#if defined CONFIG_SPX_FEATURE_MFP_3_1 && defined (MRT_CPU_HBM)
	/* MRT3.1 specific to HBM*/
	pthread_t mfp2ErrCollectHbm;
#endif
	size_t i;
	struct timeval readTimeout;
	int retVal = 0;
	struct mfp_stat_result *mstat=NULL;

	if(daemon_init() != 0) {
       TCRIT("Error Daemonizing !!!\n");
    }
    
    (void)umask( 0022 );
	
    /* save my pid */
	save_pid ("mfp");
	
    if (-1 == mkfifo (MFPQUEUE, 0777) && (errno != EEXIST))
    {
        TCRIT("Error creating named pipe %s\n", MFPQUEUE);
        goto END;
    }
   
    fdFifo = sigwrap_open (MFPQUEUE, O_RDWR);
    if (-1 == fdFifo)
    {
        TCRIT("Error opening named pipe %s\n", MFPQUEUE);
        goto END;
    }

    if (-1 == mkfifo (MFPVALQUEUE, 0777) && (errno != EEXIST))
    {
        TCRIT("Error creating named pipe %s\n", MFPVALQUEUE);
        goto END;
    }
    
    fdValFifo = sigwrap_open (MFPVALQUEUE, O_RDWR);
    if (-1 == fdValFifo)
    {
        TCRIT("Error opening named pipe %s\n", MFPVALQUEUE);
        goto END;
    }

	
    if (-1 == mkfifo (MFPFAULTQUEUE, 0777) && (errno != EEXIST))
    {
        TDBG("Error creating named pipe %s\n", MFPFAULTQUEUE);
        goto END;
    }

    fdFaultFifo = sigwrap_open (MFPFAULTQUEUE, O_RDWR|O_CREAT);
    if (-1 == fdFaultFifo)
    {
        TDBG("Error opening named pipe %s\n", MFPFAULTQUEUE);
        goto END;
    }

#if 0
    TDBG("MFP calls connect_signal(SIGUSR1)\n");
    connect_signal(SIGUSR1, mfp_sigusr1_handler, NULL);
#endif 
    
	gettimeofday(&tlastErr, NULL);
	gettimeofday(&tlastSave, NULL);

#if defined(EVB_DEBUG)
	setDimm();
	memcpy(memEntry[0], "DevType2_DIMM0", strlen("DevType2_DIMM0"));
	memcpy(memEntry[1], "DevType2_DIMM1", strlen("DevType2_DIMM1"));
	memcpy(env_systems_name, "Self", strlen("Self"));
#else
	
	if ( -1 == checkInventoryDataReady() ) {
		TCRIT("MFP failed: checkInventoryDataReady()\n");
		goto END;
	}
	TDBG("MFP: checkInventoryDataReady() is done\n");

	if ( -1 == getRedfishEnv() ) {
		TCRIT("MFP failed: getRedfishEnv()\n");
		goto END;
	}
	
	if ( -1 == getMemEntries() ) {
		TCRIT("MFP failed: getMemEntries()\n");
		goto END;
	}	

#endif
	
	if ( -1 == getDimm(&dimmCount, dimmArray, dimmID) ) {
		TCRIT("MFP failed: getDimm()\n");
		goto END;
	}
	TDBG("DIMM count %u\n", dimmCount);

	if ( -1 == getCPUNrTypeAndBus(&nrCPU, type, bus) ) {
		TCRIT("Error: Get CPU number, or CPU Type or Bus number\n ");
		goto END;
	}
#ifdef CONFIG_SPX_FEATURE_MFP_3
	//Always use first available dimm to get ecc mode for simplicity	
	eccMode = GetEccMode((INT8U) dimmArray[0].loc.socket,
			             (INT8U) bus[dimmArray[0].loc.socket],
			             (INT8U) ((device_of_first_imc + dimmArray[0].loc.imc)<<3));
	TINFO("GetEccMode(): %d", eccMode);
	
	DDR5ColWidth = GetCAWidth((INT8U) dimmArray[0].loc.socket, (INT8U) bus[dimmArray[0].loc.socket], 
							(INT8U)((device_of_first_imc + dimmArray[0].loc.imc)<<3),  (INT8U)dimmArray[0].loc.channel);
	if (DDR5ColWidth < 0 ) {
		TCRIT("Get DDR5 Column Width Error");
		goto END;
	}
	TINFO("DDR5 Column Width: %d", DDR5ColWidth);

#if defined CONFIG_SPX_FEATURE_MFP_3_1 && defined (MRT_CPU_HBM)
    /* To be filled with HBM */

#endif
#endif
	
    if (access( MFP_STAT_RESULT, F_OK ) != 0 ) {
    	fpStat = fopen(MFP_STAT_RESULT,"w+b");		
		if(fpStat == NULL) {
			TCRIT("Unable to create %s file\n", MFP_STAT_RESULT);
			goto END;
		}
		else {
			mstat = (struct mfp_stat_result *)calloc(MAX_DIMM_COUNT, sizeof(struct mfp_stat_result));
		    if( NULL == mstat)
		    {
		        TCRIT("Unable to Allocate Memory for MFP Stat Result\n");
		        goto END;
		    }
			
			TINFO("stat result mstat size is  %u \n", MAX_DIMM_COUNT*sizeof(struct mfp_stat_result));
			if ( 1 != fwrite(mstat, MAX_DIMM_COUNT*sizeof(struct mfp_stat_result), 1, fpStat)) {
				TCRIT("writing MFP Stat Result error\n");
				free(mstat);
				fclose(fpStat);
				goto END;
			}
			free(mstat);
			fclose(fpStat);
		}
    }

	results = malloc(dimmCount * sizeof(struct mfp_evaluate_result));
    if( NULL == results)
    {
        TCRIT("Unable to Allocate Memory for MFP results\n");
        goto END;
    }
    
    for (i = 0; i< dimmCount; i++) {
        results[i].loc = dimmArray[i].loc;
    }
    
	rowOffLinedPagesSysAddr = malloc(MAX_TOTAL_ROW_FAULT_PAGE_NUM * sizeof(unsigned long long));
    if( NULL == rowOffLinedPagesSysAddr)
    {
        TCRIT("Unable to Allocate Memory for rowOffLinedPagesSysAddr\n");
        goto END;
    }

	cellOffLinedPagesSysAddr = malloc(MAX_TOTAL_CELL_FAULT_PAGE_NUM * sizeof(unsigned long long));
    if( NULL == cellOffLinedPagesSysAddr)
    {
        TCRIT("Unable to Allocate Memory for cellOffLinedPagesSysAddr\n");
        goto END;
    }
	
	pRowFaultRec = fopen(MRT_ROW_FAULT_REC,"a+b");
	if(pRowFaultRec == NULL) {
		TCRIT("Unable to open %s\n", MRT_ROW_FAULT_REC);
		 goto END;
	}
	
	pCellFaultRec = fopen(MRT_CELL_FAULT_REC,"a+b");
	if(pCellFaultRec == NULL) {
		TCRIT("Unable to open %s\n", MRT_CELL_FAULT_REC);
		goto END;
	}	
    
    /* This thread keeps fetching memory errors from CPU using PECI */
	if (0 != pthread_create(&mfp2ErrCollect, NULL, mfp2Thread, NULL)) {
		TCRIT("Unable create mfp Compute thread\n");
		goto END;
	}

#if defined CONFIG_SPX_FEATURE_MFP_3_1 && defined (MRT_CPU_HBM)
	/* MRT3.1 specific to HBM*/
	/* This thread keeps fetching memory errors from CPU using PECI */
	if (0 != pthread_create(&mfp2ErrCollectHbm, NULL, mfp2ThreadHbm, NULL)) {
		TCRIT("Unable create HBM mfp Compute thread\n");
		goto END;
	}
#endif

	if (0 != pthread_create(&mfpCompute, NULL, computeMFPThread, NULL)) {
		TCRIT("Unable create mfp Compute thread\n");
		goto END;
	}

	if (0 != pthread_create(&mfpMemFault, NULL, colMemFaultThread, NULL)) {
		TCRIT("Unable create mfp Memory Fault Monitoring thread\n");
		goto END;
	}
	TDBG("Created mfp Memory Fault Monitoring thread: colMemFaultThread\n");


	if ( 0 != ProcMonitorRegister("/usr/local/bin/mfp",0,"mfp",mfp_signal_handler, 0)) {
		TCRIT("Process Monitor Register mfp fails\n");
	}

DATA_REC:
	while (1)
	{
		if ( access( MFP_VAL_KEY, F_OK ) == 0 ) {
			TINFO("Go to MFP Validation Data Receiving loop\n");
			break;
		}
		
		if (newErrNum < MAX_NEWERR) {
			readTimeout.tv_sec = PIPE_READ_TIMEOUT;
			readTimeout.tv_usec = 0;
			retVal = checkPipeDataAvail(fdFifo, &readTimeout);
			if ( retVal> 0 ) {
				readByte = sigwrap_read(fdFifo, (void *)&err, sizeof (struct mfp_error));
				if (sizeof (struct mfp_error) == readByte) {
					TDBG(" Get Data mfp: newErrNum = %d\n", newErrNum+1);
					pthread_mutex_lock(&mfpDataMutex);
					
					memcpy(&newErr[newErrNum], &err, sizeof(err));
					newErrNum++;
#ifdef DEBUG
					TDBG("MFP error count %d:", newErrNum);
			        for(i=0; (int)i<newErrNum;++i){
			        	TDBG("\t[%d] skt %d imc %d ch %d dimm %d rank %d device %d bg %d bank %d row 0x%x col 0x%x", i
			        		, newErr[i].socket, newErr[i].imc,  newErr[i].channel
							, newErr[i].dimm,   newErr[i].rank, newErr[i].device, newErr[i].bank_group
							, newErr[i].bank,   newErr[i].row,  newErr[i].col);
			        }
#endif
					gettimeofday(&tlastErr, NULL);
					AddMFPSELEntries(&err);
					
					pthread_mutex_unlock(&mfpDataMutex);
				}
				else {
					if (readByte == 0) {
						TCRIT(" no data is read \n");
					}
					if(errno == EINTR || errno == EAGAIN)
					{
						TCRIT(" reading mfp pipe gets error %d, readByte= %d", errno, readByte);
					}
					TWARN("Not get right size of data\n");
				}
			}		
			else if (retVal == 0) {
//				TDBG(" read %s time out %u seconds\n", MFPQUEUE, readTimeout.tv_sec);
			}
			else {
				TCRIT("%s \n", strerror(errno));
			}
		}
		else {
			TDBG("Wait for MFP Data finish processing\n");
			sleep(1);
		}
	}
	
// MFP Validation loop
	while (1)
	{
		if ( access( MFP_VAL_KEY, F_OK ) != 0 ) {
			TINFO("Go to MFP Data Receiving loop\n");
			goto DATA_REC;
		}
		
		if (newValErrNum < MAX_NEWERR) {
			readTimeout.tv_sec = PIPE_READ_TIMEOUT;
			readTimeout.tv_usec = 0;
			retVal = checkPipeDataAvail(fdValFifo, &readTimeout);
			if ( retVal> 0 ) {
				readByte = sigwrap_read(fdValFifo, (void *)&valerr, sizeof (valerr));
				if (sizeof (mfpval_error) == readByte) {
					TDBG(" Get Data mfp validation\n");
					pthread_mutex_lock(&mfpDataMutex);
					
					memcpy(&newValErr[newValErrNum], &valerr, sizeof(valerr));
					newValErrNum++;
					gettimeofday(&tlastErr, NULL);

					pthread_mutex_unlock(&mfpDataMutex);
				}
				else {
					if (readByte == 0) {
						TCRIT(" no data is read \n");
					}
					if(errno == EINTR || errno == EAGAIN)
					{
						TCRIT(" reading mfpval pipe gets error %d, readByte= %d", errno, readByte);
					}
					TWARN("Not get right size of data\n");
				}
			}		
			else if (retVal == 0) {
				TDBG(" read %s time out %u seconds\n", MFPVALQUEUE, readTimeout.tv_sec);
			}
			else {
				TCRIT("%s \n", strerror(errno));
			}
		}
		else {
			TDBG("Wait for MFP Val Data finish processing\n");
			sleep(1);
		}
	}

END:
	TCRIT("MFP Daemon fails to start\n");
	if (fdFifo > 0) {
		sigwrap_close(fdFifo);
	}
	
	if (fdValFifo > 0) {
		sigwrap_close(fdValFifo);
	}
	
	if(results != NULL) {
		free(results);
	}
	
	if (fpStat != NULL) {
		fclose(fpStat);  	
	}
	
	if (fdFaultFifo > 0) {
		sigwrap_close(fdFaultFifo);
	}
	
	if (pRowFaultRec) {
		fclose(pRowFaultRec);
	}
	
	if (pCellFaultRec) {
		fclose(pCellFaultRec);
	}
	
	if (rowOffLinedPagesSysAddr) {
		free(rowOffLinedPagesSysAddr);
	}
	
	if (cellOffLinedPagesSysAddr) {
		free(cellOffLinedPagesSysAddr);
	}
	
	return 0;
}

void mfp_comp_print(struct mfp_component_fault component_fault) {
	printf("\n\t\t\tRank-Device-BGroup-Bank: %d-%d-%d-%d", component_fault.rank, component_fault.device, component_fault.bank_group, component_fault.bank);
	printf("\n\t\t\tComponent grain: %d", component_fault.grain);
	printf("\n\t\t\tComponent prone: %d", component_fault.prone);
	printf("\n\t\t\tRow range: 0x%x - 0x%x", component_fault.min_row, component_fault.max_row);
	printf("\n\t\t\tColumn range: 0x%x - 0x%x", component_fault.min_col, component_fault.max_col);
	printf("\n");
}

void mfp_stat_print(struct mfp_stat_result* mstat) {
	if(mstat->err_count != 0){
		printf("\tStat info:");
		printf("\n\t\tGrain: %d", mstat->grain);
		printf("\n\t\tError_count: %llu", mstat->err_count);

		if(mstat->hard_error_grain != 0) {
			printf("\n\t\tHard_error_grain: %d", mstat->hard_error_grain);
		}

		if(mstat->err_storm)
			printf("\n\t\tError_storm has triggered");

		if(mstat->err_daily_threshold)
			printf("\n\t\tError daily threshold has triggered");

		if(mstat->row_fault_count != 0) {
			printf("\n\t\tRow_fault_count: %d", mstat->row_fault_count);
			printf("\n\t\tTop Rows info:");
			for(int i = 0; i < TOPN; i++) {
				if(mstat->topN_row_fault[i].valid) {
					mfp_comp_print(mstat->topN_row_fault[i]);
				}
			}
		}

		if(mstat->col_fault_count != 0) {
			printf("\n\t\tCol_fault_count: %d", mstat->col_fault_count);
			printf("\n\t\tTop Columns info:");
			for(int i = 0; i < TOPN; i++) {
				if(mstat->topN_col_fault[i].valid){
					mfp_comp_print(mstat->topN_col_fault[i]);
				}
			}
		}

		if(mstat->bank_fault_count != 0) {
			printf("\n\t\tBank_fault_count: %d", mstat->bank_fault_count);
			printf("\n\t\tTop Banks info:");
			for(int i = 0; i < TOPN; i++) {
				if(mstat->topN_bank_fault[i].valid) {
					mfp_comp_print(mstat->topN_bank_fault[i]);
				}
			}
			printf("\n");
		}
		printf("\n");
	}
}

int getIndexOfDimm(INT8U socket, INT8U imc, INT8U channel, INT8U slot, INT32U *ind)
{
	if (socket>SOCKET_MASK || imc>IMC_MASK || channel>IMC_BASE_CHANNEL_MASK || slot>DIMM_MASK) {
		TCRIT("socket=%u, imc=%u, channel=%u, slot=%u out of range\n", socket, imc, channel, slot);
		return -1;
	}
	
	*ind = socket*16 + imc*4 + channel*2 + slot;
	return 0;
}

int updateStatResult(struct mfp_dimm dimm, struct mfp_stat_result *result)
{
	INT32U dimmInd = 0;
	
	if (getIndexOfDimm(dimm.socket, dimm.imc, dimm.channel, dimm.dimm, &dimmInd)) {
		return -1;
	}

	fpStat = fopen(MFP_STAT_RESULT,"rb+");		
	if(fpStat == NULL) {
		TCRIT("Unable to open %s file\n", MFP_STAT_RESULT);
		return -1;
	}
	
	if ( 0 != fseek(fpStat, dimmInd*sizeof(struct mfp_stat_result), SEEK_SET)) {
		TCRIT("fseek returns error, not update MFP Stat Result\n");
	}
	else {
		if ( 1 != fwrite(result, sizeof(struct mfp_stat_result), 1, fpStat) ) {
			TCRIT("Update MFP Stat Result error\n");
			fclose(fpStat);
			return -1;
		}
	}
	fclose(fpStat);
	return 0;
}


int updateStatResultByFault(struct mfp_component *fault)
{
	struct mfp_dimm	dimm;
	struct mfp_stat_result result;
	
	dimm.socket = fault->socket;
	dimm.imc = fault->imc;
	dimm.channel = fault->channel;
	dimm.dimm = fault->dimm;
	dimm.reserved = 0;
		
	mfp_stat(dimm, &result);	
	updateStatResult(dimm, &result);
	return 0;
}


int updateStatResultByMemErr(struct mfp_error *memErr)
{
	struct mfp_dimm	dimm;
	struct mfp_stat_result result;
	
	dimm.socket = memErr->socket;
	dimm.imc = memErr->imc;
	dimm.channel = memErr->channel;
	dimm.dimm = memErr->dimm;
	dimm.reserved = 0;
		
	mfp_stat(dimm, &result);	
	updateStatResult(dimm, &result);
	return 0;
}


int printStatResult()
{
	INT32U i = 0;
	struct mfp_stat_result tmpResult;
	
	fpStat = fopen(MFP_STAT_RESULT,"rb+");		
	if(fpStat == NULL) {
		TCRIT("Unable to open %s file\n", MFP_STAT_RESULT);
		return -1;
	}
	
	while ( 1 == fread(&tmpResult, sizeof(struct mfp_stat_result), 1, fpStat) ) {
		if (tmpResult.err_count != 0) {
			printf("DIMM%u: Scoket-IMC-Channel-Slot: %u-%u-%u-%u\n", i, i/16, (i%16)/4, (i%4)/2, i%2);
			mfp_stat_print(&tmpResult);
		}
		i++;
	}
	
	fclose(fpStat);
	return 0;
}

int print_mfp_component(struct mfp_component x, int printhex)
{
	uint64_t data = 0;
	if (!printhex) {
		printf("sock:%1u imc:%1u chan:%1u slot:%1u ", x.socket, x.imc, x.channel, x.dimm);
#if defined (CONFIG_SPX_FEATURE_MFP_2)
		printf("rank:%1u dev:%1u bg:%1u bk:%1u ", x.rank, x.device, x.bank_group, x.bank);
#elif defined (CONFIG_SPX_FEATURE_MFP_3)
		/* DDR5 ChipSelect field definition       mc.addTran() DRAM address translation
		 * Bit 0: sub-channel                     Physical Sub-Channel (0-1)
		 * Bit 1: physical rank ID                Rank Select (0-1)
		 * Bit 2: DIMM index                      Dimm/Slot (0-1)
		 */
#define CS_RANK_BIT       1
		printf("rank:%1u (SubCh:%d PhyRank %d) dev:%1u bg:%1u bk:%1u ", x.rank, x.rank&1, (x.rank>>CS_RANK_BIT)&1, x.device, x.bank_group, x.bank);
#endif
		printf("row:0x%5x col:0x%3x valid:%1u\n", x.row, x.col, x.valid);
	}
	else {
		memcpy(&data, &x, sizeof(data));
		printf("data = 0x%llx\n", data);
	}
	return 0;
}

int print_mfp_faults(struct mfp_faults faults)
{
	int i = 0;
	printf("Row Faults\n");
	for ( i=0; i<FAULTN; i++ ) {
		printf("%d: ", i);
		print_mfp_component(faults.rows[i], 0);
	}
	printf("Cell Faults\n");
	for ( i=0; i<FAULTN; i++ ) {
		printf("%d: ", i);
		print_mfp_component(faults.cells[i], 0);
	}
	return 0;
}
