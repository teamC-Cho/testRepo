#include <string.h>
#include <dbgout.h>
#include "peciifc.h"
#include "cpu.h"
#include "spd_dimms_reader.h"

uint8_t getDimmNumber(uint8_t imc, uint8_t chan, uint8_t slot)
{
	return (imc * 4 + chan * 2 + slot);
}

uint8_t getPeciSmbCmdCfgFunction(uint8_t imc, uint8_t chan, uint8_t slot)
{
	uint8_t peciSmbCmdCfgFunction = 0;
	uint8_t dimmNumber = imc * 4 + chan * 2 + slot;
	if (dimmNumber < 8) {
		peciSmbCmdCfgFunction = 0;
	}
	else {
		peciSmbCmdCfgFunction = 1;
	}
	return peciSmbCmdCfgFunction;
}

int ReadDimmData(uint8_t cpu, uint8_t imc, uint8_t chan, uint8_t slot, uint32_t *serialNumber, char *partNumber, uint8_t partNumberLen)
{
	unsigned int i;
	uint16_t younger;
	uint16_t older;
	uint16_t data;
	SERIAL_NUMBER_T serialNumberBuilder = {0};
	
	if (serialNumber == NULL || partNumber == NULL || partNumberLen < 32) {
		TCRIT("One or more arguments value are incorrect\n");
		return -1;
	}
	
	for ( i=0; i<partNumberLen; i++ ) {
		partNumber[i] = 0;
	}
	
	if ( 0 != ReadWordFromSpdData(cpu, imc, chan, slot, SERIAL_NUMBER_FIRST_PAGE_OFFSET, &younger) )  {
		return -1;
	}
	serialNumberBuilder.words.YOUNGER_WORD = younger;
	if ( 0 != ReadWordFromSpdData(cpu, imc, chan, slot, SERIAL_NUMBER_FIRST_PAGE_OFFSET_OLDER_WORD, &older) ) {
		return -1;
	}
	serialNumberBuilder.words.OLDER_WORD = older;
	*serialNumber = serialNumberBuilder.data;
	TINFO("Serial Number is 0x%x\n", *serialNumber);
	
	for (i = 0; i < 10; i++) {
		if ( 0 != ReadWordFromSpdData(cpu, imc, chan, slot, PART_NUMBER_FIRST_PAGE_OFFSET + 2 * i, &data) ) {
	
			partNumber[i++] = (data & 0xFF00) >> 8;
			partNumber[i++] = data & 0x00FF;
		}
		else {
			return -1;
		}
	}
	
	TINFO("Part Number is %s\n", partNumber);

	return 0;
}


int ReadWordFromSpdData(uint8_t cpu, uint8_t imc, uint8_t chan, uint8_t slot, uint8_t offset, uint16_t *data)
{
	uint8_t peciSmbCmdCfgFunction = getPeciSmbCmdCfgFunction(imc, chan, slot);
	SMB_CMD_CFG_T originalCfg;
	if ( 0 != RetrieveSmbCmdCfg(cpu, peciSmbCmdCfgFunction, &originalCfg) ) {
    	return -1;
	}

	if ( 0 == ReadWordFromSpdData1(cpu, imc, chan, slot, offset, originalCfg, data) ) {
    	return 0;
	}
	else {
		TCRIT("Exception during SPD data gathering, trying to clear error bit and reenable TSOD readings");
		ClearSmbStatusCfgErrorBit(cpu, imc, chan, slot);
		EnDisableTsodReadings(cpu, imc, chan, slot, originalCfg, 1);
		return -1;
	}
}


int ReadWordFromSpdData1(uint8_t cpu, uint8_t imc, uint8_t chan, uint8_t slot, uint8_t offset, SMB_CMD_CFG_T originalCfg, uint16_t *data)
{
	if (originalCfg.bits.SMB_TSOD_POOL_EN == 1) {
		if ( 0 != EnDisableTsodReadings(cpu, imc, chan, slot, originalCfg, 0) ) {
			return -1;
		}
	}
	else {
		TWARN("TSOD readings initially disabled");
	}
	
	if ( 0 != ReadSpdDataWithTsodDisabled(cpu, imc, chan, slot, originalCfg, offset, data) ) {
		return -1;
	}
	
	if ( 0 != EnDisableTsodReadings(cpu, imc, chan, slot, originalCfg, 0) ) {
		return -1;
	}
	
	return 0;
}


int ReadSpdDataWithTsodDisabled(uint8_t cpu, uint8_t imc, uint8_t chan, uint8_t slot, SMB_CMD_CFG_T originalCfg, uint8_t offset, uint16_t *data)
{
	if ( 0 != SetReadingsToPage(cpu, imc, chan, slot, originalCfg, SMB_SA_CHANE_PAGE_TO_1) ) {
		return -1;
	}
	
	if ( 0 != ReadSpdDataFromProperPage(cpu, imc, chan, slot, originalCfg, offset, data) ) {
		return -1;
	}
	
	if ( 0 != SetReadingsToPage(cpu, imc, chan, slot, originalCfg, SMB_SA_CHANE_PAGE_TO_0) ) {
		return -1;
	}
	
	return 0;
}


