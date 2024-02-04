#include <string.h>
#include <endian.h>
#include <dbgout.h>
#include "peciifc.h"
#include "cpu.h"
#include "channel.h"

#define wakePECIAfterHostReset	1

int isPECIEnabled(uint8_t index, unsigned int *enabled)
{
	/***************************************************************************
	 * Check Ice Lake SP EDS, Vol 1, chapter 10.4 Power and Thermal Optimization
	 * Capabilities Package Config Space (PCS)
	 * PECI may be initially disabled after reboot
	 ***************************************************************************/
	int retVal = 0;
	int8_t 	dev_id = 0;
	uint8_t target = getAddress(index);
	int8_t 	domain = 0;
	int8_t	xmit_feedback = 0;
	int8_t	awfcs = 1;
	int32_t readLen = 0;
	
	peci_rdpkgconfig_req_t rdpkg_in = {0};
	peci_rdpkgconfig_res_t rdpkg_out = {0};
	
	rdpkg_in.option = DWORD;
	rdpkg_in.host_id = 0;
	rdpkg_in.index = peci_pkg_index_wake_peci;
	rdpkg_in.parameter =peci_pkg_param_set_wake_peci;
	
	if (0 == peci_cmd_rdpkgconfig(dev_id, target, domain, xmit_feedback, awfcs, &readLen, &rdpkg_in, &rdpkg_out)) {
		TDBG("\tCompletion Code : %x\n", rdpkg_out.completion_code);
		if ( rdpkg_out.completion_code != 0x40 ) {
			TCRIT("\tCompletion Code : %x\n", rdpkg_out.completion_code);
			retVal = -1; 
		} 
		else {
			TDBG("\tData : %lx\n", rdpkg_out.data.peci_dword);
			*enabled = rdpkg_out.data.peci_dword;
			if (!rdpkg_out.data.peci_dword)  {
				TCRIT("PECI is not waked up\n");
			}
		}
	}
	else {
		TCRIT("\tERR : RdPkgConfig command failed\n");
		retVal = -1;
	}
	return retVal;
}

int WakePECI(uint8_t index)
{
	int retVal = 0;
	int8_t 	dev_id = 0;
	uint8_t target = getAddress(index);
	int8_t 	domain = 0;
	int8_t	xmit_feedback = 0;
	int8_t	awfcs = 1;
	int32_t readLen = 1;
	
	peci_wrpkgconfig_req_t wrpkg_in = {0};	
	peci_wrpkgconfig_res_t wrpkg_out = {0};	
	
	wrpkg_in.option = DWORD;
	wrpkg_in.host_id = 0;
	wrpkg_in.index = peci_pkg_index_wake_peci;
	wrpkg_in.parameter = peci_pkg_param_set_wake_peci;
	wrpkg_in.data.peci_dword = peci_pkg_val_wake_peci_on;

	if(0 == peci_cmd_wrpkgconfig(dev_id, target, domain, xmit_feedback, awfcs, &readLen, &wrpkg_in, &wrpkg_out)) {
		TDBG("\tCompletion Code : %x\n", wrpkg_out.completion_code);
		retVal = 0;
	}
	else {
		TCRIT("\tERR : WrPkgConfig command failed, CC=0x%x\n", wrpkg_out.completion_code);
		retVal = -1;
	}
	return retVal;
}


uint8_t getAddress(uint8_t index)
{
    return index + MIN_CPU_ADDRESS;
}


int DetectCpuType(uint8_t index, CpuTypes *type)
{
	int		retVal = 0;
	int8_t 	dev_id = 0;
	uint8_t target = getAddress(index);
	int8_t 	domain = 0;
	int8_t	xmit_feedback = 0;
	int8_t	awfcs = 1;
	int32_t readLen = 1;
	
	peci_rdpkgconfig_req_t rdpkg_in = {0};
	peci_rdpkgconfig_res_t rdpkg_out = {0};
	uint32_t cpuid = 0;
	CpuInfo cpuInfo;

	rdpkg_in.option = DWORD;
	rdpkg_in.host_id = 0;
	rdpkg_in.index = peci_pkg_index_pkg_id;
	rdpkg_in.parameter = peci_pkg_param_cpu_id;
    
	if(0 != peci_cmd_rdpkgconfig(dev_id, target, domain, xmit_feedback, awfcs, &readLen, &rdpkg_in, &rdpkg_out)) {
		TCRIT("\tERR : RdPkgConfig command failed, CC = 0x%x\n", rdpkg_out.completion_code);
		return -1;		
	}
	
	if ( rdpkg_out.completion_code != 0x40 ) {
		TCRIT("CC = 0x%x\n", rdpkg_out.completion_code);
		return -1;
	}
	
	memcpy(&cpuid, &rdpkg_out.data, sizeof(cpuid));
	ParseCpuInfo(cpuid, &cpuInfo);

    switch (cpuInfo.cpuModelData) {
    	case skx_model:
    		if (cpuInfo.stepping >= cpx_stepping) {
    			TDBG("   CPX detected, Model = 0x%x, Stepping = 0x%x \n", cpuInfo.cpuModelData, cpuInfo.stepping);
    			*type = CPU_TYPE_CPX;
    		}
    		else if (cpuInfo.stepping >= clx_stepping) {
    			TDBG("   CLX detected , Model = 0x%x, Stepping = 0x%x \n", cpuInfo.cpuModelData, cpuInfo.stepping);
    			*type = CPU_TYPE_CLX;
    		}
    		else {
    			TDBG("   SKX detected , Model = 0x%x, Stepping = 0x%x \n", cpuInfo.cpuModelData, cpuInfo.stepping);
    			*type = CPU_TYPE_SKX;
    		}
    		break;
		case icx_model:
			TDBG("   ICX detected , Model = 0x%x, Stepping = 0x%x \n", cpuInfo.cpuModelData, cpuInfo.stepping);
		
			if (cpuInfo.stepping >= icx2_stepping) {
				*type = CPU_TYPE_ICX2;
			}
			else {
				*type = CPU_TYPE_ICX;
			}
			break;
		default:
			TCRIT("   Unsupported CPUID: , Model = 0x%x, Stepping = 0x%x \n", cpuInfo.cpuModelData, cpuInfo.stepping);
			*type = CPU_TYPE_UNSUPPORTED;
			retVal = -1;
			break;
    }

    return retVal;
}


