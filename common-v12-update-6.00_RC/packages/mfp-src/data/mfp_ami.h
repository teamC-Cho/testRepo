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
 * mfp_ami.h
 * memory failure prediction main function
 *
 * Author: Changjun Kang <changjunk@ami.com>
 ******************************************************************/

#ifndef MFP_AMI_H
#define MFP_AMI_H

#define SOCKET_MASK		0x7
#define IMC_MASK		0x3
#define	IMC_BASE_CHANNEL_MASK		0x1
#define SOCKET_BASE_CHANNEL_MASK	0x7
#define DIMM_MASK		0x3
#define RANK_MASK		0xF
#define DEVICE_MASK		0x3F
#define BG_MASK			0x7
#define BANK_MASK		0x3
#define ROW_MASK		0xFFFFF
#define COLUMN_MASK		0x3FF
#define UE_MASK			0x1
#define	MODE_MASK		0xF

#define	SEL_SOCKET_SHFT	0x5
#define	SEL_IMC_SHFT	0x3
#define	SEL_CHAN_SHFT	0x1
#define	SEL_DIMM_SHFT	0x6
#define	SEL_RANK_SHFT	0x2
#define	SEL_BG_SHFT		0x5
#define	SEL_BANK_SHFT	0x3
#define SEL_ERRT_SHFT	0x2
#define SEL_COL_RSHFT	0x2

/* ************************************************************************************
 * Max record size for transfer between BMC and BIOS, limited by 255 bytes of kcs buffer 
 * 31*sizeof(INT64U)+2 = 250. 
 * In addition, as of linux 5.17, the fifo size of memory failure in mm/memory-failure.c
 * is 16. So we set MAX_FAULT_ERR at 16. If BMC sends more than 16 records at a time, 
 * Host memory failure fifo overflows, truncating pages over 16.
 * ************************************************************************************/
#define MAX_FAULT_ERR   16
#define MAX_DIMM_COUNT	256 /* Max record size that BMC can store */
/* Max fault numbers on a system 
 * Assuming 24 pages associated with a fault row, 24*256=6k pages */
#define MAX_TOTAL_FAULT_PAGE_NUM	(6*1024)
/* Assuming 24-48 pages associated with a row, 256 is big enough */
#define MAX_TOTAL_FAULT_ROW_NUM		256
/* ******************************************************
 * row fault cap per dimm 30 is suggested by Intel  
 * prevent predominant faults from coming from one dimm
 * *******************************************************/
#define ROW_FAULT_CAP_PER_DIMM	30


#define MFPQUEUE 		"/var/MFPQUEUE"
#define MFPVALQUEUE 	"/var/MFPVALQUEUE"
#ifdef CONFIG_SPX_FEATURE_MFP_2_1
#define MFPFAULTQUEUE 	"/var/MFPFAULTQUEUE"
#define MFP_FAULT_KEY	"/var/tmp/MFP_FAULT_KEY"
#endif
#define MFP_SNAPSHOT	"/conf/mfp_snapshot"
#define VARTMP_DIR		"/var/tmp"
#define MFP_VAL_KEY		"/var/tmp/MFP_VAL_KEY"
#define MFP_REPORT		"/conf/mfp_report"

#define TMP_DIR         "/var/tmp"
#define ADDR_TRANS_INFO "/var/tmp/addr_trans_params"

typedef struct {
	uint32_t timestamp;
	struct mfp_error	valerr;
} mfpval_error;

typedef struct {
	uint16_t dimmID;
	struct mfp_evaluate_result result;
} ami_mfp_evaluate_result;

struct mfp_mem_fault_t {
	INT8U	faultType; //row:0, col:1 or bank:2
	INT8U	socket;
	INT8U	imc;
	INT8U	channel;
	INT8U 	slot;
	INT8U	rank;
	INT8U	device;
	INT8U	bankgroup;
	INT8U	bank;
	INT32U	min_row;
	INT32U	max_row;
	INT32U	min_col;
 	INT32U	max_col;
 	unsigned long long	phy_addr; //at min_row
} PACKED;

struct addrTrParam_T{
	unsigned long long  mMmCfgBase;
	unsigned long long  mMmCfgLimit;
	unsigned long long  mPlatGlobalMmiolBase;
	unsigned long long  mPlatGlobalMmiolLimit;
	unsigned long long  mPlatGlobalMmiohBase;
	unsigned long long  mPlatGlobalMmiohLimit;
	UINT32  mSocketPresentBitMap;
	UINT8   mTotCha;
} PACKED;

#if defined (CONFIG_SPX_FEATURE_MFP_2_1)
struct mfp_faults_ipmi {
	INT32U counts;
	struct mfp_mem_fault_t fault_result[MAX_TOTAL_FAULT_PAGE_NUM];
} PACKED;

enum memFaults {MEM_FAULT_ROW, MEM_FAULT_COLUMN, MEM_FAULT_BANK};

typedef struct {
    struct mfp_dimm loc;
    uint32_t faultCount;
} dimmFaultCount;

int getDimmsForStat(struct mfp_dimm *pDimm, int lastDimmForStatNum, struct mfp_error *pError, int errorNumber);

#endif /* CONFIG_SPX_FEATURE_MFP_2_1 */
#endif
