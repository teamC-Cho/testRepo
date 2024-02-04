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
 1.	MFP 1.0
  *	All memory errors
	-	BIOS -> IPMI(libipmioemmfp) -> BMC MFP main()
	-	Used pipe: MFPQUEUE "/var/MFPQUEUE"
	-	BIOS collects all memory error and sends it to BMC computeMFPThread() thread via IPMI

 2.	MFP 2.0
  * Include MFP 1.0 features
  *	Uncorrectable error
	-	BIOS -> IPMI (libipmiamioemmfpvalid) -> BMC MFP main()
	-	Used pipe: MFPVALQUEUE "/var/MFPVALQUEUE"
	-	BIOS collects uncorrectable error and sends it to BMC computeMFPThread() thread via IPMI
  *	Correctable error
	-	BMC mfp2Thread() thread collects correctable memory error data
	-	BIOS is not involved

 3.	MFP 2.1
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
	
 4. Channel number index
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
#if defined (CONFIG_SPX_FEATURE_MFP_2_0) || defined (CONFIG_SPX_FEATURE_MFP_2_1)
#include "peciifc.h"
#include "cpu.h"
#include "channel.h"
#endif
#include "libipmi_session.h"
#include "libipmi_StorDevice.h"
#include "IPMI_SEL.h"
#include "SEL_OEMRcdType.h"
#include "featuredef.h"
#include "AddressDecodeLib.h"

/*** test purpose defs ***/
/* Define EVB_DEBUG in case local evaluation board is used instead of a real Wilsoncity host */
//#define	EVB_DEBUG		1

/*
 * If defined, a fake error can be injected by IPMItool
 * e.g. ipmitool -U admin -P admin123 -I lanplus -H 172.31.8.151 raw
 *      0x30 0x45 1 2 1 0 1 1 1  0 1 0 0  0 1 0 0 1 0 0 0 1
 */
//#define MFP21_TEST_USE_BIOS_DATA

/***********/
#define	MAX_NEWERR		32
#define SLEEP_THRESH	16
#define ELAPSE_LIMIT	3
#if !defined(DEBUG)
#define SNAPSHOT_SAVE_INTERVAL  (24*60*60)
#else
#define SNAPSHOT_SAVE_INTERVAL  200
#endif
#define DATA_PROC_DEFER_TIME	60
#define DATA_REC_SLEEP			5
#define DATA_MEMORY_FAULT_TRANSFER_SLEEP  5
#define DATA_MEMORY_FAULT_COLLECT_SLEEP   5
#define CPU_DETECT_INTERVAL     300

#define PIPE_READ_TIMEOUT		10
#define PIPE_WRITE_TIMEOUT		10
#define MFP_SNAPSOT		"/conf/mfp_snapshot"
#define DIMM_MAXCNT		256
#define REDIS_SOCK		"/run/redis/redis.sock"
#define REDIS_LENGTH 100
#define MEM_ENTRY_LEN	32

/* Enumerate the column 128 times: 0 to (1024-8) */
//#define ADDR_TRANS_ENUMERATE_COLUMN
#define ADDR_TRANS_MASK_4K       ((unsigned long long) (~0x0FFF))
#define ADDR_TRANS_COL_ADD_MIN   ((unsigned long long) (0x0000))
#define ADDR_TRANS_COL_ADD_MAX   ((unsigned long long) (0x03FF))

#if defined (CONFIG_SPX_FEATURE_MFP_2_0) || defined (CONFIG_SPX_FEATURE_MFP_2_1)
#define wakePECIAfterHostReset	1
#define WaitSecondsPeriodBetweenErrorPooling	1
#endif

static INT32	gSigUSR1=0;
static struct timeval tReportStart, tReportEnd;
static struct timeval tlastErr, tCur, tlastSave;
pthread_mutex_t	mfpDataMutex = PTHREAD_MUTEX_INITIALIZER;
static INT32	newErrNum = 0;
static INT32	newValErrNum = 0;
struct mfp_error	newErr[MAX_NEWERR];
mfpval_error	newValErr[MAX_NEWERR];

#if defined (CONFIG_SPX_FEATURE_MFP_2_1)
static struct mfp_faults_ipmi memFaultIPMI = {0};
//mem_fault_link *head = NULL;
static int fdFaultFifo = 0;

typedef void (*pPDKFunc) (void );
void *dl_pdkhandle = NULL;
static int pdkSCIFuncsInited = 0;
static int pdkSCIInited = 0;
static pPDKFunc pdkSCIInitFunc = NULL;
static pPDKFunc pdkSCITrigFunc = NULL;
static INT32 dimmForStatNum = 0;
static struct mfp_dimm dimmShadowForStat[MAX_DIMM_COUNT];
#ifndef MFP21_TEST_USE_BIOS_DATA
static struct mfp_stat_result mstat[MAX_DIMM_COUNT];
#endif
#endif

