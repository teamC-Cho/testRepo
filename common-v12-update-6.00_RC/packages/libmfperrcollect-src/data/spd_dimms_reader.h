
#ifndef __SPD_DIMMS_READER_H__
#define __SPD_DIMMS_READER_H__
#include <stdint.h>

uint8_t PECI_SMB_CMD_CFG_BUS = 13; // Magic mapping from 30, tip form pcode team
uint8_t PECI_SMB_CMD_CFG_DEVICE = 11;
uint16_t SMB_CMD_CFG_REG_OFFSET = 0x80;
uint16_t SMB_STATUS_CFG_REG_OFFSET = 0x84;
uint16_t SMB_DATA_CFG_REG_OFFSET = 0x88;

uint8_t SMB_SA_CHANE_PAGE_TO_0 = 0b110;
uint8_t SMB_SA_CHANE_PAGE_TO_1 = 0b111;
uint8_t SMB_DTI_PAGE_CHANGE_SETTING = 0b0110;
uint8_t SMB_DTI_READING_EEPROM_SETTING = 0b1010;

uint8_t SERIAL_NUMBER_FIRST_PAGE_OFFSET = 69;
uint8_t SERIAL_NUMBER_FIRST_PAGE_OFFSET_OLDER_WORD = 71;
uint8_t PART_NUMBER_FIRST_PAGE_OFFSET = 73;

typedef union SMB_CMD_CFG_T
{
    uint32_t data;
    struct Bits
    {
        uint32_t SMB_BA : 8, SMB_SA : 3, SMB_DTI : 4, SMB_WRT : 2, SMB_WORD_ACCESS : 1, SMB_PNTR_SEL : 1, SMB_CMD_TRIGGER : 1,
            SMB_TSOD_POOL_EN : 1, Reserved2 : 3, SMB_SOFT_RST : 1, SMB_SBE_ERR0_EN : 1, SMB_SBE_SMI_EN : 1, SMB_SBE_EN : 1,
            SMB_SVE_EN : 1, SMB_DIS_WRT : 1, SMB_CKOVRD : 1, Reserved : 2;

    } bits;
} SMB_CMD_CFG_T;

typedef union SMB_STATUS_CFG_T
{
    uint32_t data;
    struct
    {
        uint32_t SMB_BUSY : 1, SMB_SBE : 1, SMB_RDO : 1, SMB_WOD : 1, Reserved4 : 4, TSOD_SA : 3, LAST_DTI : 4, Reserved3 : 1,
            LAST_BRANCH_CFG : 3, Reserved2 : 1, TSOD_POOL_EN : 1, Reserved : 11;
    } bits;
} SMB_STATUS_CFG_T;

typedef union SMB_DATA_CFG_T
{
    uint32_t data;
    struct
    {
        uint32_t SMB_RDATA : 16, SMB_WDATA : 16;
    } bits;
} SMB_DATA_CFG_T;

typedef union SERIAL_NUMBER_T
{
    uint32_t data;
    struct
    {
        uint32_t OLDER_WORD : 16, YOUNGER_WORD : 16;
    } words;
} SERIAL_NUMBER_T;

uint8_t getDimmNumber(uint8_t imc, uint8_t chan, uint8_t slot);
uint8_t getPeciSmbCmdCfgFunction(uint8_t imc, uint8_t chan, uint8_t slot);
int ReadDimmData(uint8_t cpu, uint8_t imc, uint8_t chan, uint8_t slot, uint32_t *serialNumber, char *partNumber, uint8_t partNumberLen);
//uint32_t GetSerialNumber();
//int GetPartNumber(char *pn);

int ReadWordFromSpdData(uint8_t cpu, uint8_t imc, uint8_t chan, uint8_t slot, uint8_t offset, uint16_t *data);
int ReadWordFromSpdData1(uint8_t cpu, uint8_t imc, uint8_t chan, uint8_t slot, uint8_t offset, SMB_CMD_CFG_T originalCfg, uint16_t *data);
int ReadSpdDataWithTsodDisabled(uint8_t cpu, uint8_t imc, uint8_t chan, uint8_t slot, SMB_CMD_CFG_T originalCfg, uint8_t offset, uint16_t *data);
int ReadSpdDataFromProperPage(uint8_t cpu, uint8_t imc, uint8_t chan, uint8_t slot, SMB_CMD_CFG_T originalCfg, uint8_t offset, uint16_t *smbRdData);
int SetReadingsToPage(uint8_t cpu, uint8_t imc, uint8_t chan, uint8_t slot, SMB_CMD_CFG_T originalCfg, uint8_t pageAddress);
int EnDisableTsodReadings(uint8_t cpu, uint8_t imc, uint8_t chan, uint8_t slot, SMB_CMD_CFG_T originalCfg, uint8_t enDisable);
int SetSmbCmdCfg(uint8_t cpu, uint8_t peciSmbCmdCfgFunction, SMB_CMD_CFG_T smbCmfCfg);
int RetrieveSmbCmdCfg(uint8_t cpu, uint8_t peciSmbCmdCfgFunction, SMB_CMD_CFG_T *smbCmfCfg);
int RetrieveSmbDataCfg(uint8_t cpu, uint8_t peciSmbCmdCfgFunction, SMB_DATA_CFG_T *smbDataCfg);
int RetrieveSmbStatusCfg(uint8_t cpu, uint8_t peciSmbCmdCfgFunction, SMB_STATUS_CFG_T *smbStatusCfg);
int SetSmbStatusCfg(uint8_t cpu, uint8_t peciSmbCmdCfgFunction, SMB_STATUS_CFG_T smbStatusCfg);
int ClearSmbStatusCfgErrorBit(uint8_t cpu, uint8_t imc, uint8_t chan, uint8_t slot);

#endif
