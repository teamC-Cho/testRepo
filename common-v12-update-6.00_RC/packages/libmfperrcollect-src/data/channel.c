#include <string.h>
#include <dbgout.h>
#include "peciifc.h"
#include "mfp_structs.h"
#include "cpu.h"
#include "channel.h"

bool nooverInSet[NUMBER_OF_MMIO_REGISTERS_SETS] = {false};
bool registerContainValidErrorData[NUMBER_OF_MMIO_REGISTERS_SETS] = {false};

uint64_t	ucErrorSysAddr = 0;
MemErrorStruct	ucMemErr;
uint32_t	readUcErrorTimes = 0;		

#if defined (CONFIG_SPX_FEATURE_MFP_2_1)
uint8_t PECI_CORR_ERROR_STATUS_FUNCTION = 0;
uint64_t PECI_CORR_ERROR_STATUS_CHN0_OFFSET = 0x22c50;
uint64_t PECI_CORR_ERROR_STATUS_CHN1_OFFSET = 0x26c50;

uint8_t PECI_CORR_ERR_CNT_FUNCTION = 0;
uint64_t PECI_CORR_ERR_CNT_0_CHN0_OFFSET = 0x22c18;
uint64_t PECI_CORR_ERR_CNT_1_CHN0_OFFSET = 0x22c1c;
uint64_t PECI_CORR_ERR_CNT_2_CHN0_OFFSET = 0x22c20;
uint64_t PECI_CORR_ERR_CNT_3_CHN0_OFFSET = 0x22c24;
uint64_t PECI_CORR_ERR_CNT_0_CHN1_OFFSET = 0x26c18;
uint64_t PECI_CORR_ERR_CNT_1_CHN1_OFFSET = 0x26c1c;
uint64_t PECI_CORR_ERR_CNT_2_CHN1_OFFSET = 0x26c20;
uint64_t PECI_CORR_ERR_CNT_3_CHN1_OFFSET = 0x26c24;
#endif

MMIO_REGISTERS_ADDRESSES MMIO_REGISTERS_ADDRESSES_PACKAGE[NUMBER_OF_CHANNELS][NUMBER_OF_MMIO_REGISTERS_SETS] = {
        // Channel 0
         {// SET1
          {RETRY_RD_ERR_LOG_CH_0_REG, RETRY_RD_ERR_LOG_ADDRESS1_CH_0_REG,
                                   RETRY_RD_ERR_LOG_ADDRESS2_CH_0_REG, RETRY_RD_ERR_LOG_PARITY_CH_0_REG, RETRY_RD_ERR_LOG_ADDRESS3_CH0_REG},
          // SET2
          {RETRY_RD_ERR_SET2_LOG_CH_0_REG, RETRY_RD_ERR_SET2_LOG_ADDRESS1_CH_DP_REG,
                                   RETRY_RD_ERR_SET2_LOG_ADDRESS2_CH_0_REG, RETRY_RD_ERR_SET2_LOG_PARITY_CH_0_REG, RETRY_RD_ERR_SET2_LOG_ADDRESS3_CH_0_REG}
         },
         // CHannel 1
         {// SET1
          {RETRY_RD_ERR_LOG_CH_1_REG, RETRY_RD_ERR_LOG_ADDRESS1_CH_1_REG,
                                   RETRY_RD_ERR_LOG_ADDRESS2_CH_1_REG, RETRY_RD_ERR_LOG_PARITY_CH_1_REG, RETRY_RD_ERR_LOG_ADDRESS3_CH1_REG},
          // SET2
          {RETRY_RD_ERR_SET2_LOG_CH_1_REG, RETRY_RD_ERR_SET2_LOG_ADDRESS1_CH_1_REG,
                                   RETRY_RD_ERR_SET2_LOG_ADDRESS2_CH_1_REG, RETRY_RD_ERR_SET2_LOG_PARITY_CH_1_REG, RETRY_RD_ERR_SET2_LOG_ADDRESS3_CH_1_REG}
         }
};

void LookForErrors(uint8_t bus, CpuTypes type, uint8_t cpu, uint8_t imc, uint8_t chan, MemErrorStruct *pMemErr, bool *validErr)
{
	RetryRdErr rdData[NUMBER_OF_MMIO_REGISTERS_SETS] = {0};
	uint8_t devFunc = ((device_of_first_imc+imc)<<3) | 0;
	int i = 0;
	for (i=0; i<NUMBER_OF_MMIO_REGISTERS_SETS; i++) {
		registerContainValidErrorData[i] = false;
		validErr[i] = false;
	}
	
	TDBG("Getting register data: cpu=%u, bus=%u, imc=%u, devFunc=0x%02X, channel=%u\n",
			cpu, bus, imc, devFunc, chan);
		
	GatherRetryRegistersData(cpu, bus, type, devFunc, chan, rdData);

	ParseRetryRegistersData(cpu, type, imc, chan, pMemErr, rdData);
	
	for (i=0; i<NUMBER_OF_MMIO_REGISTERS_SETS; i++) {
		validErr[i] = registerContainValidErrorData[i];
	}
	ClearRetryRegister(cpu, bus, devFunc, chan, rdData);
}