static size_t	dimmCount = 0;
static INT32	memEntryCount = 0;		//SLOT Count
static struct mfp_dimm_entry dimmArray[MAX_DIMM_COUNT];

#if defined (CONFIG_SPX_FEATURE_MFP_2_0) || defined (CONFIG_SPX_FEATURE_MFP_2_1)
static struct mfp_dimm_entry dimmArrayVal[MAX_DIMM_COUNT];
#endif

struct mfp_evaluate_result *results = NULL;
static UINT16	dimmID[MAX_DIMM_COUNT];
static char		memEntry[MAX_DIMM_COUNT][MEM_ENTRY_LEN] =  {{0}};
static FILE 	*fp = NULL;
static FILE		*fpSave	= NULL;
static char env_systems_name[REDIS_LENGTH] = {0};
static INT32	redfishReportInit = 0;

static volatile INT8U	nrCPU = 0;
static CpuTypes type[MAX_AMOUNT_OF_CPUS];
static INT8U	bus[MAX_AMOUNT_OF_CPUS];


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

void mfp_signal_handler(int signum)
{
	UN_USED(signum);
	int retVal = 0;
	FILE *f = NULL;

    // clear error number, or mfp_save encounters error
    errno = 0;
	
	if (fpSave != NULL) {
		fclose(fpSave);
		TDBG(" fpSave of %s is open, close fpSave\n", MFP_SNAPSOT);
	}
	else {
		TDBG("fpSave of %s is already closed\n", MFP_SNAPSOT);		
	}

	if ( access( MFP_VAL_KEY, F_OK ) != 0 ) {
	    errno = 0;
		f = fopen(MFP_SNAPSOT,"wb");
	
		if(f == NULL) {
			TCRIT("Unable to open %s file\n", MFP_SNAPSOT);
		}
		else {
			retVal = mfp_save(f);
			if(retVal != MFP_OK) {
				TCRIT("mfp_save error, retVal=%d\n", retVal);
				perror("mfp_save system error number : ");			
			} 
			else {
				TINFO("%s is saved \n", MFP_SNAPSOT);
			}
			fclose(f);
		}
	}
	retVal = mfp_fin();
	if(retVal != MFP_OK) {
		TCRIT("mfp_fin meet error, retVal=%d\n", retVal);
		if ( retVal == MFP_SYS_ERR) {
			perror("mfp_fin system error number : ");
		}
	}

    if(results != NULL) {
    	free(results);
    }

#ifdef   CONFIG_SPX_FEATURE_MFP_2_1
    if (dl_pdkhandle) {
    	dlclose(dl_pdkhandle);
    }
#endif

    ProcMonitorDeRegister("/usr/local/bin/mfp");
	if (delete_pid_file("mfp")) {
		TWARN("Unable to delete the pid file, error = %d\n", errno);
	}
    exit(0);
}

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
     *	OEMData[4] : least significant 2 bits of Column
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
	
//#if defined(DEBUG)
	time_t evalSec = 0;
	suseconds_t evaluSec=0;
	struct timeval	tEval;
//#endif
#if defined (CONFIG_SPX_FEATURE_MFP_2_0) || defined (CONFIG_SPX_FEATURE_MFP_2_1)
	FILE *fKey;
	int c;
	int pnLen = 0;
	struct mfp_part_number pnVal;