int ValidatePECIBus(uint8_t index)
{
	int8_t 	dev_id = 0;
	uint8_t target = getAddress(index);
	int8_t 	domain = 0;
	int8_t	xmit_feedback = 0;
	int8_t	awfcs = 1;
	int32_t readLen = 1;

	peci_rdpkgconfig_req_t rdpkg_in = {0};
	peci_rdpkgconfig_res_t rdpkg_out = {0};
	uint32_t u32PkgValue = 0;

	rdpkg_in.option = DWORD;
	rdpkg_in.host_id = 0;
	rdpkg_in.index = pcs_index_for_msm_regs;
	rdpkg_in.parameter = cpubusno_valid;
	
	if(0 != peci_cmd_rdpkgconfig(dev_id, target, domain, xmit_feedback, awfcs, &readLen, &rdpkg_in, &rdpkg_out)) {
		TCRIT("\tERR : RdPkgConfig command failed, CC = 0x%x\n", rdpkg_out.completion_code);
		return -1;		
	}
	
	if ( rdpkg_out.completion_code != 0x40 ) {
		TCRIT("CC = 0x%x\n", rdpkg_out.completion_code);
		return -1;
	}
	
	memcpy(&u32PkgValue, &rdpkg_out.data, sizeof(u32PkgValue));
	TDBG("ValidatePECIBus%u 0x%x \n", index, u32PkgValue);
	if (!(u32PkgValue & (1 << 10))) {
		TCRIT("Validating CPUBUSNO %u failed\n", index);
		return -1;
	}
	return 0;
}


int RetrievePECIBus(uint8_t index, uint8_t *bus)
{
	int8_t 	dev_id = 0;
	uint8_t target = getAddress(index);
	int8_t 	domain = 0;
	int8_t	xmit_feedback = 0;
	int8_t	awfcs = 1;
	int32_t readLen = 1;

	peci_rdpkgconfig_req_t rdpkg_in = {0};
	peci_rdpkgconfig_res_t rdpkg_out = {0};
    uint32_t u32PkgValue = 0;
    
	rdpkg_in.option = DWORD;
	rdpkg_in.host_id = 0;
	rdpkg_in.index = pcs_index_for_msm_regs;
	rdpkg_in.parameter = cpubusno2;
	
	if(0 != peci_cmd_rdpkgconfig(dev_id, target, domain, xmit_feedback, awfcs, &readLen, &rdpkg_in, &rdpkg_out)) {
		TCRIT("\tERR : Retrieving Bus number failed, CC = 0x%x\n", rdpkg_out.completion_code);
		return -1;		
	}
	
	if ( rdpkg_out.completion_code != 0x40 ) {
		TCRIT("CC = 0x%x\n", rdpkg_out.completion_code);
		return -1;
	}
	
	memcpy(&u32PkgValue, &rdpkg_out.data, sizeof(u32PkgValue));
	TDBG("RetrievePECIBus%u 0x%x \n", index, u32PkgValue);
	*bus = (uint8_t)((u32PkgValue >> 16) & 0x000000ff);
	return 0;
}


int ParseCpuInfo(uint32_t id, CpuInfo *info)
{
    info->cpuModelData = id & 0xFFFFFFF0;
    info->stepping = id & 0x0000000F;
    return 0;
}