int GatherRetryRegistersData(uint8_t cpu, uint8_t bus, CpuTypes type, uint8_t devFunc, uint8_t chan, RetryRdErr *rdData)
{
	if ((type == CPU_TYPE_ICX || type == CPU_TYPE_ICX2) && nooverInSet[0] == false && nooverInSet[1] == false) {
		TDBG("Both retry registers could be gathered");
		GatherBothRetryLogRegisters(cpu, bus, devFunc, chan, rdData);
	}
	else if (type != CPU_TYPE_ICX && type != CPU_TYPE_ICX2) {
		TDBG("Only set index 0 retry register could be gathered in case of Cpu Type different than ICX");
		GatherOneRetryLogRegister(cpu, bus, devFunc, chan, 0, rdData);
	}
	else if (nooverInSet[0] == true) {
		TDBG("Only set index 1 retry register could be gathered in case of noover bit in set index 0");
		GatherOneRetryLogRegister(cpu, bus, devFunc, chan, 1, rdData);
	}
	else if (nooverInSet[1] == true) {
		TDBG("Only set index 0 retry register could be gathered in case of noover bit in set index 1");
		GatherOneRetryLogRegister(cpu, bus, devFunc, chan, 0, rdData);
	}
	else {
		TCRIT("Unexpected condition while choosing which retry registers could be gathered");
		return -1;
	}
	return 0;
}


int GatherBothRetryLogRegisters(uint8_t cpu, uint8_t bus, uint8_t devFunc, uint8_t chan, RetryRdErr *rdData)
{
	uint8_t setIndex = 0;
	
	// SET1
	if ( 0 != GetMMIORegisterData(cpu, bus, devFunc, MMIO_REGISTERS_ADDRESSES_PACKAGE[chan][0].RETRY_RD_ERR_LOG_REG, &rdData[0].Log.Data) ) {
		TCRIT("Get Channel %u, Set1, RETRY_RD_ERR_LOG_REG failed\n", chan);
		//return -1;
	}

	TDBG("Get Channel %u, Set1, RETRY_RD_ERR_LOG_REG, read value = 0x%x", chan, rdData[0].Log.Data);
    
	// SET2
	if ( 0 != GetMMIORegisterData(cpu, bus, devFunc, MMIO_REGISTERS_ADDRESSES_PACKAGE[chan][1].RETRY_RD_ERR_LOG_REG, &rdData[1].Log.Data) ) {
		TCRIT("Get Channel %u, Set2, RETRY_RD_ERR_LOG_REG failed\n", chan);
		//return -1;
	}
	TDBG("Get Channel %u, Set2, RETRY_RD_ERR_LOG_REG, read value = 0x%x", chan, rdData[1].Log.Data);
	
    ChooseRetryRegisterSetNumber(rdData);
    if ( registerContainValidErrorData[0] == false && registerContainValidErrorData[1] == false) {
    	return -1;
    }

    for (setIndex = 0; setIndex < NUMBER_OF_MMIO_REGISTERS_SETS; setIndex++)
    {
        if (registerContainValidErrorData[setIndex] == true)
        {
            GatherSpecifiedSetRetryRegistersData(cpu, bus, devFunc, chan, setIndex, rdData);
        }
    }
    return 0;
}


int	GatherOneRetryLogRegister(uint8_t cpu, uint8_t bus, uint8_t devFunc, uint8_t chan, uint8_t setIndex, RetryRdErr *rdData)
{
	if (setIndex >= NUMBER_OF_MMIO_REGISTERS_SETS) {
		TCRIT("SETINDEX %u is out of range\n", setIndex);
		return -1;	
	}
	
	if ( 0 != GetMMIORegisterData(cpu, bus, devFunc, MMIO_REGISTERS_ADDRESSES_PACKAGE[chan][setIndex].RETRY_RD_ERR_LOG_ADDRESS1_REG, &rdData[setIndex].LogAddr1.Data) ) {
		TCRIT("Get Channel %u, Set %u, RETRY_RD_ERR_LOG_ADDRESS1_REG failed\n", chan, setIndex);
		return -1;
	}
	
	TDBG("Get Channel %u, Set %u, RETRY_RD_ERR_LOG_ADDRESS1_REG, read value = 0x%x", chan, setIndex, rdData->LogAddr1.Data);

    if (rdData[setIndex].Log.Bits.v != 1)
    {
        TCRIT("No Valid Err log\n");
        return -1;
    }

    if ( 0 != GatherSpecifiedSetRetryRegistersData(cpu, bus, devFunc, chan, setIndex, rdData)) {
    	return -1;
    }
    return 0;
}