int ReadSpdDataFromProperPage(uint8_t cpu, uint8_t imc, uint8_t chan, uint8_t slot, SMB_CMD_CFG_T originalCfg, uint8_t offset, uint16_t *smbRdData)
{
	uint8_t peciSmbCmdCfgFunction = getPeciSmbCmdCfgFunction(imc, chan, slot);
	SMB_STATUS_CFG_T receivedStatus = {0};
	SMB_DATA_CFG_T receivedData = {0};
	unsigned int waitTime;
	originalCfg.bits.SMB_TSOD_POOL_EN = 0;
	originalCfg.bits.SMB_WORD_ACCESS = 1;
	originalCfg.bits.SMB_WRT = 0;
	
	originalCfg.bits.SMB_SA = getDimmNumber(imc, chan, slot);
	originalCfg.bits.SMB_BA = offset;
	originalCfg.bits.SMB_DTI = SMB_DTI_READING_EEPROM_SETTING;
	
	originalCfg.bits.SMB_CMD_TRIGGER = 1;
	
	if ( 0 != SetSmbCmdCfg(cpu, peciSmbCmdCfgFunction, originalCfg) ) {
		return -1;
	}
	
	for (waitTime = 1; waitTime <= 256; waitTime = waitTime * 2) {
		RetrieveSmbStatusCfg(cpu, peciSmbCmdCfgFunction, &receivedStatus);
	
		if (receivedStatus.bits.LAST_DTI == SMB_DTI_READING_EEPROM_SETTING && receivedStatus.bits.SMB_RDO == 1) {
			if ( 0 == RetrieveSmbDataCfg(cpu, peciSmbCmdCfgFunction, &receivedData) ) {
				*smbRdData = receivedData.bits.SMB_RDATA;
				return 0;
			}
			else {
				return -1;
			}
		}
		else {
			sleep(waitTime);
		}
	}	
	return -1;
}


int SetReadingsToPage(uint8_t cpu, uint8_t imc, uint8_t chan, uint8_t slot, SMB_CMD_CFG_T originalCfg, uint8_t pageAddress)
{
	uint8_t peciSmbCmdCfgFunction = getPeciSmbCmdCfgFunction(imc, chan, slot);
	SMB_STATUS_CFG_T receivedStatus = {0};
	unsigned int waitTime;
	originalCfg.bits.SMB_TSOD_POOL_EN = 0;
	originalCfg.bits.SMB_WORD_ACCESS = 0;
	originalCfg.bits.SMB_WRT = 1;
	
	originalCfg.bits.SMB_SA = pageAddress;
	originalCfg.bits.SMB_DTI = SMB_DTI_PAGE_CHANGE_SETTING;
	
	originalCfg.bits.SMB_CMD_TRIGGER = 1;
	
	if ( 0 != SetSmbCmdCfg(cpu, peciSmbCmdCfgFunction, originalCfg) ) {
		return -1;
	}
	
	for (waitTime = 1; waitTime <= 256; waitTime = waitTime * 2) {
		RetrieveSmbStatusCfg(cpu, peciSmbCmdCfgFunction, &receivedStatus);
	
		if (receivedStatus.bits.LAST_DTI == SMB_DTI_PAGE_CHANGE_SETTING && receivedStatus.bits.SMB_WOD == 1) {
			return 0;
		}
		else {
			sleep(waitTime);
		}
	}	
	return -1;
}


int EnDisableTsodReadings(uint8_t cpu, uint8_t imc, uint8_t chan, uint8_t slot, SMB_CMD_CFG_T originalCfg, uint8_t enDisable)
{
	uint8_t peciSmbCmdCfgFunction = getPeciSmbCmdCfgFunction(imc, chan, slot);
	SMB_CMD_CFG_T receivedCfg = {0};
	unsigned int waitTime;
	originalCfg.bits.SMB_TSOD_POOL_EN = enDisable & 0x1;
	if ( 0 != SetSmbCmdCfg(cpu, peciSmbCmdCfgFunction, originalCfg) ) {
		return -1;
	}
	
	for (waitTime = 1; waitTime <= 256; waitTime = waitTime * 2) {
		RetrieveSmbCmdCfg(cpu, peciSmbCmdCfgFunction, &receivedCfg);
	
		if (receivedCfg.bits.SMB_TSOD_POOL_EN == (enDisable & 0x1) ) {
			return 0;
		}
		else {
			sleep(waitTime);
		}
	}
	return -1;
}


