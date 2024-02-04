
#ifndef __CHANNEL_H__
#define __CHANNEL_H__

#define	mmio_bdf_and_bar_32bit_address_type		5
#define	mmio_bdf_and_bar_64bit_address_type		6
#define	device_of_first_imc						26

#if defined (CONFIG_SPX_FEATURE_MFP_2_1)
typedef union CORR_ERROR_STATUS_T
{
    uint32_t data;
    struct //Bits
    {
        uint32_t ERR_OVERFLOW_STAT : 8, DIMM_ALERT : 3, DIMM_ALERT_SELECT : 3, DDR4CRC_RANK_LOG : 8, Reserved : 10;
    } bits;
} CORR_ERROR_STATUS_T;

typedef union CORR_ERR_CNT_T
{
    uint32_t data;
    struct //Bits
    {
        uint32_t COR_ERR_CNT_0 : 15, OVERFLOW_0 : 1, COR_ERR_CNT_1 : 15, OVERFLOW_1 : 1;
    } bits;
} CORR_ERR_CNT_T;
#endif // CONFIG_SPX_FEATURE_MFP_2_1

void LookForErrors(uint8_t bus, CpuTypes type, uint8_t cpu, uint8_t imc, uint8_t chan, MemErrorStruct *pMemErr, bool *validErr);
//void AddDimm(uint8_t slot, uint32_t sn, char *pn);

int GatherRetryRegistersData(uint8_t cpu, uint8_t bus, CpuTypes type, uint8_t devFunc, uint8_t chan, RetryRdErr *rdData);
int GatherBothRetryLogRegisters(uint8_t cpu, uint8_t bus, uint8_t devFunc, uint8_t chan, RetryRdErr *rdData);
int GatherOneRetryLogRegister(uint8_t cpu, uint8_t bus, uint8_t devFunc, uint8_t chan, uint8_t setIndex, RetryRdErr *rdData);
int GatherSpecifiedSetRetryRegistersData(uint8_t cpu, uint8_t bus, uint8_t devFunc, uint8_t chan, uint8_t setIndex, RetryRdErr *rdData);
void ChooseRetryRegisterSetNumber(RetryRdErr *rrdeSets);
void ParseRetryRegistersData(uint8_t cpu, CpuTypes type, uint8_t imc, uint8_t chan, MemErrorStruct *pMemErr, RetryRdErr *rrdeSets);
int ClearRetryRegister(uint8_t cpu, uint8_t bus, uint8_t devFunc, uint8_t chan, RetryRdErr *rrdeSets);
int GetMMIORegisterData(uint8_t cpu, uint8_t bus, uint8_t devFunc, uint64_t registerAddress, uint32_t *resData);
int SetMMIORegisterData(uint8_t cpu, uint8_t bus, uint8_t devFunc, uint64_t registerAddress, uint32_t data);
int GetMMIORegister64bitData(uint8_t cpu, uint8_t bus, uint8_t devFunc, uint64_t registerAddress, uint64_t *resData);

#endif