int GatherSpecifiedSetRetryRegistersData(uint8_t cpu, uint8_t bus, uint8_t devFunc, uint8_t chan, uint8_t setIndex, RetryRdErr *rdData)
{
	if (setIndex >= NUMBER_OF_MMIO_REGISTERS_SETS) {
		TCRIT("SETINDEX %u is out of range\n", setIndex);
		return -1;	
	}
	
	if ( 0 != GetMMIORegisterData(cpu, bus, devFunc, MMIO_REGISTERS_ADDRESSES_PACKAGE[chan][setIndex].RETRY_RD_ERR_LOG_ADDRESS1_REG, &rdData[setIndex].LogAddr1.Data) ) {
		TCRIT("Get Channel %u, Set %u, RETRY_RD_ERR_LOG_ADDRESS1_REG failed\n", chan, setIndex);
		return -1;
	}
	
	TDBG("Get Channel %u, Set %u, RETRY_RD_ERR_LOG_ADDRESS1_REG, read value = 0x%x", chan, setIndex, rdData[setIndex].LogAddr1.Data);

	if ( 0 != GetMMIORegisterData(cpu, bus, devFunc, MMIO_REGISTERS_ADDRESSES_PACKAGE[chan][setIndex].RETRY_RD_ERR_LOG_ADDRESS2_REG, &rdData[setIndex].LogAddr2.Data) ) {
		TCRIT("Get Channel %u, Set %u, RETRY_RD_ERR_LOG_ADDRESS2_REG failed\n", chan, setIndex);
		return -1;
	}
	
	TDBG("Get Channel %u, Set %u, RETRY_RD_ERR_LOG_ADDRESS2_REG, read value = 0x%x", chan, setIndex, rdData[setIndex].LogAddr2.Data);
	
	if ( 0 != GetMMIORegisterData(cpu, bus, devFunc, MMIO_REGISTERS_ADDRESSES_PACKAGE[chan][setIndex].RETRY_RD_ERR_LOG_PARITY_REG, &rdData[setIndex].Parity.Data) ) {
		TCRIT("Get Channel %u, Set %u, RETRY_RD_ERR_LOG_ADDRESS2_REG failed\n", chan, setIndex);
		return -1;
	}
		
	TDBG("Get Channel %u, Set %u, RETRY_RD_ERR_LOG_ADDRESS2_REG, read value = 0x%x", chan, setIndex, rdData[setIndex].Parity.Data);
	
	if ( 0 != GetMMIORegister64bitData(cpu, bus, devFunc, MMIO_REGISTERS_ADDRESSES_PACKAGE[chan][setIndex].RETRY_RD_ERR_LOG_ADDRESS3, &rdData[setIndex].LogAddr3) ) {
		TCRIT("Get Channel %u, Set %u, RETRY_RD_ERR_LOG_ADDRESS2_REG failed\n", chan, setIndex);
		return -1;
	}
		
	TDBG("Get Channel %u, Set %u, RETRY_RD_ERR_LOG_ADDRESS3_REG, read value = 0x%llx", chan, setIndex, rdData[setIndex].LogAddr3);
	
	return 0;
}

void ChooseRetryRegisterSetNumber(RetryRdErr *rrdeSets)
{
	if ((rrdeSets[0].Log.Bits.v == 0) && (rrdeSets[1].Log.Bits.v == 0)) {
		TDBG("(RetryRdErrLog.Bits.v == 0) && (RetryRdErrLogSet2.Bits.v == 0), No Valid Error Data\n");
		registerContainValidErrorData[0] = false;
		registerContainValidErrorData[1] = false;      
	}
	else if ((rrdeSets[0].Log.Bits.v == 1) && (rrdeSets[1].Log.Bits.v == 0)) {
		TDBG("(RetryRdErrLog.Bits.v == 1) && (RetryRdErrLogSet2.Bits.v == 0)");
		registerContainValidErrorData[0] = true;
		registerContainValidErrorData[1] = false;
	}
	else if ((rrdeSets[0].Log.Bits.v == 0) && (rrdeSets[1].Log.Bits.v == 1)) {
		TDBG("(RetryRdErrLog.Bits.v == 0) && (RetryRdErrLogSet2.Bits.v == 1)");
		registerContainValidErrorData[0] = false;
		registerContainValidErrorData[1] = true;
	}
	else if ((rrdeSets[0].Log.Bits.v == 1) && (rrdeSets[1].Log.Bits.v == 1)) {
		TDBG("(RetryRdErrLog.Bits.v == 1) && (RetryRdErrLogSet2.Bits.v == 1)");
		// According to INTEL, CE from patrol scrub and ECC CE are different type of CEs. 
		// if en_patspr is set, this set of registers will record the CEs from patrol scrub, 
		// while false only ECC errors are recorded. 
		// if en_patsr values are same on 2 sets, only 1 set should be valid;
		// if en_patsr values differ on 2 sets, both sets should be valid
		if (rrdeSets[0].Log.Bits.en_patspr == rrdeSets[1].Log.Bits.en_patspr) {
			TDBG("RetryRdErrLog.Bits.en_patspr=0x%x\n", rrdeSets[0].Log.Bits.en_patspr);
			registerContainValidErrorData[0] = true;
			registerContainValidErrorData[1] = false;
		}
		else {
			TDBG("RetryRdErrLogSet1.Bits.en_patspr=0x%x\n, RetryRdErrLogSet2.Bits.en_patspr=0x%x\n", rrdeSets[0].Log.Bits.en_patspr, rrdeSets[1].Log.Bits.en_patspr);
			registerContainValidErrorData[0] = true;
			registerContainValidErrorData[1] = true;
		}
	}
}