int SetSmbCmdCfg(uint8_t cpu, uint8_t peciSmbCmdCfgFunction, SMB_CMD_CFG_T smbCmfCfg)
{
	int8_t 	dev_id = 0;
	uint8_t target = getAddress(cpu);
	int8_t 	domain = 0;
	int8_t	xmit_feedback = 0;
	int8_t	awfcs = 1;
	int32_t readLen = 1;
	
	peci_wrendpointconfig_pci_req_t wrEndPntPciReq = {0};
	peci_wrendpointconfig_pci_res_t wrEndPntPciRes = {0};
	
	wrEndPntPciReq.option = DWORD;
	wrEndPntPciReq.host_id = 0;
	wrEndPntPciReq.message_type = 0x3;
	wrEndPntPciReq.segment = 0;
	wrEndPntPciReq.pci_config_addr = PECI_SMB_CMD_CFG_BUS<<20 | PECI_SMB_CMD_CFG_DEVICE<<15 | peciSmbCmdCfgFunction<<12 | SMB_CMD_CFG_REG_OFFSET;
	wrEndPntPciReq.data.peci_dword = smbCmfCfg.data;
	
	if ( 0 != peci_cmd_wrendpointconfig_pci(dev_id, target, domain, xmit_feedback, awfcs, &readLen, (unsigned char*)&wrEndPntPciReq, (unsigned char *)&wrEndPntPciRes)) {
		TCRIT("peci_cmd_wrendpointconfig_pci failed, CC=0x%x\n", wrEndPntPciRes.completion_code);
		return -1;				
	}
	if ( wrEndPntPciRes.completion_code != 0x40 ) {
		TCRIT("CC = 0x%x\n", wrEndPntPciRes.completion_code);
		return -1;
	}
	return 0;
}


int RetrieveSmbCmdCfg(uint8_t cpu, uint8_t peciSmbCmdCfgFunction, SMB_CMD_CFG_T *smbCmfCfg)
{
	int8_t 	dev_id = 0;
	uint8_t target = getAddress(cpu);
	int8_t 	domain = 0;
	int8_t	xmit_feedback = 0;
	int8_t	awfcs = 1;
	int32_t readLen = 1;
	
	peci_rdendpointconfig_pci_req_t	rdEndPntPciReq = {0};
	peci_rdendpointconfig_pci_res_t	reEndPntPciRes = {0};
	
	rdEndPntPciReq.option = DWORD;
	rdEndPntPciReq.host_id = 0;
	rdEndPntPciReq.message_type = 0x3;
	rdEndPntPciReq.segment = 0;
	rdEndPntPciReq.pci_config_addr = PECI_SMB_CMD_CFG_BUS<<20 | PECI_SMB_CMD_CFG_DEVICE<<15 | peciSmbCmdCfgFunction<<12 | SMB_CMD_CFG_REG_OFFSET;
	
	if ( 0 != peci_cmd_rdendpointconfig_pci(dev_id, target, domain, xmit_feedback, awfcs, &readLen, (unsigned char*)&rdEndPntPciReq, (unsigned char *)&reEndPntPciRes) ) {
		TCRIT("peci_cmd_rdendpointconfig_pci failed, CC=0x%x\n", reEndPntPciRes.completion_code);
		return -1;		
	}
	if ( reEndPntPciRes.completion_code != 0x40 ) {
		TCRIT("CC = 0x%x\n", reEndPntPciRes.completion_code);
		return -1;
	}
	memcpy(smbCmfCfg, &reEndPntPciRes.data, DWORD);
	return 0;
}


int RetrieveSmbDataCfg(uint8_t cpu, uint8_t peciSmbCmdCfgFunction, SMB_DATA_CFG_T *smbDataCfg)
{
	int8_t 	dev_id = 0;
	uint8_t target = getAddress(cpu);
	int8_t 	domain = 0;
	int8_t	xmit_feedback = 0;
	int8_t	awfcs = 1;
	int32_t readLen = 1;
	
	peci_rdendpointconfig_pci_req_t	rdEndPntPciReq = {0};
	peci_rdendpointconfig_pci_res_t	reEndPntPciRes = {0};
	
	rdEndPntPciReq.option = DWORD;
	rdEndPntPciReq.host_id = 0;
	rdEndPntPciReq.message_type = 0x3;
	rdEndPntPciReq.segment = 0;
	rdEndPntPciReq.pci_config_addr = PECI_SMB_CMD_CFG_BUS<<20 | PECI_SMB_CMD_CFG_DEVICE<<15 | peciSmbCmdCfgFunction<<12 | SMB_DATA_CFG_REG_OFFSET;
	
	if ( 0 != peci_cmd_rdendpointconfig_pci(dev_id, target, domain, xmit_feedback, awfcs, &readLen, (unsigned char*)&rdEndPntPciReq, (unsigned char *)&reEndPntPciRes) ) {
		TCRIT("peci_cmd_rdendpointconfig_pci failed, CC=0x%x\n", reEndPntPciRes.completion_code);
		return -1;		
	}
	if ( reEndPntPciRes.completion_code != 0x40 ) {
		TCRIT("CC = 0x%x\n", reEndPntPciRes.completion_code);
		return -1;
	}
	memcpy(smbDataCfg, &reEndPntPciRes.data, DWORD);
	return 0;
}


