
#ifndef __MFP_STRUCTS_H__
#define __MFP_STRUCTS_H__

#include "mfp_registers_structs.h"

#define MIN_CPU_ADDRESS		48
#define DWORD_SIZE			4
#define MAX_AMOUNT_OF_CPUS	8
#define MAX_AMOUNT_OF_IMCS	4
#define MAX_AMOUNT_OF_DIMM_CHANNELS		2
#define MAX_AMOUNT_OF_DIMM_SLOTS		2

typedef struct Dimm
{
    uint8_t slot;
    uint32_t serialNumber;
    char partNumber[32];
} Dimm;

typedef enum CpuTypes
{
    CPU_TYPE_UNSUPPORTED,
    CPU_TYPE_SKX,
    CPU_TYPE_CLX,
    CPU_TYPE_CPX,
    CPU_TYPE_ICX,
    CPU_TYPE_ICX2
} CpuTypes;

typedef struct RetryRdErr
{
    RETRY_RD_ERR_LOG_MCDDC_DP_STRUCT Log;
    RETRY_RD_ERR_LOG_ADDRESS1_MCDDC_DP_STRUCT LogAddr1;
    RETRY_RD_ERR_LOG_ADDRESS2_MCDDC_DP_STRUCT LogAddr2;
    RETRY_RD_ERR_LOG_PARITY_STRUCT Parity;
    uint64_t	LogAddr3;
} RetryRdErr;

typedef struct MemErrorStruct
{
    uint8_t socket;
    uint8_t imc;
    uint8_t channel;
    uint8_t slot;
    uint8_t rank;
    uint8_t device;
    uint8_t bankGroup;
    uint8_t bank;
    uint32_t row;
    uint32_t col;
    uint8_t errorType;
    uint32_t paritySyndrome;
    uint8_t mode;
} MemErrorStruct;

typedef struct CpuInfo
{
    uint32_t cpuModelData;
    uint32_t stepping;
} CpuInfo;


#endif