int GetMMIORegisterData(uint8_t cpu, uint8_t bus, uint8_t devFunc, uint64_t registerAddress, uint32_t *resData)
{
	int8_t 	dev_id = 0;
	uint8_t target = getAddress(cpu);
	int8_t 	domain = 0;
	int8_t	xmit_feedback = 0;
	int8_t	awfcs = 1;
	int32_t readLen = 1;
	
	peci_rdendpointconfig_mmio_req_t	rdMMIOReq = {0};
	peci_rdendpointconfig_mmio_res_t	rdMMIORes = {0};
	
	*resData = 0;
	
	rdMMIOReq.option = DWORD;
	rdMMIOReq.host_id = 0;
	rdMMIOReq.message_type = PECI_ENDPTCFG_TYPE_MMIO;
	rdMMIOReq.bar = 0;
	rdMMIOReq.bus = bus;
	rdMMIOReq.devicefun = devFunc;
	rdMMIOReq.segment = 0;
	rdMMIOReq.address_type = mmio_bdf_and_bar_64bit_address_type;
	rdMMIOReq.pci_config_addr = registerAddress;
		
	if ( 0 != peci_cmd_rdendpointconfig_mmio(dev_id, target, domain, xmit_feedback, awfcs, &readLen, &rdMMIOReq, &rdMMIORes)) {
		TCRIT("peci_cmd_rdendpointconfig_mmio() failed: CC=0x%x\n", rdMMIORes.completion_code);
		TDBG("\ttarget:          %d",target);
		TDBG("\toption:          %d",rdMMIOReq.option);
		TDBG("\tbus:             %d",rdMMIOReq.bus);
		TDBG("\tdevicefun:       %d",rdMMIOReq.devicefun);
		TDBG("\taddress_type:    %d",rdMMIOReq.address_type);
		TDBG("\tpci_config_addr: 0x%llx\n",rdMMIOReq.pci_config_addr);
		return -1;
	} else {
		TDBG("peci_cmd_rdendpointconfig_mmio() OK: CC=0x%x\n", rdMMIORes.completion_code);
		TDBG("\ttarget:          %d",target);
		TDBG("\toption:          %d",rdMMIOReq.option);
		TDBG("\tbus:             %d",rdMMIOReq.bus);
		TDBG("\tdevicefun:       %d",rdMMIOReq.devicefun);
		TDBG("\taddress_type:    %d",rdMMIOReq.address_type);
		TDBG("\tpci_config_addr: 0x%llx\n",rdMMIOReq.pci_config_addr);
	}
	
	if ( rdMMIORes.completion_code != 0x40  && rdMMIORes.completion_code != 0x94 ) {
		TCRIT("CC = 0x%x\n", rdMMIORes.completion_code);
		return -1;
	}

	/* Ice Lake EDS Vol 1, chapter 10.3, PECI Response Codes 
	 * 0x94, Fatal MCA detected. Command Passed, but data may not be valid.
	 * Returned instead of 0x4x and there was a firmware
	 * machine check, to indicate that the response was
	 * provided on a best-efforts basis. Response can be
	 * taken as a pass with caution. Retry will not help to get
	 * a success completion code. Recommend to initiate
	 * crashdump
	 * However, peci still works properly as we observed
	 * The data are still valid
	 */

	memcpy(resData, &rdMMIORes.data, DWORD);
	return 0;	
}