int RetrieveSmbStatusCfg(uint8_t cpu, uint8_t peciSmbCmdCfgFunction, SMB_STATUS_CFG_T *smbStatusCfg)
{	
	int8_t 	dev_id = 0;
	uint8_t target = getAddress(cpu);
	int8_t 	domain = 0;
	int8_t	xmit_feedback = 0;
	int8_t	awfcs = 1;
	int32_t readLen = 1;
	
	peci_rdendpointconfig_pci_req_t	rdEndPntPciReq = {0};
	peci_rdendpointconfig_pci_res_t	reEndPntPciRes = {0};
	
	rdEndPntPciReq.option = DWORD;
	rdEndPntPciReq.host_id = 0;
	rdEndPntPciReq.message_type = 0x3;
	rdEndPntPciReq.segment = 0;
	rdEndPntPciReq.pci_config_addr = PECI_SMB_CMD_CFG_BUS<<20 | PECI_SMB_CMD_CFG_DEVICE<<15 | peciSmbCmdCfgFunction<<12 | SMB_STATUS_CFG_REG_OFFSET;
	
	if ( 0 != peci_cmd_rdendpointconfig_pci(dev_id, target, domain, xmit_feedback, awfcs, &readLen, (unsigned char*)&rdEndPntPciReq, (unsigned char *)&reEndPntPciRes) ) {
		TCRIT("peci_cmd_rdendpointconfig_pci failed, CC=0x%x\n", reEndPntPciRes.completion_code);
		return -1;		
	}
	if ( reEndPntPciRes.completion_code != 0x40 ) {
		TCRIT("CC = 0x%x\n", reEndPntPciRes.completion_code);
		return -1;
	}
	memcpy(smbStatusCfg, &reEndPntPciRes.data, DWORD);
	return 0;	
}


int SetSmbStatusCfg(uint8_t cpu, uint8_t peciSmbCmdCfgFunction, SMB_STATUS_CFG_T smbStatusCfg)
{
	int8_t 	dev_id = 0;
	uint8_t target = getAddress(cpu);
	int8_t 	domain = 0;
	int8_t	xmit_feedback = 0;
	int8_t	awfcs = 1;
	int32_t readLen = 1;
	
	peci_wrendpointconfig_pci_req_t wrEndPntPciReq = {0};
	peci_wrendpointconfig_pci_res_t wrEndPntPciRes = {0};
	
	wrEndPntPciReq.option = DWORD;
	wrEndPntPciReq.host_id = 0;
	wrEndPntPciReq.message_type = 0x3;
	wrEndPntPciReq.segment = 0;
	wrEndPntPciReq.pci_config_addr = PECI_SMB_CMD_CFG_BUS<<20 | PECI_SMB_CMD_CFG_DEVICE<<15 | peciSmbCmdCfgFunction<<12 | SMB_STATUS_CFG_REG_OFFSET;
	wrEndPntPciReq.data.peci_dword = smbStatusCfg.data;
	
	if ( 0 != peci_cmd_wrendpointconfig_pci(dev_id, target, domain, xmit_feedback, awfcs, &readLen, (unsigned char*)&wrEndPntPciReq, (unsigned char *)&wrEndPntPciRes)) {
		TCRIT("peci_cmd_wrendpointconfig_pci failed, CC=0x%x\n", wrEndPntPciRes.completion_code);
		return -1;				
	}
	if ( wrEndPntPciRes.completion_code != 0x40 ) {
		TCRIT("CC = 0x%x\n", wrEndPntPciRes.completion_code);
		return -1;
	}
	return 0;
}


int ClearSmbStatusCfgErrorBit(uint8_t cpu, uint8_t imc, uint8_t chan, uint8_t slot)
{
	uint8_t peciSmbCmdCfgFunction = getPeciSmbCmdCfgFunction(imc, chan, slot);
	SMB_STATUS_CFG_T smbStatusCfg = {0};
	
	if ( 0 != RetrieveSmbStatusCfg(cpu, peciSmbCmdCfgFunction, &smbStatusCfg) ) {
		return -1;
	}
	if (smbStatusCfg.bits.SMB_SBE == 1) {
		smbStatusCfg.bits.SMB_SBE = 0;
		if ( 0 != SetSmbStatusCfg(cpu, peciSmbCmdCfgFunction, smbStatusCfg) ) {
			return -1;
		}
	}
	return 0;
}
