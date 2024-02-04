
#ifndef __CPU_H__
#define __CPU_H__

#include <stdint.h>
#include <stdbool.h>
#include "mfp_structs.h"

#define skx_model		0x00050650
#define icx_model		0x000606A0
#define cpx_stepping	10
#define clx_stepping	6
#define icx2_stepping	4

#define	cpubusno2		4
#define	cpubusno_valid	5
#define	peci_pkg_index_pkg_id	0
#define	pcs_index_for_msm_regs	76
#define	peci_pkg_param_cpu_id	0
#define	peci_pkg_index_wake_peci		5
#define	peci_pkg_param_set_wake_peci	1
#define	peci_pkg_val_wake_peci_on		1

#define PECI_ENDPTCFG_TYPE_MMIO            0x05      //sync with the counterpart in peciifc.c because no header file is available

uint8_t getAddress(uint8_t index);
uint8_t GetPeciDimmsAmount();
int ScanDIMMsOnSPD(uint8_t index);

int WakePECI(uint8_t index);
int DetectCpuType(uint8_t index, CpuTypes *type);
int ValidatePECIBus(uint8_t index);
int RetrievePECIBus(uint8_t index, uint8_t *bus);
int ParseCpuInfo(uint32_t id, CpuInfo *info);
int PeciCheckDIMMPopulationAmount(uint8_t index, uint8_t *dimmAmount);
int PeciCheckSingleDimmPresence(uint8_t index, uint8_t imc, uint8_t channel, uint8_t slot, uint8_t *presence);
int CPUAddDimm(uint8_t index, uint8_t imc, uint8_t channel, uint8_t slot, uint8_t presence);
int isPECIEnabled(uint8_t index, unsigned int *enabled);

#endif