int SetMMIORegisterData(uint8_t cpu, uint8_t bus, uint8_t devFunc, uint64_t registerAddress, uint32_t data)
{
	int8_t 	dev_id = 0;
	uint8_t target = getAddress(cpu);
	int8_t 	domain = 0;
	int8_t	xmit_feedback = 0;
	int8_t	awfcs = 1;
	int32_t readLen = 1;
	
	peci_wrendpointconfig_mmio_req_t	wrMMIOReq = {0};
	peci_wrendpointconfig_mmio_res_t	wrMMIORes = {0};
	
	wrMMIOReq.option = DWORD;
	wrMMIOReq.host_id = 0;
	wrMMIOReq.message_type = PECI_ENDPTCFG_TYPE_MMIO;
	wrMMIOReq.bar = 0;
	wrMMIOReq.bus = bus;
	wrMMIOReq.devicefun = devFunc;
	wrMMIOReq.segment = 0;
	wrMMIOReq.address_type = mmio_bdf_and_bar_64bit_address_type;
	wrMMIOReq.pci_config_addr = registerAddress;
	wrMMIOReq.data.peci_dword = data;

	if ( 0 != peci_cmd_wrendpointconfig_mmio(dev_id, target, domain, xmit_feedback, awfcs, &readLen, &wrMMIOReq, &wrMMIORes)) {
		TCRIT("peci_cmd_wrendpointconfig_mmio failed, CC=0x%x\n", wrMMIORes.completion_code);
		return -1;		
	}
	
	if ( wrMMIORes.completion_code != 0x40 && wrMMIORes.completion_code != 0x94  ) {
		TCRIT("CC = 0x%x\n", wrMMIORes.completion_code);
		return -1;
	}
	return 0;
}

int GetMMIORegister64bitData(uint8_t cpu, uint8_t bus, uint8_t devFunc, uint64_t registerAddress, uint64_t *resData)
{
	int8_t 	dev_id = 0;
	uint8_t target = getAddress(cpu);
	int8_t 	domain = 0;
	int8_t	xmit_feedback = 0;
	int8_t	awfcs = 1;
	int32_t readLen = 1;
	
	peci_rdendpointconfig_mmio_req_t	rdMMIOReq = {0};
	peci_rdendpointconfig_mmio_res_t	rdMMIORes = {0};
	
	*resData = 0;
	
	rdMMIOReq.option = QWORD;
	rdMMIOReq.host_id = 0;
	rdMMIOReq.message_type = PECI_ENDPTCFG_TYPE_MMIO;
	rdMMIOReq.bar = 0;
	rdMMIOReq.bus = bus;
	rdMMIOReq.devicefun = devFunc;
	rdMMIOReq.segment = 0;
	rdMMIOReq.address_type = mmio_bdf_and_bar_64bit_address_type;
	rdMMIOReq.pci_config_addr = registerAddress;
		
	if ( 0 != peci_cmd_rdendpointconfig_mmio(dev_id, target, domain, xmit_feedback, awfcs, &readLen, &rdMMIOReq, &rdMMIORes)) {
		TCRIT("peci_cmd_rdendpointconfig_mmio() failed: CC=0x%x\n", rdMMIORes.completion_code);
		TDBG("\ttarget:          %d",target);
		TDBG("\toption:          %d",rdMMIOReq.option);
		TDBG("\tbus:             %d",rdMMIOReq.bus);
		TDBG("\tdevicefun:       %d",rdMMIOReq.devicefun);
		TDBG("\taddress_type:    %d",rdMMIOReq.address_type);
		TDBG("\tpci_config_addr: 0x%llx\n",rdMMIOReq.pci_config_addr);
		return -1;
	} else {
		TDBG("peci_cmd_rdendpointconfig_mmio() OK: CC=0x%x\n", rdMMIORes.completion_code);
		TDBG("\ttarget:          %d",target);
		TDBG("\toption:          %d",rdMMIOReq.option);
		TDBG("\tbus:             %d",rdMMIOReq.bus);
		TDBG("\tdevicefun:       %d",rdMMIOReq.devicefun);
		TDBG("\taddress_type:    %d",rdMMIOReq.address_type);
		TDBG("\tpci_config_addr: 0x%llx\n",rdMMIOReq.pci_config_addr);
	}
	
	if ( rdMMIORes.completion_code != 0x40  && rdMMIORes.completion_code != 0x94 ) {
		TCRIT("CC = 0x%x\n", rdMMIORes.completion_code);
		return -1;
	}

	/* Ice Lake EDS Vol 1, chapter 10.3, PECI Response Codes 
	 * 0x94, Fatal MCA detected. Command Passed, but data may not be valid.
	 * Returned instead of 0x4x and there was a firmware
	 * machine check, to indicate that the response was
	 * provided on a best-efforts basis. Response can be
	 * taken as a pass with caution. Retry will not help to get
	 * a success completion code. Recommend to initiate
	 * crashdump
	 * However, peci still works properly as we observed
	 * The data are still valid
	 */

	memcpy(resData, &rdMMIORes.data, QWORD);
	return 0;	
}