int PeciCheckDIMMPopulationAmount(uint8_t index, uint8_t *dimmAmount)
{
	*dimmAmount = 0;
	uint8_t imc = 0;
	uint8_t presence = 0;
	
	for (imc = 0; imc < MAX_AMOUNT_OF_IMCS; imc++) {
		if ( 0 == PeciCheckSingleDimmPresence(index, imc, 0, 0, &presence)) {
			*dimmAmount += presence;
		}
		else {
			return -1;
		}
		
		if ( 0 == PeciCheckSingleDimmPresence(index, imc, 0, 1, &presence)) {
			*dimmAmount += presence;
		}
		else {
			return -1;
		}
		
		if (0 == PeciCheckSingleDimmPresence(index, imc, 1, 0, &presence)) {
			*dimmAmount += presence;
		}
		else {
			return -1;
		}
		
		if (0 == PeciCheckSingleDimmPresence(index, imc, 1, 1, &presence)) {
			*dimmAmount += presence;
		}
		else {
			return -1;
		}
	}
	return 0;	
	
}


int PeciCheckSingleDimmPresence(uint8_t index, uint8_t imc, uint8_t channel, uint8_t slot, uint8_t *presence)
{
	int8_t 	dev_id = 0;
	uint8_t target = getAddress(index);
	int8_t 	domain = 0;
	int8_t	xmit_feedback = 0;
	int8_t	awfcs = 1;
	int32_t readLen = 1;
	uint8_t	bus = 0;

	peci_rdendpointconfig_mmio_req_t rdEndCfgMMIO_in = {0};
	peci_rdendpointconfig_mmio_res_t rdEndCfgMMIO_out = {0};
    uint32_t u32PciReadVal = 0;
    uint32_t regOffset = 0;
    
    if ( RetrievePECIBus(index, &bus) ) {
    	return -1;
    }
    
    if ( channel==0 && slot==0 ) {
    	regOffset = DIMM_CH_0_SLOT_0_REG;
    }
    else if ( channel==0 && slot==1 ) {
    	regOffset = DIMM_CH_0_SLOT_1_REG;
    }
    else if ( channel==1 && slot==0 ) {
    	regOffset = DIMM_CH_1_SLOT_0_REG;
    }
    else if ( channel==1 && slot==1 ) {
    	regOffset = DIMM_CH_1_SLOT_1_REG;
    }
    else {
    	TCRIT("channel=%u>1,slot=%u>1, out of range\n", channel, slot);
    	return -1;
    }
    
    *presence = 0;

    rdEndCfgMMIO_in.option = DWORD;
    rdEndCfgMMIO_in.host_id = 0;
    rdEndCfgMMIO_in.message_type = PECI_ENDPTCFG_TYPE_MMIO;
    rdEndCfgMMIO_in.bar = 0;
    rdEndCfgMMIO_in.bus = bus;
    rdEndCfgMMIO_in.devicefun = ((device_of_first_imc+imc)<<3) | 0;
    rdEndCfgMMIO_in.segment = 0;
    rdEndCfgMMIO_in.pci_config_addr = regOffset;
    rdEndCfgMMIO_in.address_type = mmio_bdf_and_bar_32bit_address_type; /*Intel source set address_type to 32 bit*/
	
	if ( 0 != peci_cmd_rdendpointconfig_mmio(dev_id, target, domain, xmit_feedback, awfcs, &readLen, &rdEndCfgMMIO_in, &rdEndCfgMMIO_out)) {
        TCRIT("Checking population amount failed, CC = 0x%x\n", rdEndCfgMMIO_out.completion_code);
        return -1;
	}

	if ( rdEndCfgMMIO_out.completion_code != 0x40 ) {
		TCRIT("CC = 0x%x\n", rdEndCfgMMIO_out.completion_code);
		return -1;
	}
	
	memcpy(&u32PciReadVal, &rdEndCfgMMIO_out.data, sizeof(u32PciReadVal));
    if (u32PciReadVal & (1 << 15)) {
        *presence = 1;
    }

    return 0;
}


int CPUAddDimm(uint8_t index, uint8_t imc, uint8_t channel, uint8_t slot, uint8_t presence)
{
	UN_USED(index);
	UN_USED(imc);
	UN_USED(channel);
	UN_USED(slot);
	UN_USED(presence);
	TINFO("AddDimm is not implemented as of now\n");
	return 0;
}

int ScanDIMMsOnSPD(uint8_t index)
{
	uint8_t imc = 0;
	uint8_t presence = 0;
	
	for (imc = 0; imc < MAX_AMOUNT_OF_IMCS; imc++) {
		if ( 0 == PeciCheckSingleDimmPresence(index, imc, 0, 0, &presence)) {
			CPUAddDimm(index, imc, 0, 0, presence);
		}
		else {
			return -1;
		}
		
		if ( 0 == PeciCheckSingleDimmPresence(index, imc, 0, 1, &presence)) {
			CPUAddDimm(index, imc, 0, 1, presence);
		}
		else {
			return -1;
		}
		
		if (0 == PeciCheckSingleDimmPresence(index, imc, 1, 0, &presence)) {
			CPUAddDimm(index, imc, 1, 0, presence);
		}
		else {
			return -1;
		}
		
		if (0 == PeciCheckSingleDimmPresence(index, imc, 1, 1, &presence)) {
			CPUAddDimm(index, imc, 1, 1, presence);
		}
		else {
			return -1;
		}
	}
	return 0;
}