#endif

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
				fpSave = fopen(MFP_SNAPSOT,"wb");

				if(fpSave != NULL) {
					retVal = mfp_save(fpSave);
					if(retVal != MFP_OK) {
						TCRIT("mfp_save error, retVal=%d\n", retVal);
						perror("mfp_save system error number : ");	
					}
					else {
						TINFO("%s is saved \n", MFP_SNAPSOT);
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
			fp = fopen(MFP_SNAPSOT,"rb");
			if(fp == NULL) {
				TCRIT("Unable to open %s file, not necessarily error because snapshot may not exist in the beginning\n", MFP_SNAPSOT);
			}
			//file size is 0, close it because mfp_init doesn't take 0 byte file
			if (fp != NULL ) {
				fseek (fp, 0, SEEK_END);
				if ( 0 == ftell (fp) ) {
					TINFO("%s size 0 byte\n", MFP_SNAPSOT);
					fclose(fp);
					fp = NULL;
				}
			}

			retVal = mfp_init(time(0), fp, dimmCount, dimmArray);
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

				pthread_mutex_lock(&mfpDataMutex);
				TINFO("process %d mfp data in single evaluation\n", newErrNum);				
	            retVal = mfp_evaluate_dimm(time(0), newErrNum, newErr, dimmCount, results);
	            if(retVal != MFP_OK) {
	                TCRIT("mfp_evaluate_dimm meet error, retVal=%d\n", retVal);
	            }

#ifdef CONFIG_SPX_FEATURE_MFP_2_1
	            dimmForStatNum = getDimmsForStat(dimmShadowForStat, dimmForStatNum, newErr, newErrNum);
#endif
				newErrNum = 0;
				pthread_mutex_unlock(&mfpDataMutex);
				
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
	        fpSave = fopen(MFP_SNAPSOT,"wb");

	        if(fpSave == NULL) {
        		TCRIT("Unable to open %s file\n", MFP_SNAPSOT);
        	}
	        else {
				retVal = mfp_save(fpSave);
				if(retVal != MFP_OK) {
					TCRIT("mfp_save error, retVal=%d\n", retVal);
				}
				else {
					TDBG("%s is saved \n", MFP_SNAPSOT);
				}
				fclose(fpSave);
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
#if defined (CONFIG_SPX_FEATURE_MFP_2_0) || defined (CONFIG_SPX_FEATURE_MFP_2_1)
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
			retVal = mfp_init(0, NULL, dimmCount, dimmArrayVal);
#else
			retVal = mfp_init(0, NULL, dimmCount, dimmArray);
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
				pthread_mutex_lock(&mfpDataMutex);
				TDBG("process %d mfp validation error data one by one\n", newValErrNum);
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

#if defined (CONFIG_SPX_FEATURE_MFP_2_1)

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
				TINFO("dimm associated with error[%d] already exists\n", i);
				TDBG("i=%d, k=%d, socket %u, imc %u, channel %u, dimm %u\n", i, k, (pError+i)->socket, (pError+i)->imc, (pError+i)->channel, (pError+i)->dimm );
				break;
			}
			k++;
		}
		if (k==j) {
			TINFO("dimm associated with error[%d] does not exist,add a new dimm for mfp_stat\n", i);
			TDBG("__i=%d, k=%d, socket %u, imc %u, channel %u, dimm %u\n", i, k, (pError+i)->socket, (pError+i)->imc, (pError+i)->channel, (pError+i)->dimm );
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

/* check if the newly translated system addres is already in offline page record 
 * if the system address is new, return 1. otherwise, return 0
 * */
int isNewPageAddress(unsigned long long *offlinePageRecord, int size, unsigned long long sysAddr)
{
	int i;
	int retval = 1;

	for (i=0; i<size; i++)  {
		if (*(offlinePageRecord+i)==sysAddr) {
			retval=0;
			break;
		}
	}
	return retval;
}

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

void *colMemFaultThread(void *pArg)
{
	sigset_t   mask;
	int writtenByte = 0, writeByte;
	int tx_count, cpu_wait = 0;

	int i;
	int offLinedPages = 0;
	unsigned long long offLinedPagesSysAddr[MAX_TOTAL_FAULT_PAGE_NUM] = {0xFFFFFFFFFFFFFFFF};
	int row_fault_counts = 0;
	int row_fault_index  = 0;
#ifndef MFP21_TEST_USE_BIOS_DATA
	int offLinedRows = 0;
	struct mfp_mem_fault_t offLinedRowsRec[MAX_TOTAL_FAULT_ROW_NUM] = {0};
	int dimmForStatLoopNum;	
	/*  run first time mfp_stat on all dimms after boot  */
	int firstMFPStatSinceBoot = 1;
	dimmFaultCount faultsOnDimm[MAX_DIMM_COUNT] = {0};
#endif	
    /* Used for ADDRESS_TRANSLATION */
	dimmBDFst dimmBdp;
	TRANSLATED_ADDRESS  TranslatedAddress = {0};
	EFI_STATUS          Status;
	struct mfp_mem_fault_t fault_new;
	
	UN_USED(pArg);
	sigfillset(&mask);
	sigprocmask(SIG_SETMASK, &mask, NULL);
	prctl(PR_SET_NAME,__FUNCTION__,0,0,0);
	
	/* Use constant column address to avoid duplication due to different columns */
	fault_new.min_col = ADDR_TRANS_COL_ADD_MIN;
	fault_new.max_col = ADDR_TRANS_COL_ADD_MAX;

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
	EFI_STATUS eresult = InitAddressDecodeLib(nrCPU);
	if( EFI_SUCCESS !=  eresult) {
		TCRIT("InitAddressDecodeLib() fails: 0x%llx\n", eresult);
	}

	while (1)
	{

		/* Transfer the fault data to IPMI */
		TDBG("mfp: memory fault = %d\n", row_fault_counts);
		while (row_fault_counts > 0) {
			//TDBG("Write mfp fault data (%d) on a pipe for IPMI\n", row_fault_counts);
/*			if (fdFaultFifo <= 0) {
				if (-1 == (fdFaultFifo = sigwrap_open (MFPFAULTQUEUE, O_WRONLY))) {
					TCRIT("can not open MFPFAULTQUEUE Pipe \n");
					sleep(1);
					break;
				}
			}
*/
       	    tx_count = row_fault_counts >= MAX_FAULT_ERR? MAX_FAULT_ERR : row_fault_counts;
   	    	memFaultIPMI.counts = tx_count;

			/*
			 * Transmit the memory fault data over FIFO
			 */
       	    /* Tx 1: record size */
			writtenByte = sigwrap_write(fdFaultFifo, (void *)&memFaultIPMI.counts, sizeof(memFaultIPMI.counts));
			/* Tx 2: records */
			writeByte   = tx_count * sizeof(struct mfp_mem_fault_t);
			writtenByte = sigwrap_write(fdFaultFifo, (void *)&memFaultIPMI.fault_result[row_fault_index], writeByte);
			if (writeByte == writtenByte) {
				TCRIT("mfp fault data was written: %d records\n", tx_count);
				/*
				 * Trigger SCI pin
				 */
				TDBG("Trigger the SCI pin to notify BIOS of memory fault\n");
				triggerSci();
				sleep(1);	//Give host and bios 1 second to handle SCI
			} else {
				if (writtenByte == 0) {
					TCRIT("no mfp fault data is written\n");
				}
				if(errno == EINTR || errno == EAGAIN) {
					TCRIT("writing mfp fault pipe gets error %d, writtenByte= %d", errno, writtenByte);
				}
				TCRIT("Not get right size of data\n");
			}
			row_fault_counts -= tx_count;
			row_fault_index  += tx_count;
		}

		/* Collect fault memory info */
	    /* Collect only when fault count is zero because there is a situation
	     * that fault count exceeds maximal transferable size, MAX_FAULT_ERR
	     * In that case, data is being split into multiple records.
	     */
#ifdef MFP21_TEST_USE_BIOS_DATA
		if(dimmForStatNum)
		{
			row_fault_index = offLinedPages;
			row_fault_counts = 0;
			TCRIT("mfp: dimmForStatNum = %d\n", dimmForStatNum);
			for(i=0; i< (int) dimmForStatNum; i++) {
				fault_new.faultType = MEM_FAULT_ROW;
				fault_new.socket    = newErr[i].socket;
				fault_new.imc       = newErr[i].imc;
				fault_new.channel   = newErr[i].channel;
				fault_new.slot      = newErr[i].dimm;

				fault_new.rank      = newErr[i].rank;
				fault_new.device    = newErr[i].device;
				fault_new.bankgroup = newErr[i].bank_group;
				fault_new.bank      = newErr[i].bank;
				fault_new.min_row   = newErr[i].row;
				fault_new.max_row   = newErr[i].row;

				/***** Start of ADDRESS_TRANSLATION *****/
				/*
				 * Reverse Address translation: DIMM to physical address
				 */
				TranslatedAddress.SocketId           = newErr[i].socket;
				TranslatedAddress.MemoryControllerId = newErr[i].imc;
				TranslatedAddress.ChannelId          = newErr[i].channel%2;   //AMI Channel to Intel Channel
				TranslatedAddress.DimmSlot           = newErr[i].dimm;
				TranslatedAddress.PhysicalRankId     = newErr[i].rank;
				TranslatedAddress.BankGroup          = newErr[i].bank_group;
				TranslatedAddress.Bank               = newErr[i].bank;
				TranslatedAddress.Row                = newErr[i].row;

				dimmBdp.cpuType  = (uint8_t) type[newErr[i].socket];
				dimmBdp.bus      = bus[newErr[i].socket];
				dimmBdp.socket   = newErr[i].socket;
				dimmBdp.imc      = newErr[i].imc;
				dimmBdp.channel  = newErr[i].channel % 2;   //AMI Channel to Intel Channel

		#ifdef ADDR_TRANS_ENUMERATE_COLUMN
				for(int col=0; col<1024; col+=8)
				{
					TranslatedAddress.Col = col;

					Status = DimmAddressToSystemAddress(&dimmBdp, &TranslatedAddress);
					if (Status) {
						TCRIT("[col 0x%x]: DimmAddressToSystemAddress() Error: 0x%llx\n", col, Status);
						/* This is a just test. Therefore, do not break although error happens */
						//break ;
					} else {
						TCRIT("[col 0x%x]: DimmAddressToSystemAddress(): address 0x%llx\n", col, TranslatedAddress.SystemAddress);
					}

					/* 4K page masking */
					fault_new.phy_addr = ADDR_TRANS_MASK_4K & TranslatedAddress.SystemAddress;
					/***** End of ADDRESS_TRANSLATION *****/

					/* Process only new data */
//					if((is_new_fault(&head, &fault_new) == 1)  && (offLinedPages<MAX_TOTAL_FAULT_PAGE_NUM)) {
					if (isNewPageAddress(offLinedPagesSysAddr, offLinedPages, fault_new.phy_addr) && (offLinedPages<MAX_TOTAL_FAULT_PAGE_NUM)) {
						TCRIT("New page address 0x%llx \n", fault_new.phy_addr);
						memcpy(&memFaultIPMI.fault_result[offLinedPages], &fault_new, sizeof(struct mfp_mem_fault_t));
						offLinedPagesSysAddr[offLinedPages] = fault_new.phy_addr;
						++offLinedPages;
						++row_fault_counts;
						if(MAX_TOTAL_FAULT_PAGE_NUM == offLinedPages){
							TCRIT("The number of memory fault reaches storage limit: %d\n", offLinedPages);
							break;
						}
					}
					else
					{
						TCRIT("0x%llx is not new page address. Ignored\n", fault_new.phy_addr);
					}
				}
		#else
				TranslatedAddress.Col                = newErr[i].col;
				/* Process only new data */
//				if(is_new_fault(&head, &fault_new) == 1) {

				Status = DimmAddressToSystemAddress(&dimmBdp, &TranslatedAddress);
				if (Status) {
					TCRIT("RAS: DimmAddressToSystemAddress() Error: 0x%llx\n", Status);
					/* This is a just test. Therefore, do not break although error happens */
				} else {
					TCRIT("RAS: DimmAddressToSystemAddress(): address 0x%llx\n", TranslatedAddress.SystemAddress);
				}

				fault_new.phy_addr = TranslatedAddress.SystemAddress;
				/***** End of ADDRESS_TRANSLATION *****/
				if(offLinedPages<MAX_TOTAL_FAULT_PAGE_NUM) {
					memcpy(&memFaultIPMI.fault_result[offLinedPages], &fault_new, sizeof(struct mfp_mem_fault_t));
					//offLinedPagesSysAddr[offLinedPages] = fault_new.phy_addr;
					++offLinedPages;
					++row_fault_counts;
					if(MAX_TOTAL_FAULT_PAGE_NUM == offLinedPages){
						TCRIT("The number of memory fault reaches storage limit: %d\n", offLinedPages);
						break;
					}
				}

		#endif /* ADDR_TRANS_ENUMERATE_COLUMN */
			}
			if (row_fault_counts) {
				TINFO("The number of memory fault pages: %d\n", row_fault_counts);
			}
			dimmForStatNum = 0;
		}

#else /* non-MFP21_TEST_USE_BIOS_DATA */
		/* Use constant column address to avoid duplication due to different columns */
		
		if ((offLinedRows < MAX_TOTAL_FAULT_ROW_NUM) && (offLinedPages < MAX_TOTAL_FAULT_PAGE_NUM)) {
			
			row_fault_index = offLinedPages;
			row_fault_counts = 0;
			
			pthread_mutex_lock(&mfpDataMutex);
			if (firstMFPStatSinceBoot) {
				for (i=0; i< (int)dimmCount; i++) {
					dimmShadowForStat[i].socket = dimmArray[i].loc.socket;
					dimmShadowForStat[i].imc = dimmArray[i].loc.imc;
					dimmShadowForStat[i].channel = dimmArray[i].loc.channel;
					dimmShadowForStat[i].dimm = dimmArray[i].loc.dimm;
				}
				dimmForStatLoopNum = (int)dimmCount;
				printf("First loop after boot, dimmForStatLoopNum is %d\n", dimmForStatLoopNum);
				/* Init fault count on each dimm as 0 */
				for (i=0; i< (int)dimmCount; i++) {
					faultsOnDimm[i].loc.socket = dimmArray[i].loc.socket;
					faultsOnDimm[i].loc.imc = dimmArray[i].loc.imc;
					faultsOnDimm[i].loc.channel = dimmArray[i].loc.channel;
					faultsOnDimm[i].loc.dimm = dimmArray[i].loc.dimm;
					faultsOnDimm[i].faultCount = 0;
				}
				firstMFPStatSinceBoot = 0;     // only run once
			}
			else {
				dimmForStatLoopNum =  dimmForStatNum;				
			}
			
			if (dimmForStatLoopNum) {
				TINFO("dimmForStatLoopNum is %d\n", dimmForStatLoopNum); 
			}
			
			for(i=0; i< dimmForStatLoopNum; i++) {
				printf("%d: socket=%d, imc=%d, channel=%d, dimm=%d goes through mfp_stat()\n", 
						__LINE__,dimmShadowForStat[i].socket,dimmShadowForStat[i].imc,dimmShadowForStat[i].channel,dimmShadowForStat[i].dimm);
				if ( isRowFaultCapReached(faultsOnDimm, (int)dimmCount, &dimmShadowForStat[i]) ) {
					continue;
				}
				mfp_stat(dimmShadowForStat[i], &mstat[i]);
				
	//#ifdef DEBUG
				printf("grain=%u, hard_error_grain=%u,row_fault_count=%u, col_fault_count=%u, bank_fault_count=%u\n", 
						mstat[i].grain, mstat[i].hard_error_grain, mstat[i].row_fault_count, mstat[i].col_fault_count, mstat[i].bank_fault_count );
				printf("err_storm=%u, err_daily_threshold=%u, err_count=%llu\n", mstat[i].err_storm, mstat[i].err_daily_threshold, mstat[i].err_count);
	//#endif
				if(mstat[i].row_fault_count == 0)
					continue ;
	
				/* row_fault_count != 0 */
				for(int j = 0; j < TOPN; j++) {
					if(mstat[i].topN_row_fault[j].valid) {
						fault_new.faultType = MEM_FAULT_ROW;
						fault_new.socket    = dimmShadowForStat[i].socket;
						fault_new.imc       = dimmShadowForStat[i].imc;
						fault_new.channel   = dimmShadowForStat[i].channel;
						fault_new.slot      = dimmShadowForStat[i].dimm;
	
						fault_new.rank      = mstat[i].topN_row_fault[j].rank;
						fault_new.device    = mstat[i].topN_row_fault[j].device;
						fault_new.bankgroup = mstat[i].topN_row_fault[j].bank_group;
						fault_new.bank      = mstat[i].topN_row_fault[j].bank;
						/*  max_row == min_row if row fault */
						fault_new.min_row   = mstat[i].topN_row_fault[j].min_row;
						fault_new.max_row   = mstat[i].topN_row_fault[j].max_row;
						
						if (offLinedRows<MAX_TOTAL_FAULT_ROW_NUM && isNewFaultRow(&fault_new, offLinedRowsRec, offLinedRows)) {
							memcpy(&offLinedRowsRec[offLinedRows++], &fault_new, sizeof(fault_new));
							updateRowFaultOnDimm(faultsOnDimm, (int)dimmCount, &fault_new);
						}
						else {
							TINFO("This Row: socket=%u, imc=%u, channel=%u, dimm=%u, rank=%u, device=%u, bankgroup=%u, bank=%u, "
									"row_min=%u, row_max=%u has been offlined. Or max total fault rows has readched, Ignored\n"
									,fault_new.socket, fault_new.imc, fault_new.channel, fault_new.slot, fault_new.rank, fault_new.device,
									fault_new.bankgroup, fault_new.bank, fault_new.min_row, fault_new.max_row);
							continue;
						}
						
						if (offLinedRows==MAX_TOTAL_FAULT_ROW_NUM) {
							TCRIT("The number of memory fault rows reach its limit: %d\n", offLinedRows);
							break;
						}
						/***** Start of ADDRESS_TRANSLATION *****/
						/*
						 * Reverse Address translation: DIMM to physical address
						 */
						TranslatedAddress.SocketId           = dimmShadowForStat[i].socket;
						TranslatedAddress.MemoryControllerId = dimmShadowForStat[i].imc;
						TranslatedAddress.ChannelId          = dimmShadowForStat[i].channel%2;   //AMI Channel to Intel Channel
						TranslatedAddress.DimmSlot           = dimmShadowForStat[i].dimm;
						TranslatedAddress.PhysicalRankId     = mstat[i].topN_row_fault[j].rank;
						TranslatedAddress.BankGroup          = mstat[i].topN_row_fault[j].bank_group;
						TranslatedAddress.Bank               = mstat[i].topN_row_fault[j].bank;
						TranslatedAddress.Row                = mstat[i].topN_row_fault[j].min_row;//max_row == min_row if row fault
	
						/*
						 * Followings are unnecessary
						 */
						//TranslatedAddress.DPA     = 0; /* DevicePhysicalAddress. All function calls set DPA to NULL */
						//TranslatedAddress.ChipId  = mstat[i].topN_row_fault[j].device; /* Is this correct? */
	
						dimmBdp.cpuType  = (uint8_t) type[dimmShadowForStat[i].socket];
						dimmBdp.bus      = bus[dimmShadowForStat[i].socket];
						dimmBdp.socket   = dimmShadowForStat[i].socket;
						dimmBdp.imc      = dimmShadowForStat[i].imc;
						dimmBdp.channel  = dimmShadowForStat[i].channel % 2;   //AMI Channel to Intel Channel
	
							/* Enumerate column */
						for(int col=0; col<1024; col+=8)
						{
							TranslatedAddress.Col = col;
	
							Status = DimmAddressToSystemAddress(&dimmBdp, &TranslatedAddress);
							if (Status) {
								TCRIT("[col 0x%x]: DimmAddressToSystemAddress() Error: 0x%llx\n", col, Status);
								TINFO("skt %d imc %d ch %d dimm %d rank %d, bg %d b %d r %d\n",
										TranslatedAddress.SocketId,
										TranslatedAddress.MemoryControllerId,
										TranslatedAddress.ChannelId,
										TranslatedAddress.DimmSlot,
										TranslatedAddress.PhysicalRankId,
										TranslatedAddress.BankGroup,
										TranslatedAddress.Bank,
										TranslatedAddress.Row); 
										offLinedRows--;	//rewind previously added row record in case address translation error
								break ;
							} else {
								TDBG("[col 0x%x]: DimmAddressToSystemAddress(): address 0x%llx\n", col, TranslatedAddress.SystemAddress);
							}
	
							/* 4K page masking */
							fault_new.phy_addr = ADDR_TRANS_MASK_4K & TranslatedAddress.SystemAddress;
							/***** End of ADDRESS_TRANSLATION *****/
	
							/* Process only new data */
							if (isNewPageAddress(offLinedPagesSysAddr, offLinedPages, fault_new.phy_addr) && (offLinedPages<MAX_TOTAL_FAULT_PAGE_NUM)) {
								TCRIT("New page address 0x%llx \n", fault_new.phy_addr);
								memcpy(&memFaultIPMI.fault_result[offLinedPages], &fault_new, sizeof(struct mfp_mem_fault_t));
								offLinedPagesSysAddr[offLinedPages] = fault_new.phy_addr;
								++offLinedPages;
								++row_fault_counts;
								if(MAX_TOTAL_FAULT_PAGE_NUM == offLinedPages){
									TCRIT("The number of memory fault reaches storage limit: %d\n", offLinedPages);
									break;
								}
							}
							else
							{
								TDBG("0x%llx is not new page address. Ignored\n", fault_new.phy_addr);	
							}
						} /* column enumeration */
					} /* if(mstat[i].topN_row_fault[j].valid) */
				} /* for(j = 0; j < TOPN; j++) */
			}
			if (row_fault_counts) {
				TINFO("The number of memory fault: %d\n", row_fault_counts);
			}
			dimmForStatNum = 0;
			pthread_mutex_unlock(&mfpDataMutex);
		}

#endif /* MFP21_TEST_USE_BIOS_DATA */
		if ((row_fault_counts == 0) && (dimmForStatNum == 0))
		{
			TDBG("Wait until MFP Fault Data is collected: %d DIMMs\n", dimmCount);
			sleep(DATA_MEMORY_FAULT_COLLECT_SLEEP);
		}
	}

	return NULL;
}
#endif /* CONFIG_SPX_FEATURE_MFP_2_1 */

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
    char * pEnd;

    
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
						strtol((reply->str), &pEnd, 16);
						if(*pEnd == '-'){
                        //dimm_arr[j].sn = (UINT32)strtol((reply->str), NULL, 16);
                        dimm_arr[j].sn = strtoul(pEnd+1, NULL,16);						
						}
						else{
						TCRIT("Serial Number string %s is not according to SMBIOS Type 17 format. Eg xxxx-xxxxxxxx \n", reply->str);
						}
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
				
#if defined (CONFIG_SPX_FEATURE_MFP_2_0) || defined (CONFIG_SPX_FEATURE_MFP_2_1)
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
#endif
				
				j++;
				
#if defined(DEBUG)
				TDBG("Found DIMM %i, socket=%u, imc=%u, channel=%u, dimm=%u, sn=0x%x \n", i, dimm_arr[i].loc.socket, dimm_arr[i].loc.imc,
    			dimm_arr[i].loc.channel,dimm_arr[i].loc.dimm,dimm_arr[i].sn);
#endif
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
				TWARN("BIOS Inventory Data is ready\n");
				break;
			}
			else {
				TWARN("reply->str = %s\n", reply->str);
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

#if defined (CONFIG_SPX_FEATURE_MFP_2_0) || defined (CONFIG_SPX_FEATURE_MFP_2_1)
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

	for (nrCPU=0; nrCPU<MAX_AMOUNT_OF_CPUS;nrCPU++) {
		if( 0 == peci_cmd_ping(0, MIN_CPU_ADDRESS + nrCPU) )  {
			TINFO("Ping address 0x%x Success\n", (MIN_CPU_ADDRESS + nrCPU));
		}
		else {
			break;
		}
	}
	if ( nrCPU == 0 ) {
		TCRIT("No CPU is detected by PECI PING\n");
		return NULL;
	}
	for (i=0; i<nrCPU;i++)  {
	    if ( 0 != ValidatePECIBus(i) || 0 !=  DetectCpuType(i, &type[i]) || 0 != RetrievePECIBus(i, &bus[i])) {
	    	return NULL;
	    }
#ifdef wakePECIAfterHostReset
		if ( 0 != WakePECI(i) ) {
			return NULL;
		}
#endif
	}

	TINFO("Pass PECI CPU Commands\n");
	
	while (1) {
		for (i=0; i<dimmCount; i++) {
			if (newErrNum < MAX_NEWERR-1) {
				pthread_mutex_lock(&mfpDataMutex);
				iCPU = (INT8U)dimmArray[i].loc.socket;
				iIMC = (INT8U)dimmArray[i].loc.imc;
				iChan = (INT8U)dimmArray[i].loc.channel;
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
			}
			else {
				TDBG("Wait for MFP Data finish processing\n");
				sleep(1);
			}	
		}

		sleep(WaitSecondsPeriodBetweenErrorPooling);
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
#if defined (CONFIG_SPX_FEATURE_MFP_2_1)
	pthread_t mfpMemFault;
#endif
	int readByte = 0;
	struct mfp_error err;
	mfpval_error	valerr;
	pthread_t mfpCompute;
#if defined (CONFIG_SPX_FEATURE_MFP_2_0) || defined (CONFIG_SPX_FEATURE_MFP_2_1)
	pthread_t mfp2ErrCollect;
#endif
	size_t i;
	struct timeval readTimeout;
	int retVal = 0;

	if(daemon_init() != 0) {
       TCRIT("Error Daemonizing !!!\n");
    }
    
    (void)umask( (mode_t)011 );
	
    /* save my pid */
	save_pid ("mfp");
	
    if (-1 == mkfifo (MFPQUEUE, 0777) && (errno != EEXIST))
    {
        TCRIT("Error creating named pipe %s\n", MFPQUEUE);
        return -1;
    }
   
    fdFifo = sigwrap_open (MFPQUEUE, O_RDWR);
    if (-1 == fdFifo)
    {
        TCRIT("Error opening named pipe %s\n", MFPQUEUE);
        return -1;
    }

    if (-1 == mkfifo (MFPVALQUEUE, 0777) && (errno != EEXIST))
    {
        TCRIT("Error creating named pipe %s\n", MFPVALQUEUE);
        return -1;
    }
    
    fdValFifo = sigwrap_open (MFPVALQUEUE, O_RDWR);
    if (-1 == fdValFifo)
    {
        TCRIT("Error opening named pipe %s\n", MFPVALQUEUE);
        return -1;
    }

#ifdef CONFIG_SPX_FEATURE_MFP_2_1
	
    if (-1 == mkfifo (MFPFAULTQUEUE, 0777) && (errno != EEXIST))
    {
        TDBG("Error creating named pipe %s\n", MFPFAULTQUEUE);
        return -1;
    }

    fdFaultFifo = sigwrap_open (MFPFAULTQUEUE, O_RDWR|O_CREAT);
    if (-1 == fdFaultFifo)
    {
        TDBG("Error opening named pipe %s\n", MFPFAULTQUEUE);
        return -1;
    }
#endif

    TDBG("MFP calls connect_signal(SIGUSR1)\n");
    connect_signal(SIGUSR1, mfp_sigusr1_handler, NULL);
	
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
		return -1;
	}
	TDBG("MFP: checkInventoryDataReady() is done\n");

	if ( -1 == getRedfishEnv() ) {
		TCRIT("MFP failed: getRedfishEnv()\n");
		return -1;
	}
	
	if ( -1 == getMemEntries() ) {
		TCRIT("MFP failed: getMemEntries()\n");
		return -1;
	}	

#endif
	
	if ( -1 == getDimm(&dimmCount, dimmArray, dimmID) ) {
		TCRIT("MFP failed: getDimm()\n");
		return -1;
	}
	TDBG("DIMM count %u\n", dimmCount);

	results = malloc(dimmCount * sizeof(struct mfp_evaluate_result));
    if( NULL == results)
    {
        TCRIT("Unable to Allocate Memory for MFP results\n");
        return -1;
    }

    for (i = 0; i< dimmCount; i++) {
        results[i].loc = dimmArray[i].loc;
    }
    
#if defined (CONFIG_SPX_FEATURE_MFP_2_0) || defined (CONFIG_SPX_FEATURE_MFP_2_1)
    /* This thread keeps fetching memory errors from CPU using PECI */
	if (0 != pthread_create(&mfp2ErrCollect, NULL, mfp2Thread, NULL)) {
		TCRIT("Unable create mfp Compute thread\n");
		return -1;
	}
#endif

//    initMFPReport(dimmID, results, dimmCount);
    
	if (0 != pthread_create(&mfpCompute, NULL, computeMFPThread, NULL)) {
		TCRIT("Unable create mfp Compute thread\n");
		return -1;
	}

#if defined (CONFIG_SPX_FEATURE_MFP_2_1)
	if (0 != pthread_create(&mfpMemFault, NULL, colMemFaultThread, NULL)) {
		TCRIT("Unable create mfp Memory Fault Monitoring thread\n");
		return -1;
	}
	TDBG("Created mfp Memory Fault Monitoring thread: colMemFaultThread\n");
#endif

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
				TDBG(" read %s time out %u seconds\n", MFPQUEUE, readTimeout.tv_sec);
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

	return 0;
}