void ParseRetryRegistersData(uint8_t cpu, CpuTypes type, uint8_t imc, uint8_t chan, MemErrorStruct *pMemErr, RetryRdErr *rrdeSets)
{
    TDBG("Starting parse of a dimm");
    uint8_t setIndex = 0;
    uint8_t Bank = 0;
    for (setIndex = 0; setIndex < NUMBER_OF_MMIO_REGISTERS_SETS; setIndex++)
    {
        TDBG("Bits: 0x%x", rrdeSets[setIndex].Log.Data);

        if (registerContainValidErrorData[setIndex] == true)
        {

            if (rrdeSets[setIndex].Log.Bits.noover == 1)
            {
                nooverInSet[setIndex] = true;
            }

            pMemErr[setIndex].socket = cpu;
            pMemErr[setIndex].imc = imc;
            pMemErr[setIndex].channel = chan;
            pMemErr[setIndex].rank = (uint8_t)(rrdeSets[setIndex].LogAddr1.Bits.chip_select);
            pMemErr[setIndex].slot = (pMemErr[setIndex].rank >> 2) & 0x1;
            Bank = (uint8_t)(rrdeSets[setIndex].LogAddr1.Bits.bank);
            if ( type == CPU_TYPE_SKX)
            {
            	pMemErr[setIndex].bankGroup = Bank & 0b00000011;
            	pMemErr[setIndex].bank = (Bank >> 2) & 0b00111111;
            }
            else
            {
            	pMemErr[setIndex].bankGroup = (Bank & 0b00000011) | ((Bank & 0b00010000) >> 2);
            	pMemErr[setIndex].bank = (Bank >> 2) & 0b00000011;
            }
            pMemErr[setIndex].row = (uint32_t)(rrdeSets[setIndex].LogAddr2.Bits.row) & 0x0003ffff;
            pMemErr[setIndex].col = (uint32_t)(rrdeSets[setIndex].LogAddr1.Bits.col) << 2;
            pMemErr[setIndex].errorType = (uint8_t)(rrdeSets[setIndex].Log.Bits.uc);
            pMemErr[setIndex].device = (uint8_t)(rrdeSets[setIndex].LogAddr1.Bits.failed_dev);
            pMemErr[setIndex].paritySyndrome = (uint32_t)(rrdeSets[setIndex].Parity.Bits.par_syn);
            pMemErr[setIndex].mode = (uint8_t)(rrdeSets[setIndex].Log.Bits.mode);
            
            /* record UC and read times
             * Since read error log is done by channel,and each channel has up to 2 DIMMs
             * The same uc could be read twice if a channel has 2 populated DIMMs
             * When read times reaches 3, then we think host is not dealing with this uc.
             * BMC then clears the retry register error valid bit uc bit in ClearRetryRegister
             */
            if ( pMemErr[setIndex].errorType==1 ) {
            	if ( rrdeSets[setIndex].Log.Bits.system_addr_valid ) {
            		TINFO("Check uce by sys address\n");
					if ( readUcErrorTimes==0 ) {
						ucErrorSysAddr=rrdeSets[setIndex].LogAddr3 & SYS_ADDR_MASK;
						readUcErrorTimes++;
					}
					else {
						/* if a different ue occurs too, skip it to deal with current ue first */
						if ( ucErrorSysAddr == (rrdeSets[setIndex].LogAddr3 & SYS_ADDR_MASK)) {
							readUcErrorTimes++;
						}
					}
					if ( readUcErrorTimes<3 ) {
						registerContainValidErrorData[setIndex] = false;
					}
            	}
            	else {
            		TINFO("Check uce by dimm address\n");
					if ( readUcErrorTimes==0 ) {
						memcpy(&ucMemErr, pMemErr, sizeof(ucMemErr));
						readUcErrorTimes++;
					}
					else {
						/* if a different ue occurs too, skip it to deal with current ue first */
						if (!memcmp(&ucMemErr, pMemErr, sizeof(ucMemErr))) {
							readUcErrorTimes++;
						}
					}
					if ( readUcErrorTimes<3 ) {
						registerContainValidErrorData[setIndex] = false;
					}
            	}
            }
        }
    }
}

int ClearRetryRegister(uint8_t cpu, uint8_t bus, uint8_t devFunc, uint8_t chan, RetryRdErr *rrdeSets)
{
	TDBG("Clearing register data for dimm");
	
	for (uint8_t setIndex = 0; setIndex < NUMBER_OF_MMIO_REGISTERS_SETS; setIndex++) {
		if (registerContainValidErrorData[setIndex] == true && nooverInSet[setIndex] == false && rrdeSets[setIndex].Log.Bits.uc == 0 ) {
			rrdeSets[setIndex].Log.Bits.v = 0;
	
			if (0 != SetMMIORegisterData(cpu, bus, devFunc, MMIO_REGISTERS_ADDRESSES_PACKAGE[chan][setIndex].RETRY_RD_ERR_LOG_REG, rrdeSets[setIndex].Log.Data)) {
				TCRIT("PECI not responding : ClearRetryRegister Channel=0%u, SET=%u", chan, setIndex);
				return -1;
			}
		}
		if (registerContainValidErrorData[setIndex] == true && nooverInSet[setIndex] == false && rrdeSets[setIndex].Log.Bits.uc == 1 && readUcErrorTimes >=3 ) {
			rrdeSets[setIndex].Log.Bits.v = 0;
			rrdeSets[setIndex].Log.Bits.uc = 0;
			readUcErrorTimes = 0;
			if (0 != SetMMIORegisterData(cpu, bus, devFunc, MMIO_REGISTERS_ADDRESSES_PACKAGE[chan][setIndex].RETRY_RD_ERR_LOG_REG, rrdeSets[setIndex].Log.Data)) {
				TCRIT("PECI not responding : ClearRetryRegister Channel=0%u, SET=%u", chan, setIndex);
				return -1;
			}
		}
		registerContainValidErrorData[setIndex] = false;
	}
	
	return true;
}


#if defined (CONFIG_SPX_FEATURE_MFP_2_1)
uint64_t CalculateCorrErrCntRegisterOffset(MemErrorStruct *pMemErr, uint8_t index)
{
    uint64_t registerOffset = 0;
    if (index == 0)
    {
        if ( pMemErr[index].rank == 0 ||  pMemErr[index].rank == 1)
        {
            registerOffset = PECI_CORR_ERR_CNT_0_CHN0_OFFSET;
        }
        else if ( pMemErr[index].rank == 2 ||  pMemErr[index].rank == 3)
        {
            registerOffset = PECI_CORR_ERR_CNT_1_CHN0_OFFSET;
        }
        else if ( pMemErr[index].rank == 4 ||  pMemErr[index].rank == 5)
        {
            registerOffset = PECI_CORR_ERR_CNT_2_CHN0_OFFSET;
        }
        else if ( pMemErr[index].rank == 6 ||  pMemErr[index].rank == 7)
        {
            registerOffset = PECI_CORR_ERR_CNT_3_CHN0_OFFSET;
        }
        else
        {
			TCRIT("CalculateCorrErrCntRegisterOffset : Wrong rank value=%u\n",
					pMemErr[index].rank);
        }
    }
    else if (index == 1)
    {
        if ( pMemErr[index].rank == 0 ||  pMemErr[index].rank == 1)
        {
            registerOffset = PECI_CORR_ERR_CNT_0_CHN1_OFFSET;
        }
        else if ( pMemErr[index].rank == 2 ||  pMemErr[index].rank == 3)
        {
            registerOffset = PECI_CORR_ERR_CNT_1_CHN1_OFFSET;
        }
        else if ( pMemErr[index].rank == 4 ||  pMemErr[index].rank == 5)
        {
            registerOffset = PECI_CORR_ERR_CNT_2_CHN1_OFFSET;
        }
        else if ( pMemErr[index].rank == 6 ||  pMemErr[index].rank == 7)
        {
            registerOffset = PECI_CORR_ERR_CNT_3_CHN1_OFFSET;
        }
        else
        {
			TCRIT("CalculateCorrErrCntRegisterOffset : Wrong rank value=%u\n",
					pMemErr[index].rank);
        }
    }
    else
    {
		TCRIT("CalculateCorrErrCntRegisterOffset : Wrong channel index=%u\n", index);
    }

    return registerOffset;
}

uint64_t CalculateCorrErrorStatusRegisterOffset(uint8_t index)
{
    uint64_t registerOffset = 0;
    if (index == 0)
    {
        registerOffset = PECI_CORR_ERROR_STATUS_CHN0_OFFSET;
    }
    else if (index == 1)
    {
        registerOffset = PECI_CORR_ERROR_STATUS_CHN1_OFFSET;
    }
    else
    {
        TCRIT("CalculateCorrErrorStatusRegisterOffset: Wrong channel index: %u\n", index);
    }

    return registerOffset;
}

CORR_ERROR_STATUS_T RetrieveCorrErrorStatus(uint8_t cpu, uint8_t bus, uint8_t devFunc, uint8_t chan, uint64_t registerOffset)
{
    CORR_ERROR_STATUS_T corrErrStatus = {0};
	if ( 0 != GetMMIORegisterData(cpu, bus, devFunc, registerOffset, &corrErrStatus.data) ) {
		TCRIT("Retrieving CORR_ERROR_STATUS failed failed: %d\n", chan);
	}

    return corrErrStatus;
}

CORR_ERR_CNT_T RetrieveCorrErrCnt(uint8_t cpu, uint8_t bus, uint8_t devFunc, uint8_t chan, uint64_t registerOffset)
{
	CORR_ERR_CNT_T corrErrCnt = {0};
	if ( 0 != GetMMIORegisterData(cpu, bus, devFunc, registerOffset, &corrErrCnt.data) ) {
		TCRIT("Retrieving CORR_ERROR_STATUS failed failed: %d\n", chan);
	}

    return corrErrCnt;
}

void SetCorrErrorStatus(int8_t cpu, uint8_t bus, uint8_t devFunc, uint8_t chan, CORR_ERROR_STATUS_T corrErrStatus, uint64_t registerOffset)
{
	if ( 0 != SetMMIORegisterData(cpu, bus, devFunc, registerOffset, corrErrStatus.data) ) {
		TCRIT("Setting CORR_ERROR_STATUS[%d] failed\n", chan);
	}
}

void SetCorrErrCnt(int8_t cpu, uint8_t bus, uint8_t devFunc, uint8_t chan, CORR_ERR_CNT_T corrErrCnt, uint64_t registerOffset)
{
	if ( 0 != SetMMIORegisterData(cpu, bus, devFunc, registerOffset, corrErrCnt.data) ) {
		TCRIT("Setting CORR_ERR_CNT[%d] failed\n", chan);
	}
}

void ClearCorrErrorStatus(uint8_t cpu, uint8_t bus, uint8_t devFunc, MemErrorStruct *pMemErr)
{
	uint64_t registerOffset;
	CORR_ERROR_STATUS_T corrErrStatus;
    uint8_t rankBitToClear = 1;
	int i;
	for (i=0; i<NUMBER_OF_MMIO_REGISTERS_SETS; i++) {
		registerOffset = CalculateCorrErrorStatusRegisterOffset(i);
	    corrErrStatus = RetrieveCorrErrorStatus(cpu, bus, devFunc, i, registerOffset);

	    // In these bits occurs inverse clearing logic by setting 1 to the bit which needs to be cleared
	    if (pMemErr[i].rank != 0)
	    {
	        rankBitToClear = rankBitToClear << pMemErr[i].rank;
	    }
	    corrErrStatus.bits.ERR_OVERFLOW_STAT = rankBitToClear;

	    TCRIT("Clearing CorrErrorStatus bits\n");
	    SetCorrErrorStatus(cpu, bus, devFunc, i, corrErrStatus, registerOffset);

	    corrErrStatus = RetrieveCorrErrorStatus(cpu, bus, devFunc, i, registerOffset);
	    TDBG("corrErrStatus[%d]: ov %u, al %u, alsl %u, rl %u\n", i,
				corrErrStatus.bits.ERR_OVERFLOW_STAT, corrErrStatus.bits.DIMM_ALERT,
				corrErrStatus.bits.DIMM_ALERT_SELECT, corrErrStatus.bits.DDR4CRC_RANK_LOG);
	}
}

void ClearCorrErrCnt(uint8_t cpu, uint8_t bus, uint8_t devFunc, MemErrorStruct *pMemErr)
{
	uint64_t registerOffset;
	CORR_ERR_CNT_T corrErrCnt;

	int i;
	for (i=0; i<NUMBER_OF_MMIO_REGISTERS_SETS; i++) {
		registerOffset = CalculateCorrErrCntRegisterOffset(pMemErr, i);
		corrErrCnt = RetrieveCorrErrCnt(cpu, bus, devFunc, i, registerOffset);

	    if (pMemErr[i].rank % 2 == 0)
	    {
	        corrErrCnt.bits.COR_ERR_CNT_0 = 0;
	    }
	    else
	    {
	        corrErrCnt.bits.COR_ERR_CNT_1 = 0;
	    }

	    TCRIT("Clearing CorrErrCnt bits");
	    SetCorrErrCnt(cpu, bus, devFunc, i, corrErrCnt, registerOffset);

	    corrErrCnt = RetrieveCorrErrCnt(cpu, bus, devFunc, i, registerOffset);
	    TDBG("corrErrCnt[%d]: cor_err_0 %u, ov_0 %u, cor_err_` %u, ov_` %u\n", i,
				corrErrCnt.bits.COR_ERR_CNT_0, corrErrCnt.bits.OVERFLOW_0,
				corrErrCnt.bits.COR_ERR_CNT_1, corrErrCnt.bits.OVERFLOW_1);
	}
}
#endif /* CONFIG_SPX_FEATURE_MFP_2_1 */

