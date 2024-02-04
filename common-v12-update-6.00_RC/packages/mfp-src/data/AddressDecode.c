/** @file
  Implementation of system address translation.

  @copyright
  INTEL CONFIDENTIAL
  Copyright 2016 - 2021 Intel Corporation. <BR>

  The source code contained or described herein and all documents related to the
  source code ("Material") are owned by Intel Corporation or its suppliers or
  licensors. Title to the Material remains with Intel Corporation or its suppliers
  and licensors. The Material may contain trade secrets and proprietary    and
  confidential information of Intel Corporation and its suppliers and licensors,
  and is protected by worldwide copyright and trade secret laws and treaty
  provisions. No part of the Material may be used, copied, reproduced, modified,
  published, uploaded, posted, transmitted, distributed, or disclosed in any way
  without Intel's prior express written permission.

  No license under any patent, copyright, trade secret or other intellectual
  property right is granted to or conferred upon you by disclosure or delivery
  of the Materials, either expressly, by implication, inducement, estoppel or
  otherwise. Any license under such intellectual property rights must be
  express and approved by Intel in writing.

  Unless otherwise agreed by Intel in writing, you may not remove or alter
  this notice or any other notice embedded in Materials by Intel or
  Intel's suppliers or licensors in any way.
**/

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/errno.h>
#include <signal.h>
#include <sys/time.h>
#include "Types.h"
#include "dbgout.h"
#include "AddressDecodeLib.h"
#include "bios.h"
#include "peciifc.h"
#include "mfp.h"
#include "mfp_ami.h"

#define MC_ROUTE_TABLE          0
#define CH_ROUTE_TABLE          1

#define VGA_RAM_BASE            0xA0000

#define MAX_MODE_TABLES_IMC   2  //Mode0 and Mode1 per IMC

#define RTx_INTLV_SHIFT_6 0
#define RTx_INTLV_SHIFT_7 1
#define RTx_INTLV_SHIFT_8 2
#define RTx_INTLV_SHIFT_9 3
#define RTx_INTLV_SHIFT_10 4
#define RTx_INTLV_SHIFT_11 5
#define RTx_INTLV_SHIFT_12 6
#define RTx_INTLV_SHIFT_13 7

#define INTERNAL_TAD_RULES  20

/* Default parameters */
#define BIOS_ADDR_TRAN_TIMEOUT    30
#define DEFAULT_BIOS_SKT_PRESENT  3 /* BIT0 | BIT1. Each bit represents a CPU. getDimm() */
#define DEFAULT_BIOS_MM_CFG_BASE  0x80000000
#define DEFAULT_BIOS_MM_CFG_LIMIT 0x90000000
#define DEFAULT_BIOS_MMIO_L_BASE  0x90000000
#define DEFAULT_BIOS_MMIO_L_LIMIT 0xFBFFFFFF
#define DEFAULT_BIOS_MMIO_H_BASE  0x200000000000
#define DEFAULT_BIOS_MMIO_H_LIMIT 0x209FFFFFFFFF
#define DEFAULT_BIOS_TOT_CHA      0x24 /* 24: WC-new, 26: WC-BPS, BIOS */ /* This may change based on the processor SKU */


//
// Register offsets created as arrays for portability across silicon generations
//
UINT32 mRemoteDramRuleCfgN0[REMOTE_SAD_RULES_10NM_WAVE1] = { /* page-3446. $15.3.66: 8 BYTE size */
  REMOTE_DRAM_RULE_CFG_0_N0_CHABC_SAD_REG,  REMOTE_DRAM_RULE_CFG_1_N0_CHABC_SAD_REG,  REMOTE_DRAM_RULE_CFG_2_N0_CHABC_SAD_REG,
  REMOTE_DRAM_RULE_CFG_3_N0_CHABC_SAD_REG,  REMOTE_DRAM_RULE_CFG_4_N0_CHABC_SAD_REG,  REMOTE_DRAM_RULE_CFG_5_N0_CHABC_SAD_REG,
  REMOTE_DRAM_RULE_CFG_6_N0_CHABC_SAD_REG,  REMOTE_DRAM_RULE_CFG_7_N0_CHABC_SAD_REG,  REMOTE_DRAM_RULE_CFG_8_N0_CHABC_SAD_REG,
  REMOTE_DRAM_RULE_CFG_9_N0_CHABC_SAD_REG,  REMOTE_DRAM_RULE_CFG_10_N0_CHABC_SAD_REG, REMOTE_DRAM_RULE_CFG_11_N0_CHABC_SAD_REG,
  REMOTE_DRAM_RULE_CFG_12_N0_CHABC_SAD_REG, REMOTE_DRAM_RULE_CFG_13_N0_CHABC_SAD_REG
};

UINT32 mRemoteDramRuleCfgN1[REMOTE_SAD_RULES_10NM_WAVE1] = { /* 8 BYTE size */
  REMOTE_DRAM_RULE_CFG_0_N1_CHABC_SAD_REG,  REMOTE_DRAM_RULE_CFG_1_N1_CHABC_SAD_REG,  REMOTE_DRAM_RULE_CFG_2_N1_CHABC_SAD_REG,
  REMOTE_DRAM_RULE_CFG_3_N1_CHABC_SAD_REG,  REMOTE_DRAM_RULE_CFG_4_N1_CHABC_SAD_REG,  REMOTE_DRAM_RULE_CFG_5_N1_CHABC_SAD_REG,
  REMOTE_DRAM_RULE_CFG_6_N1_CHABC_SAD_REG,  REMOTE_DRAM_RULE_CFG_7_N1_CHABC_SAD_REG,  REMOTE_DRAM_RULE_CFG_8_N1_CHABC_SAD_REG,
  REMOTE_DRAM_RULE_CFG_9_N1_CHABC_SAD_REG,  REMOTE_DRAM_RULE_CFG_10_N1_CHABC_SAD_REG, REMOTE_DRAM_RULE_CFG_11_N1_CHABC_SAD_REG,
  REMOTE_DRAM_RULE_CFG_12_N1_CHABC_SAD_REG, REMOTE_DRAM_RULE_CFG_13_N1_CHABC_SAD_REG
};

UINT32 mDramRuleCfgCha[SAD_RULES_10NM] = { /* page-3436. $15.3.34: 4 BYTE size */
  DRAM_RULE_CFG_0_CHABC_SAD_REG, DRAM_RULE_CFG_1_CHABC_SAD_REG, DRAM_RULE_CFG_2_CHABC_SAD_REG,
  DRAM_RULE_CFG_3_CHABC_SAD_REG, DRAM_RULE_CFG_4_CHABC_SAD_REG, DRAM_RULE_CFG_5_CHABC_SAD_REG,
  DRAM_RULE_CFG_6_CHABC_SAD_REG, DRAM_RULE_CFG_7_CHABC_SAD_REG, DRAM_RULE_CFG_8_CHABC_SAD_REG,
  DRAM_RULE_CFG_9_CHABC_SAD_REG, DRAM_RULE_CFG_10_CHABC_SAD_REG, DRAM_RULE_CFG_11_CHABC_SAD_REG,
  DRAM_RULE_CFG_12_CHABC_SAD_REG, DRAM_RULE_CFG_13_CHABC_SAD_REG, DRAM_RULE_CFG_14_CHABC_SAD_REG,
  DRAM_RULE_CFG_15_CHABC_SAD_REG
};

UINT32 mInterleaveListCfgCha[SAD_RULES_10NM] = { /* page-3437. $15.3.35: 4 BYTE size */
  INTERLEAVE_LIST_CFG_0_CHABC_SAD_REG, INTERLEAVE_LIST_CFG_1_CHABC_SAD_REG, INTERLEAVE_LIST_CFG_2_CHABC_SAD_REG,
  INTERLEAVE_LIST_CFG_3_CHABC_SAD_REG, INTERLEAVE_LIST_CFG_4_CHABC_SAD_REG, INTERLEAVE_LIST_CFG_5_CHABC_SAD_REG,
  INTERLEAVE_LIST_CFG_6_CHABC_SAD_REG, INTERLEAVE_LIST_CFG_7_CHABC_SAD_REG, INTERLEAVE_LIST_CFG_8_CHABC_SAD_REG,
  INTERLEAVE_LIST_CFG_9_CHABC_SAD_REG, INTERLEAVE_LIST_CFG_10_CHABC_SAD_REG, INTERLEAVE_LIST_CFG_11_CHABC_SAD_REG,
  INTERLEAVE_LIST_CFG_12_CHABC_SAD_REG, INTERLEAVE_LIST_CFG_13_CHABC_SAD_REG, INTERLEAVE_LIST_CFG_14_CHABC_SAD_REG,
  INTERLEAVE_LIST_CFG_15_CHABC_SAD_REG
};

UINT32 mDramRt0ModeOffset[MaxRtHalves][MAX_MODE_TABLES_IMC] = { /* page-3456. $15.3.84: 4 BYTE size */
  {DRAM_H0_RT0_MODE0_CHABC_SAD_REG, DRAM_H0_RT0_MODE1_CHABC_SAD_REG},
  {DRAM_H1_RT0_MODE0_CHABC_SAD_REG, DRAM_H1_RT0_MODE1_CHABC_SAD_REG}
};

UINT32 mDramRt1ModeOffset[MaxRtHalves][MAX_MODE_TABLES_IMC] = { /* page-3456. $15.3.84: 4 BYTE size */
  {DRAM_H0_RT1_MODE0_CHABC_SAD_REG, DRAM_H0_RT1_MODE1_CHABC_SAD_REG},
  {DRAM_H1_RT1_MODE0_CHABC_SAD_REG, DRAM_H1_RT1_MODE1_CHABC_SAD_REG}
};

UINT32 mDramRt2ModeOffset[MaxRtHalves] = { /* page-3456. $15.3.84: 4 BYTE size */
  DRAM_H0_RT2_MODE0_CHABC_SAD_REG,
  DRAM_H1_RT2_MODE0_CHABC_SAD_REG,
};

UINT32 mDramTgtRouteTable[4][2] = { /* page-3413. $15.2.36: 4 BYTE size */
  {H0_TGT_ROUTE_TABLE_0_CHA_MISC_REG, H1_TGT_ROUTE_TABLE_0_CHA_MISC_REG},
  {H0_TGT_ROUTE_TABLE_1_CHA_MISC_REG, H1_TGT_ROUTE_TABLE_1_CHA_MISC_REG},
  {H0_TGT_ROUTE_TABLE_2_CHA_MISC_REG, H1_TGT_ROUTE_TABLE_2_CHA_MISC_REG},
  {H0_TGT_ROUTE_TABLE_2LM_CHA_MISC_REG, H1_TGT_ROUTE_TABLE_2LM_CHA_MISC_REG}
};

UINT32 mDramChRouteTable[3][2] = { /* page-3413. $15.2.36: 4 BYTE size */
  {H0_CH_ROUTE_TABLE_0_CHA_MISC_REG, H1_CH_ROUTE_TABLE_0_CHA_MISC_REG},
  {H0_CH_ROUTE_TABLE_1_CHA_MISC_REG, H1_CH_ROUTE_TABLE_1_CHA_MISC_REG},
  {H0_CH_ROUTE_TABLE_2_CHA_MISC_REG, H1_CH_ROUTE_TABLE_2_CHA_MISC_REG}
};

UINT32 mMcDramRuleCfgN0[TAD_RULES] = { /* Page-2950 $13.5.16: 8 byte size */
  DRAM_RULE_CFG0_N0_MC_MAIN_REG, DRAM_RULE_CFG1_N0_MC_MAIN_REG, DRAM_RULE_CFG2_N0_MC_MAIN_REG, DRAM_RULE_CFG3_N0_MC_MAIN_REG,
  DRAM_RULE_CFG4_N0_MC_MAIN_REG, DRAM_RULE_CFG5_N0_MC_MAIN_REG, DRAM_RULE_CFG6_N0_MC_MAIN_REG, DRAM_RULE_CFG7_N0_MC_MAIN_REG
};

UINT32 mMcDramRuleCfgN1[TAD_RULES] = { /* Page-2950 $13.5.16: 8 byte size */
  DRAM_RULE_CFG0_N1_MC_MAIN_REG, DRAM_RULE_CFG1_N1_MC_MAIN_REG, DRAM_RULE_CFG2_N1_MC_MAIN_REG, DRAM_RULE_CFG3_N1_MC_MAIN_REG,
  DRAM_RULE_CFG4_N1_MC_MAIN_REG, DRAM_RULE_CFG5_N1_MC_MAIN_REG, DRAM_RULE_CFG6_N1_MC_MAIN_REG, DRAM_RULE_CFG7_N1_MC_MAIN_REG
};


UINT32 mIioSncBaseRegisterOffset[5] = { SNC_BASE_1_IIO_VTD_REG,
                                        SNC_BASE_2_IIO_VTD_REG,
                                        SNC_BASE_3_IIO_VTD_REG,
                                        SNC_BASE_4_IIO_VTD_REG,
                                        SNC_BASE_5_IIO_VTD_REG};

UINT32                  mSocketPresentBitMap;
UINT64                  mMmCfgBase;
UINT64                  mMmCfgLimit;
UINT64                  mPlatGlobalMmiolBase;
UINT64                  mPlatGlobalMmiolLimit;
UINT64                  mPlatGlobalMmiohBase;
UINT64                  mPlatGlobalMmiohLimit;
//UINT8                   mNumOfClusters;
UINT8                   mTotCha;

/* To decrease the number of the PECI PCI register access */
UINT32 mDramRuleCfgChaCache[MAX_SOCKET][MAX_MC_CH][SAD_RULES_10NM];

/**

  This function return the InterleaveBit based on the tgt/Chn granularity

  @param  [in]  Granularity       - MC Target or Channel Granularity

  @retval InterleaveBit

**/

UINT8
EFIAPI
GetInterleaveBit (
  IN UINT8 Granularity
  )
{
  UINT8 InterleaveBit;

  switch (Granularity) {
    case 0:
      InterleaveBit = 6;
      break;
    case 1:
      InterleaveBit = 8;
      break;
    case 2:
      InterleaveBit = 12;
      break;
    case 4:
      InterleaveBit = 7;
      break;
    case 5:
      InterleaveBit = 9;
      break;
    case 6:
      InterleaveBit = 10;
      break;
    case 7:
      InterleaveBit = 11;
      break;
    default:
      InterleaveBit = 0xFF;
      break;
  }

  return InterleaveBit;


}



/**

Validate Parameters passed for Reverse Address Translation

@param  [in]  pTranslatedAddress  - pointer to the structure containing DIMM Address
@param  [out] pTranslatedAddress   -- pointer to the structure containing DIMM Address

@retval EFI_SUCCESS / Error code


**/
EFI_STATUS
EFIAPI
ValidateAddressTranslationParams (
  IN OUT  PTRANSLATED_ADDRESS TA
)
{
  TA->StatusToDsmInterface = 0;

  if (TA->SocketId == 0xFF) {
    TA->StatusToDsmInterface = 0x00010004;
  } else {
    if (TA->SocketId >= MAX_SOCKET) {
      TA->StatusToDsmInterface = 0x00020004;
    }
  }

  if (TA->MemoryControllerId == 0xFF) {
    TA->StatusToDsmInterface = 0x00010005;
  } else {
    if (TA->MemoryControllerId >= MAX_IMC) {
      TA->StatusToDsmInterface = 0x00020005;
    }
  }

  if (TA->ChannelId == 0xFF) {
    TA->StatusToDsmInterface = 0x0001000A;
  } else {
    if (TA->ChannelId >= MAX_MC_CH) {
      TA->StatusToDsmInterface = 0x0002000A;
    }
  }

  if (TA->DimmSlot == 0xFF) {
    TA->StatusToDsmInterface = 0x00010010;
  } else {
    if (TA->DimmSlot >= MAX_DIMM) {
      TA->StatusToDsmInterface = 0x00020010;
    }
  }

  if (TA->StatusToDsmInterface != 0) {
    return EFI_INVALID_PARAMETER;
  }
  return EFI_SUCCESS;
}


/**

Get the DIMM Type and Memory Type for the given DIMM info

@param  [in]  pTranslatedAddress  - pointer to the structure containing DIMM Address
@param  [out] pTranslatedAddress   -- pointer to the structure containing DIMM Address

@retval VOID


**/
VOID
EFIAPI
GetDimmType(
  IN      dimmBDFst *pdimmBdf,
  IN  OUT PTRANSLATED_ADDRESS TA
)
{
  UINT8                             DimmId;
  UINT8                             SktId, McId, ChId, NodeId;
  MCDDRTCFG_MC_MAIN_STRUCT          McDdrtCfg;
  BOOLEAN                           DdrtDimm = FALSE;
  endPointRegInfo reg = {
          .msgType         = MESSAGE_MMIO,
          .addrType        = BAR_32_BITS,
          .optType         = OPTION_DWORD,
          .dimmBdf.cpuType = pdimmBdf->cpuType,
          .dimmBdf.socket  = pdimmBdf->socket,
          .dimmBdf.channel = pdimmBdf->channel,
          .dimmBdf.bus     = pdimmBdf->bus,
          .device          = pdimmBdf->imc + DEVICE_OF_FIRST_IMC,
          .function        = FUNCTION_IMC_MMIO,
          .offset          = MCDDRTCFG_MC_MAIN_REG /* page-2925 $13.4.69: 4 BYTE */
  };

  /* [BIOS] "BoxInst" argument at ReadCpuCsr()
   * 1) BoxInst argument of all MMIO registers except MCDDRTCFG_MC_MAIN_REG
   *    SktCh = (McId * MAX_MC_CH) + ChId;
   * 2) MCDDRTCFG_MC_MAIN_REG
   *    #define NODECHA_TO_SKTCHA(node, ch) (((node%MAX_IMC)*MAX_MC_CH)+ch) // ((node%4)*2)+ch
   *    McDdrtCfg.Data=ReadCpuCsr(,NODECHA_TO_SKTCHA(NodeId, ChId), MCDDRTCFG_MC_MAIN_REG);
   */
  SktId = TA->SocketId;
  McId  = TA->MemoryControllerId;
  ChId  = TA->ChannelId;
  TA->Node = SKTMC_TO_NODE(SktId, McId);
  NodeId = TA->Node;

#ifdef SOCKET_CHANNEL_CHECK
  if(reg.dimmBdf.channel%MAX_MC_CH != pdimmBdf->channel) {
    TDBG("%s() Warning! SktCh: %d, Ch: %d\n", __FUNCTION__, reg.dimmBdf.channel, pdimmBdf->channel);
  } else {
    TDBG("%s() SktCh: %d, Ch: %d\n", __FUNCTION__, NODECHA_TO_SKTCHA(NodeId, ChId) , pdimmBdf->channel);
  }
#endif
  reg.dimmBdf.channel = NODECHA_TO_SKTCHA(NodeId, ChId);
  DimmId = TA->DimmSlot;

  McDdrtCfg.Data = ReadCpuCsrMmio(&reg);

  if ((DimmId == 0) && (McDdrtCfg.Bits.slot0 == 1)) {
    TCRIT("Error: DimmId 0x%x is DDRT\n", DimmId);
    DdrtDimm = TRUE;
  }

  if ((DimmId == 1) && (McDdrtCfg.Bits.slot1 == 1)) {
    TCRIT("Error: DimmId 0x%x is DDRT\n", DimmId);
    DdrtDimm = TRUE;
  }

  if (DdrtDimm) {
    //
    //The Memory Type can be MemType1lmAppDirect or MemType2lmDdrCacheMemoryMode.
    // It will get updated in the later stages of Translation.
    //
    TA->DimmType = ddrtdimmType;
    TA->MemType = MemTypeNone;
    TCRIT("MemType is PMEM or 2LM - ddrtdimmType\n");
  }
  else {
    TA->DimmType = ddr4dimmType;
    TA->MemType = MemType1lmDdr;
    TDBG("MemType is 1LM - ddr4dimmType\n");
  }
}


/**

Get SystemAddress from ChannelAddress

@param  [in]  pTranslatedAddress  -- pointer to the structure containing DIMM Address
@param  [out] pTranslatedAddress  -- pointer to the structure containing DIMM Address

@retval EFI_SUCCESS                 SystemAddress is built successfully
@retval EFI_INVALID_PARAMETER       SystemAddress is not built successfully


**/

EFI_STATUS
EFIAPI
GetSystemAddress(
  IN dimmBDFst *pdimmBdf,
  IN PTRANSLATED_ADDRESS TA
)
{

  TADCHNILVOFFSET_0_N0_MC_MAIN_STRUCT         TadChnIlvData0;
  TADCHNILVOFFSET_0_N1_MC_MAIN_STRUCT         TadChnIlvData1;
  TAD_RD_N0_M2MEM_MAIN_STRUCT                 MeshN0TadRd;
  TAD_RD_N1_M2MEM_MAIN_STRUCT                 MeshN1TadRd;
  MIRRORCHNLS_M2MEM_MAIN_STRUCT               MirrorChannels;
  UINT64                                      ChannelAddress;
  UINT64                                      TadOffset;
  UINT64                                      SystemAddress;
  UINT64                                      PrevSad2TadLimit;
  UINT64                                      Sad2TadLimit;
  UINT64                                      LowerAddrBits;
  UINT8                                       SktId;
  UINT8                                       McId;
  UINT8                                       ChId;
  UINT8                                       PriCh;
  UINT8                                       SktCh;
  UINT8                                       PriSecBit;
  UINT8                                       Index;
  UINT8                                       TadIndex;
  UINT8                                       TgtInterleaveBit;
  UINT8                                       ChInterleaveBit;
  UINT8                                       TgtWays;
  UINT8                                       ChWays;
  UINT8                                       TargetLid;
  UINT8                                       ChannelLid;
  UINT8                                       ExpectedSktId;
  UINT8                                       ExpectedMcId;
  UINT8                                       ExpectedChId;
  BOOLEAN                                     Match;
  TRANSLATED_DIMM_TYPE                        DimmType;
  EFI_STATUS                                  Status;

  UINT32 TadChnIlvOffsetN0[MAX_TAD_RULES_10NM] = {
    TADCHNILVOFFSET_0_N0_MC_MAIN_REG, TADCHNILVOFFSET_1_N0_MC_MAIN_REG,
	TADCHNILVOFFSET_2_N0_MC_MAIN_REG, TADCHNILVOFFSET_3_N0_MC_MAIN_REG,
    TADCHNILVOFFSET_4_N0_MC_MAIN_REG, TADCHNILVOFFSET_5_N0_MC_MAIN_REG,
	TADCHNILVOFFSET_6_N0_MC_MAIN_REG, TADCHNILVOFFSET_7_N0_MC_MAIN_REG,
    TADCHNILVOFFSET_8_N0_MC_MAIN_REG, TADCHNILVOFFSET_9_N0_MC_MAIN_REG,
	TADCHNILVOFFSET_10_N0_MC_MAIN_REG, TADCHNILVOFFSET_11_N0_MC_MAIN_REG
  };

  UINT32 TadChnIlvOffsetN1[MAX_TAD_RULES_10NM] = {
    TADCHNILVOFFSET_0_N1_MC_MAIN_REG, TADCHNILVOFFSET_1_N1_MC_MAIN_REG,
	TADCHNILVOFFSET_2_N1_MC_MAIN_REG, TADCHNILVOFFSET_3_N1_MC_MAIN_REG,
    TADCHNILVOFFSET_4_N1_MC_MAIN_REG, TADCHNILVOFFSET_5_N1_MC_MAIN_REG,
	TADCHNILVOFFSET_6_N1_MC_MAIN_REG, TADCHNILVOFFSET_7_N1_MC_MAIN_REG,
    TADCHNILVOFFSET_8_N1_MC_MAIN_REG, TADCHNILVOFFSET_9_N1_MC_MAIN_REG,
	TADCHNILVOFFSET_10_N1_MC_MAIN_REG, TADCHNILVOFFSET_11_N1_MC_MAIN_REG
  };

  endPointRegInfo reg_pci = {
          .msgType         = MESSAGE_LOCAL_PCI,
          .addrType        = PCI_BDF_REG,
          .optType         = OPTION_DWORD/*OPTION_QWORD*/,
          .dimmBdf.cpuType = pdimmBdf->cpuType,
          .dimmBdf.socket  = pdimmBdf->socket,
          .dimmBdf.channel = pdimmBdf->channel,
          .dimmBdf.bus     = BUS_NUMBER_PCI30,
          .device          = pdimmBdf->imc + DEVICE_IMC_PCI, /* page-2718 $13.1.43 */
          .function        = 0,
          .offset          = TAD_RD_N0_M2MEM_MAIN_REG /* peciapp: KO @ -r9 (8 byte) , OK @ -r5 (4 byte) */
  };

  endPointRegInfo reg_mmio = {
          .msgType         = MESSAGE_MMIO,
          .addrType        = BAR_32_BITS,
          .optType         = OPTION_DWORD,
		                  /* N0: lower 32 bits, N1: upper 32 bits => 64 bits
                           * Compare EDS V-2 $13.4.7 to TADCHNILVOFFSET_0_N0/1_MC_MAIN_STRUCT */
          .dimmBdf.cpuType = pdimmBdf->cpuType,
          .dimmBdf.socket  = pdimmBdf->socket,
          .dimmBdf.channel = pdimmBdf->channel,
          .dimmBdf.bus     = pdimmBdf->bus,
          .device          = pdimmBdf->imc + DEVICE_OF_FIRST_IMC,
          .function        = FUNCTION_IMC_MMIO,
          .offset          = TADCHNILVOFFSET_0_N0_MC_MAIN_REG /* Page-2898 $13.4.7: 8 byte */
  };
  dimmBDFst dimmBdf;

  SktId     = TA->SocketId;
  McId      = TA->MemoryControllerId;
  ChId      = TA->ChannelId;
  Match     = FALSE;
  PrevSad2TadLimit = 0;
  ChannelAddress   = TA->ChannelAddress;
  DimmType = TA->DimmType;
  ChWays = 0;
  PriCh  = ChId;
  PriSecBit = 0;

  for (Index = 0; Index < INTERNAL_TAD_RULES; Index++) {
    //
    // Read Internal Tad Table
    //
    MeshN0TadRd.Data = 0;
    MeshN0TadRd.Bits.tadid = Index;
    MeshN0TadRd.Bits.tadrden = 1;

    reg_pci.offset   = TAD_RD_N0_M2MEM_MAIN_REG;
    WriteCpuCsrPciLocal(&reg_pci, MeshN0TadRd.Data); /* SktId, McId */
    MeshN0TadRd.Data = ReadCpuCsrPciLocal(&reg_pci); /* SktId, McId */
    reg_pci.offset   = TAD_RD_N1_M2MEM_MAIN_REG;
    MeshN1TadRd.Data = ReadCpuCsrPciLocal(&reg_pci); /* SktId, McId */
    TDBG("%s(): INTERNAL TAD index = %d", __FUNCTION__, Index);
    TDBG("[PECI] - TAD_RD_N0_M2MEM_MAIN_REG: 0x%x", MeshN0TadRd.Data);
    TDBG("[PECI] - TAD_RD_N1_M2MEM_MAIN_REG: 0x%x", MeshN1TadRd.Data);
    if (!(MeshN0TadRd.Bits.tadvld)) {
      TCRIT("There is no valid TAD for SadIndex %d\n", Index);
      break;
    }

    Sad2TadLimit = LShiftU64 ((LShiftU64 (MeshN1TadRd.Bits.addresslimit, 6) | MeshN0TadRd.Bits.addresslimit) + 1, SAD_UNIT);
    TDBG("PrevSad2TadLimit is 0x%llx for Tad 0x%x", PrevSad2TadLimit, Index);
    TDBG("Sad2TadLimit is 0x%llx for Tad 0x%x\n", Sad2TadLimit, Index);

    if (((DimmType == ddr4dimmType) && !(MeshN0TadRd.Bits.ddr4)) || ((DimmType == ddrtdimmType) && (MeshN0TadRd.Bits.ddr4))) {
      TDBG("DimmType doesn't match to memory type from TAD Rule\n");
      PrevSad2TadLimit = Sad2TadLimit;
      continue;
    }

    if (PrevSad2TadLimit == Sad2TadLimit) {
      TCRIT("System Address cannot be calculated\n");
      break;
    }

    ChannelAddress   = TA->ChannelAddress;
    if ((DimmType == ddr4dimmType) && (MeshN0TadRd.Bits.mirror)) {
      LowerAddrBits = BitFieldRead64 (ChannelAddress, 0, 17);
      ChannelAddress = RShiftU64 (ChannelAddress, 18);
      PriSecBit = ChannelAddress & BIT0;
      ChannelAddress = RShiftU64 (ChannelAddress, 1);
      ChannelAddress = LShiftU64 (ChannelAddress, 18) | LowerAddrBits;
      TDBG("Mirroring is enabled, ChannelAddress after takingout Bit 18 0x%llx\n", ChannelAddress);

      if (PriSecBit) {
        reg_pci.offset = MIRRORCHNLS_M2MEM_MAIN_REG; // b 30, d 12, f 0
        MirrorChannels.Data = ReadCpuCsrPciLocal(&reg_pci);
        TDBG("[PECI] - MIRRORCHNLS_M2MEM_MAIN_REG: 0x%x", MirrorChannels.Data);

        if (ChId == (UINT8)MirrorChannels.Bits.ddr4chnl0secondary)  {
          PriCh = 0;
        } else {
          if (ChId == (UINT8)MirrorChannels.Bits.ddr4chnl1secondary)  {
            PriCh = 1;
          } else {
             TCRIT("System Address cannot be calculated, can't get primary channel for given secondary channel\n");
             return EFI_INVALID_PARAMETER;
          }
        }
        TDBG("Given Channel 0x%x is Secondary Channel, Primary Channel is 0x%lx\n", ChId, PriCh);
      } else {
        PriCh = ChId;
      }
    }

    TadIndex =  (UINT8)MeshN0TadRd.Bits.ddrtadid;
    SktCh    = (McId * MAX_MC_CH) + PriCh;
#ifdef SOCKET_CHANNEL_CHECK
    TDBG("%s() McId:%d, PriCh:%d, SktCh: %d\n", __FUNCTION__, McId,PriCh,SktCh);
#endif
    reg_mmio.dimmBdf.channel = SktCh;
    memcpy(&dimmBdf, pdimmBdf, sizeof(dimmBDFst));
    dimmBdf.channel     = SktCh;

    reg_mmio.offset     = TadChnIlvOffsetN0[TadIndex]; /* page-2898 $13.4.7 */
    TadChnIlvData0.Data = ReadCpuCsrMmio(&reg_mmio);
    reg_mmio.offset     = TadChnIlvOffsetN1[TadIndex];
    TadChnIlvData1.Data = ReadCpuCsrMmio(&reg_mmio);
    TDBG("[PECI] - TadChnIlvOffsetN0[%d] = 0x%x", TadIndex, TadChnIlvData0.Data);
    TDBG("[PECI] - TadChnIlvOffsetN1[%d] = 0x%x", TadIndex, TadChnIlvData1.Data);

    TadOffset = LShiftU64 ((LShiftU64 (TadChnIlvData1.Bits.tad_offset, 18) | TadChnIlvData0.Bits.tad_offset), SAD_UNIT);
    TDBG("TadOffset is 0x%llx for Tad 0x%x", TadOffset, TadIndex);

    TgtInterleaveBit = GetInterleaveBit ((UINT8)TadChnIlvData0.Bits.target_gran);
    ChInterleaveBit =  GetInterleaveBit ((UINT8)TadChnIlvData0.Bits.chn_gran);
    TgtWays = (UINT8) (1 << TadChnIlvData0.Bits.target_ways);

    switch (TadChnIlvData0.Bits.chn_ways) {
      case 0: ChWays = 1; break;
      case 1: ChWays = 2; break;
      case 2: ChWays = 3; break;
      case 3: ChWays = 8; break;
      case 4: ChWays = 4; break;
      default: break;
    }

    TDBG("TgtInterleaveBit:%x , ChInterleaveBit:0x%x", TgtInterleaveBit, ChInterleaveBit);
    TDBG("TgtWays:0x%x, ChWays:0x%x\n", TgtWays, ChWays);

    //
    // Try all posibilities for Target_LID and Channel_LID until we find a match
    //

    for (TargetLid = 0 ; TargetLid < TgtWays; TargetLid++) {
      for (ChannelLid = 0; ChannelLid < ChWays; ChannelLid++) {
        TDBG("Trying LogicalId combination for Target%x Channel %x", TargetLid, ChannelLid);

        if ((ChInterleaveBit <= TgtInterleaveBit) && (ChWays != 3)) {
          // Add in Channel Bits
          LowerAddrBits = BitFieldRead64 (ChannelAddress, 0, ChInterleaveBit-1);
          SystemAddress = RShiftU64 (ChannelAddress, ChInterleaveBit);
          SystemAddress = SystemAddress * ChWays;
          SystemAddress += (UINT64)ChannelLid;
          SystemAddress = LShiftU64 (SystemAddress, ChInterleaveBit) | LowerAddrBits;

          // Add in Target Bits
          LowerAddrBits = BitFieldRead64 (SystemAddress, 0, TgtInterleaveBit-1);
          SystemAddress = RShiftU64 (SystemAddress, TgtInterleaveBit);
          SystemAddress = SystemAddress * TgtWays;
          SystemAddress += (UINT64)TargetLid;
          SystemAddress = LShiftU64 (SystemAddress, TgtInterleaveBit) | LowerAddrBits;
        } else {
          // Add in Target Bits
          LowerAddrBits = BitFieldRead64 (ChannelAddress, 0, TgtInterleaveBit-1);
          SystemAddress = RShiftU64 (ChannelAddress, TgtInterleaveBit);
          SystemAddress = SystemAddress * TgtWays;
          SystemAddress += (UINT64)TargetLid;
          SystemAddress = LShiftU64 (SystemAddress, TgtInterleaveBit) | LowerAddrBits;

          // Add in Channel Bits
          LowerAddrBits = BitFieldRead64 (SystemAddress, 0, ChInterleaveBit-1);
          SystemAddress = RShiftU64 (SystemAddress, ChInterleaveBit);
          SystemAddress = SystemAddress * ChWays;
          SystemAddress += (UINT64)ChannelLid;
          SystemAddress = LShiftU64 (SystemAddress, ChInterleaveBit) | LowerAddrBits;
        }

        if (TadChnIlvData1.Bits.tad_offset_sign == 0) {
          SystemAddress += TadOffset;
          TDBG("SystemAddress after adding tadoffset 0x%llx\n", SystemAddress);
        } else {
          SystemAddress -= TadOffset;
          TDBG("SystemAddress after subtracting tadoffset =0x%llx\n", SystemAddress);
        }

        TDBG("%s() PrevSad2TadLimit 0x%llx, SystemAddress 0x%llx, Sad2TadLimit 0x%llx\n",
        		__FUNCTION__, PrevSad2TadLimit, SystemAddress, Sad2TadLimit);
        if ((PrevSad2TadLimit <= SystemAddress) && (SystemAddress < Sad2TadLimit)) {
          TDBG("SystemAddress 0x%llx falls in the limit with TargetLid %x and TargetChId %x", SystemAddress, TargetLid, ChannelLid);

          // Set Potential System Address into TA
          TA->SystemAddress = SystemAddress;
          Status = TranslateSad (&dimmBdf, TA, &ExpectedSktId, &ExpectedMcId, &ExpectedChId);

          if (Status != EFI_SUCCESS) {
            continue;
          }

          if ((ExpectedSktId == SktId) && (ExpectedMcId == McId) && (ExpectedChId == PriCh)) {
        	TDBG("SystemAddress 0x%llx found, SktId, McId and ChId from TranslateSad matches", SystemAddress);
            TA->DimmType = DimmType;
            Match = TRUE;
            break;
          } else {
            TDBG("SktId, McId and ChId from TranslateSad doesn't match for SystemAddress 0x%llx.. trying another combination", SystemAddress);
            continue;
          }
        } else {
          TDBG("SystemAddress 0x%llx doesn't fall in the limit for TargetLid %x and TargetChId %x\n", SystemAddress, TargetLid, ChannelLid);
          break;
        }
      } // Target ChId

      if (Match == TRUE) {
      break;
    }

    } // Target McId

    if (Match == TRUE) {
      break;
    }

    PrevSad2TadLimit = Sad2TadLimit;

  } // Sad2Tad

  if (Match == FALSE) {
    TCRIT("SystemAddress cannot be calculated\n");
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**

Get the ChannelAddress from RankAddress

@param  [in]  pTranslatedAddress  -- pointer to the structure containing DIMM Address
@param  [out] pTranslatedAddress  -- pointer to the structure containing DIMM Address

@retval EFI_SUCCESS                 RankAddress is built successfully
@retval EFI_INVALID_PARAMETER       RankAddress is not built successfully


**/

EFI_STATUS
EFIAPI
GetChannelAddress(
  IN dimmBDFst *pdimmBdf,
  IN PTRANSLATED_ADDRESS TA
)
{
  UINT64    RankAddress;
  UINT64    ChannelAddress;
  UINT64    RirOffset;
  UINT64    PrevLimit;
  UINT64    RirLimit;
  UINT64    LowerAddrBits;
  UINT8     SktId;
  UINT8     McId;
  UINT8     ChId;
  UINT8     SktCh;
  UINT8     ChipSelect;
  UINT8     RirIndex;
  UINT8     RirWays;
  UINT8     Index;
  UINT8     ShiftBit;
  BOOLEAN   Match;
  UINT32    ClosePg;

  RIRWAYNESSLIMIT_0_MC_MAIN_STRUCT  RirWayness;
  RIRILV0OFFSET_0_MC_MAIN_STRUCT    RirIlv0Offset;
  FMRIRWAYNESSLIMIT_0_MC_2LM_STRUCT FmRirWayness;
  FMRIRILV0OFFSET_0_MC_2LM_STRUCT   FmRirIlv0Offset;

  UINT32 RirWaynessLimit[MAX_RIR] =
    { RIRWAYNESSLIMIT_0_MC_MAIN_REG, RIRWAYNESSLIMIT_1_MC_MAIN_REG,
      RIRWAYNESSLIMIT_2_MC_MAIN_REG, RIRWAYNESSLIMIT_3_MC_MAIN_REG }; /* page-2912. $13.4.19: 4 BYTE size */

  UINT32 RirIlvOffset[MAX_RIR][MAX_RIR_WAYS] =    {
      { RIRILV0OFFSET_0_MC_MAIN_REG, RIRILV1OFFSET_0_MC_MAIN_REG, RIRILV2OFFSET_0_MC_MAIN_REG,
    		RIRILV3OFFSET_0_MC_MAIN_REG, RIRILV4OFFSET_0_MC_MAIN_REG, RIRILV5OFFSET_0_MC_MAIN_REG,
			RIRILV6OFFSET_0_MC_MAIN_REG, RIRILV7OFFSET_0_MC_MAIN_REG },
      { RIRILV0OFFSET_1_MC_MAIN_REG, RIRILV1OFFSET_1_MC_MAIN_REG, RIRILV2OFFSET_1_MC_MAIN_REG,
    		RIRILV3OFFSET_1_MC_MAIN_REG, RIRILV4OFFSET_1_MC_MAIN_REG, RIRILV5OFFSET_1_MC_MAIN_REG,
			RIRILV6OFFSET_1_MC_MAIN_REG, RIRILV7OFFSET_1_MC_MAIN_REG },
      { RIRILV0OFFSET_2_MC_MAIN_REG, RIRILV1OFFSET_2_MC_MAIN_REG, RIRILV2OFFSET_2_MC_MAIN_REG,
    		RIRILV3OFFSET_2_MC_MAIN_REG, RIRILV4OFFSET_2_MC_MAIN_REG, RIRILV5OFFSET_2_MC_MAIN_REG,
			RIRILV6OFFSET_2_MC_MAIN_REG, RIRILV7OFFSET_2_MC_MAIN_REG },
      { RIRILV0OFFSET_3_MC_MAIN_REG, RIRILV1OFFSET_3_MC_MAIN_REG, RIRILV2OFFSET_3_MC_MAIN_REG,
    		RIRILV3OFFSET_3_MC_MAIN_REG, RIRILV4OFFSET_3_MC_MAIN_REG, RIRILV5OFFSET_3_MC_MAIN_REG,
			RIRILV6OFFSET_3_MC_MAIN_REG, RIRILV7OFFSET_3_MC_MAIN_REG } /* page-2913. $13.4.23: 4 BYTE */
  };

  UINT32 FmRirWaynessLimit[MAX_RIR_DDRT] =
    { FMRIRWAYNESSLIMIT_0_MC_2LM_REG, FMRIRWAYNESSLIMIT_1_MC_2LM_REG,
      FMRIRWAYNESSLIMIT_2_MC_2LM_REG, FMRIRWAYNESSLIMIT_3_MC_2LM_REG }; /* page-2801. $13.2.13: 4 BYTE */

  UINT32 FmRirIlvOffset[MAX_RIR_DDRT][MAX_RIR_DDRT_WAYS] = {
    { FMRIRILV0OFFSET_0_MC_2LM_REG, FMRIRILV1OFFSET_0_MC_2LM_REG },
    { FMRIRILV0OFFSET_1_MC_2LM_REG, FMRIRILV1OFFSET_1_MC_2LM_REG },
    { FMRIRILV0OFFSET_2_MC_2LM_REG, FMRIRILV1OFFSET_2_MC_2LM_REG },
    { FMRIRILV0OFFSET_3_MC_2LM_REG, FMRIRILV1OFFSET_3_MC_2LM_REG } /* page-2802. $13.2.17: 4 BYTE */
  };

  endPointRegInfo reg = {
          .msgType         = MESSAGE_MMIO,
          .addrType        = BAR_32_BITS,
          .optType         = OPTION_DWORD,
          .dimmBdf.cpuType = pdimmBdf->cpuType,
          .dimmBdf.socket  = pdimmBdf->socket,
          .dimmBdf.channel = pdimmBdf->channel,
          .dimmBdf.bus     = pdimmBdf->bus,
          .device          = pdimmBdf->imc + DEVICE_OF_FIRST_IMC,
          .function        = FUNCTION_IMC_MMIO,
          .offset          = RIRWAYNESSLIMIT_0_MC_MAIN_REG /* Page-2912 $13.4.19: 4 byte */
  };
  dimmBDFst dimmBdf;

  SktId         = TA->SocketId;
  McId          = TA->MemoryControllerId;
  ChId          = TA->ChannelId;
  RankAddress   = TA->RankAddress;
  ChipSelect    = (TA->DimmSlot * 4) + TA->PhysicalRankId;
  ChannelAddress= (UINT64) -1;
  Match         = FALSE;
  SktCh         = (McId * MAX_MC_CH) + ChId;
  PrevLimit = 0;

#ifdef SOCKET_CHANNEL_CHECK
  if(SktCh%MAX_MC_CH != pdimmBdf->channel) {
    TDBG("%s() Warning! SktCh: %d, Ch: %d\n", __FUNCTION__, SktCh, pdimmBdf->channel);
  } else {
    TDBG("%s() SktCh: %d, Ch: %d\n", __FUNCTION__, SktCh, pdimmBdf->channel);
  }
#endif
  //Read McMtr Register to get the page mode to determine the no. of Lower order bits that are not touched by interleave
  reg.dimmBdf.channel = SktCh;
  memcpy(&dimmBdf, pdimmBdf, sizeof(dimmBDFst));
  dimmBdf.channel     = SktCh;
  GetMcmtr (&dimmBdf, SktId, McId, 0, &ClosePg, NULL, NULL, NULL); //Socket, Mc, ChOnSkt, mcmtr.close_page, mcmtr.bank_xor_enable

  if (ClosePg) {
    ShiftBit = 6;
  } else {
    ShiftBit = 13;
  }

  if (TA->DimmType == ddr4dimmType) {
    for (RirIndex = 0; RirIndex < MAX_RIR; RirIndex++) {
      reg.offset = RirWaynessLimit[RirIndex]; // func 1
      RirWayness.Data = ReadCpuCsrMmio(&reg);
      RirLimit = (((UINT64) RirWayness.Bits.rir_limit + 1) << RIR_UNIT) - 1;
      RirWays = 1 << RirWayness.Bits.rir_way;
      //TDBG("RIRLimit is 0x%x\n\n",RirLimit);
      //TDBG("RIRWays is 0x%x\n\n",RirWays);
      //TDBG("PrevLimit is 0x%x\n\n",PrevLimit);

      for (Index = 0; Index < RirWays; Index++) {
        reg.offset = RirIlvOffset[RirIndex][Index]; // func 1
        RirIlv0Offset.Data = ReadCpuCsrMmio(&reg);
        //TDBG("RIRIlv0Offset.Data is 0x%x\n",RirIlv0Offset.Data);

        if (RirIlv0Offset.Bits.rir_rnk_tgt0 == ChipSelect) {
          //TDBG("RirIndex is 0x%x\n",RirIndex);
          //TDBG("Index is 0x%x\n",Index);

          RirOffset = ((UINT64)(RirIlv0Offset.Bits.rir_offset0) << 26);
          //TDBG("RirOffset is 0x%x\n",RirOffset);
          ChannelAddress = RankAddress + RirOffset;
          LowerAddrBits = BitFieldRead64 (RankAddress, 0, ShiftBit-1);
          ChannelAddress = ChannelAddress >> ShiftBit;
          ChannelAddress = (ChannelAddress * RirWays) | Index;
          ChannelAddress = (ChannelAddress << ShiftBit) | LowerAddrBits;
          //TDBG("ChannelAddress is 0x%lx\n",ChannelAddress);

          if (((PrevLimit == 0) && (PrevLimit == ChannelAddress) && (ChannelAddress <= RirLimit)) ||
              ((PrevLimit < ChannelAddress) && (ChannelAddress <= RirLimit))) {
            //TDBG("ChannelAddress Found 0x%lx\n",ChannelAddress);
            Match = TRUE;
            break;
          }  // if Address within the limit
        } // if Physical Rank Target match
      } // for Rir ways

      if (Match ) {
        break;
      } else{
        PrevLimit = RirLimit;
      }
    } // for Max RIR

    if (RirIndex == MAX_RIR) {
      TCRIT("Address 0x%llx doesn't fit any of RIR Registers\n",TA->RankAddress);
      return EFI_INVALID_PARAMETER;
    }

  } else {
    for (RirIndex = 0; RirIndex < MAX_RIR_DDRT; RirIndex++) {
      reg.offset = FmRirWaynessLimit[RirIndex]; // func 1
      FmRirWayness.Data = ReadCpuCsrMmio(&reg);
      RirLimit = (((UINT64) FmRirWayness.Bits.rir_limit + 1) << RIR_UNIT) - 1;
      RirWays = 1 << FmRirWayness.Bits.rir_way;
      //TDBG("RIRLimit is 0x%x\n\n",RirLimit);
      //TDBG("RIRWays is 0x%x\n\n",RirWays);
      //TDBG("PrevLimit is 0x%x\n\n",PrevLimit);

      for (Index = 0; Index < RirWays; Index++) {
        reg.offset = FmRirIlvOffset[RirIndex][Index]; // func 1
        FmRirIlv0Offset.Data = ReadCpuCsrMmio(&reg);
        //TDBG("RIRIlv0Offset.Data is 0x%x\n",FmRirIlv0Offset.Data);

        if (FmRirIlv0Offset.Bits.rir_rnk_tgt0 == ChipSelect) {
          //TDBG("RirIndex is 0x%x\n",RirIndex);
          //TDBG("Index is 0x%x\n",Index);

          RirOffset = ((UINT64)(FmRirIlv0Offset.Bits.rir_offset0) << 26);
          //TDBG("RirOffset is 0x%x\n",RirOffset);
          ChannelAddress = RankAddress + RirOffset;
          LowerAddrBits = BitFieldRead64 (RankAddress, 0, ShiftBit-1);
          ChannelAddress = ChannelAddress >> ShiftBit;
          ChannelAddress = (ChannelAddress * RirWays) | Index;
          ChannelAddress = (ChannelAddress << ShiftBit) | LowerAddrBits;
          //TDBG("ChannelAddress is 0x%lx\n",ChannelAddress);

          if (((PrevLimit == 0) && (PrevLimit == ChannelAddress) && (ChannelAddress <= RirLimit)) ||
              ((PrevLimit < ChannelAddress) && (ChannelAddress <= RirLimit))) {
            //TDBG("ChannelAddress Found 0x%lx\n",ChannelAddress);
            Match = TRUE;
            break;
          }  // if Address within the limit
        } //if Physical Rank Target match
      } // for Rir ways

      if (Match ) {
        break;
      } else{
        PrevLimit = RirLimit;
      }
    } // for Max RIR
    if (RirIndex == MAX_RIR) {
      TCRIT("Address 0x%llx doesn't fit any of RIR Registers\n",TA->RankAddress);
      return EFI_INVALID_PARAMETER;
    }
  } // if DDRT DIMM


  TA->ChannelAddress = ChannelAddress;
  return EFI_SUCCESS;
}
/**

Get the RankAddress from Dimm Info

@param  [in]  pTranslatedAddress  --pointer to the structure containing DIMM Address
@param  [out] pTranslatedAddress   -- pointer to the structure containing DIMM Address

@retval EFI_SUCCESS                 RankAddress is built successfully
@retval EFI_INVALID_PARAMETER       RankAddress is not built successfully


**/

EFI_STATUS
EFIAPI
GetRankAddress(
  IN dimmBDFst *pdimmBdf,
  IN PTRANSLATED_ADDRESS TA
)
{
  UINT8   SktId, McId, ChId, DimmId, Bank, BankGroup, ChipId, SktCh;
  UINT8   TempBankGroup, TempBank;
  UINT8   LowPos, HighPos;
  UINT32  ClosePg, BankXorEn;
  UINT32  Row, Col;
  UINT64  RankAddress;
  DIMMMTR_0_MC_MAIN_STRUCT          DimmMtrReg;
  AMAP_MC_MAIN_STRUCT               AmapReg;

  UINT32 DimmMtrOffset[MAX_DIMM] =
    {DIMMMTR_0_MC_MAIN_REG, DIMMMTR_1_MC_MAIN_REG};

  endPointRegInfo reg = {
          .msgType         = MESSAGE_MMIO,
          .addrType        = BAR_32_BITS,
          .optType         = OPTION_DWORD,
          .dimmBdf.cpuType = pdimmBdf->cpuType,
          .dimmBdf.socket  = pdimmBdf->socket,
          .dimmBdf.channel = pdimmBdf->channel,
          .dimmBdf.bus     = pdimmBdf->bus,
          .device          = pdimmBdf->imc + DEVICE_OF_FIRST_IMC,
          .function        = FUNCTION_IMC_MMIO,
          .offset          = DIMMMTR_0_MC_MAIN_REG /* Page-2892 $13.4.2: 4 byte */
  };
  dimmBDFst dimmBdf;

  SktId = TA->SocketId;
  McId = TA->MemoryControllerId;
  ChId = TA->ChannelId;
  DimmId = TA->DimmSlot;
  Bank = TA->Bank;
  BankGroup = TA->BankGroup;
  Row = TA->Row;
  Col = TA->Col;
  ChipId = TA->ChipId;
  LowPos = 30;
  HighPos = 31;
  TempBankGroup = 0;
  TempBank = 0;
  RankAddress = 0;

  if (TA->DimmType == ddr4dimmType) {
    TDBG("%s() Skt %d Bus %d Mc %d Ch %d Dimm %d Bg %d B %d Chip %d R 0x%x C 0x%x",
    	__FUNCTION__, SktId, pdimmBdf->bus, McId, ChId, DimmId, BankGroup, Bank, ChipId, Row, Col);

    //
    //do RA -> SA translation when col/row/bank == -1.
    //
    if ((Col == (UINT32) -1) &&
        (Row == (UINT32) -1) &&
        (Bank == (UINT8) -1) &&
        (TA->RankAddress != (UINT64) -1)) {
      TCRIT("GetRankAddress: RankAddress for DDR4:0x%llx", TA->RankAddress);
      return EFI_SUCCESS;
    }

    if (TA->BankGroup == 0xFF) {
      TA->StatusToDsmInterface = 0x00010016;
    } else {
      if (TA->BankGroup >= 4) {
        TA->StatusToDsmInterface = 0x000200016;
      }
    }

    if (TA->Bank == 0xFF) {
      TA->StatusToDsmInterface = 0x00010015;
    } else {
      if (TA->Bank >= 4) {
        TA->StatusToDsmInterface = 0x00020015;
      }
    }

    //
    // Col is Max 10 bits
    //
    if (TA->Col == (UINT32)-1) {
      TA->StatusToDsmInterface = 0x00010014;
    } else {
      if (TA->Col > 0x7FF) {
        TA->StatusToDsmInterface = 0x00020014;
      }
    }

    //
    // Row is Max 18 bits
    //
    if (TA->Row == (UINT32)-1) {
      TA->StatusToDsmInterface = 0x00010013;
    } else {
      if (TA->Row > 0x3FFFF) {
        TA->StatusToDsmInterface = 0x00020013;
      }
    }

    if (TA->StatusToDsmInterface != 0) {
      TCRIT("Bank or BankGroup or Row or Col is not valid for Translation: :0x%lx\n", TA->StatusToDsmInterface);
      return EFI_INVALID_PARAMETER;
    }

    SktCh = (McId * MAX_MC_CH) + ChId;
    memcpy(&dimmBdf, pdimmBdf, sizeof(dimmBDFst));
    dimmBdf.channel     = SktCh;
    reg.dimmBdf.channel = SktCh;

    //
    // Read MCMTR register for Page Policy and Bank_XOR Enable
    //
    GetMcmtr (&dimmBdf, SktId, McId, SktCh, &ClosePg, &BankXorEn, NULL, NULL); //Socket, Mc, ChOnSkt, mcmtr.close_page, mcmtr.bank_xor_enable

    //
    // Read DIMM MTR Register for MaxRowBits, MaxColBits
    //
    reg.offset = DimmMtrOffset[DimmId]; // func 1
    DimmMtrReg.Data = ReadCpuCsrMmio(&reg);

    // Read AMAP Register
    reg.offset = AMAP_MC_MAIN_REG; /* page-2895 $13.4.4. func 1 */
    AmapReg.Data = ReadCpuCsrMmio(&reg);
    TDBG("BDF %d %d %d: DimmMtrOffset[%d] 0x%x, AmapReg 0x%x", reg.dimmBdf.bus, reg.device, reg.function, DimmId, DimmMtrReg.Data, AmapReg.Data);

    //
    //Deal with Chip_id
    //
    if (DimmMtrReg.Bits.ddr4_3dsnumranks_cs != 0) {
      RankAddress = BitFieldWrite64 (RankAddress, 31, 31, BitFieldRead32 (Row, 0, 0));

      if (DimmMtrReg.Bits.ddr4_3dsnumranks_cs > 1){
        RankAddress = BitFieldWrite64 (RankAddress, 32, 32, BitFieldRead32 (Row, 1, 1));
      }

      if (DimmMtrReg.Bits.ddr4_3dsnumranks_cs > 2){
        RankAddress = BitFieldWrite64 (RankAddress, 33, 33, BitFieldRead32 (Row, 2, 2));
      }
    }

    if (ClosePg == 1) {
      //
      // Add Bank and BankGroup
      //
      if (BankXorEn == 1) {
        TempBankGroup = (UINT8) (BitFieldRead32 (Row, 3, 4));
        TempBank = (UINT8)(BitFieldRead32(Row, 5, 5));
        TempBank |= (UINT8) (LShiftU64 (BitFieldRead64 (Row, 10, 10) , 1));
      }
      TempBankGroup ^= BankGroup;
      TempBank ^= Bank;
      TDBG("TempBankGroup = %d, TempBank=%d @ ClosePg", TempBankGroup, TempBank);

      RankAddress = BitFieldWrite64 (RankAddress, 6, 6, BitFieldRead32 (TempBankGroup, 0, 0));
      if (!(DimmMtrReg.Bits.ba_shrink)) {
        RankAddress = BitFieldWrite64 (RankAddress, 7, 7, BitFieldRead32 (TempBankGroup, 1, 1));
      }

      RankAddress = BitFieldWrite64 (RankAddress, 8, 9, BitFieldRead32 (TempBank, 0, 1));

      //
      // Add the column
      //
      RankAddress = BitFieldWrite64 (RankAddress, 3, 5, BitFieldRead32 (Col, 0, 2));
      RankAddress = BitFieldWrite64 (RankAddress, 19, 19, BitFieldRead32 (Col, 3, 3));
      RankAddress = BitFieldWrite64 (RankAddress, 14, 14, BitFieldRead32 (Col, 4, 4));
      RankAddress = BitFieldWrite64 (RankAddress, 23, 27, BitFieldRead32 (Col, 5, 9));


      //
      // Add the row and chip_id
      //
      if (DimmMtrReg.Bits.ddr4_3dsnumranks_cs == 0) {
        RankAddress = BitFieldWrite64 (RankAddress, 17, 17, BitFieldRead32 (Row, 0, 0));
      } else {
        RankAddress = BitFieldWrite64 (RankAddress, 17, 17, BitFieldRead32 (ChipId, 0, 0));
      }

      if (DimmMtrReg.Bits.ddr4_3dsnumranks_cs > 1) {
        RankAddress = BitFieldWrite64 (RankAddress, 15, 15, BitFieldRead32 (ChipId, 1, 1));
      } else {
        RankAddress = BitFieldWrite64 (RankAddress, 15, 15, BitFieldRead32 (Row, 1, 1));
      }

      if (DimmMtrReg.Bits.ddr4_3dsnumranks_cs > 2) {
        RankAddress = BitFieldWrite64 (RankAddress, 16, 16, BitFieldRead32 (ChipId, 2, 2));
      } else {
        RankAddress = BitFieldWrite64 (RankAddress, 16, 16, BitFieldRead32 (Row, 2, 2));
      }

      RankAddress = BitFieldWrite64 (RankAddress, 20, 22, BitFieldRead32 (Row, 3, 5));
      RankAddress = BitFieldWrite64 (RankAddress, 10, 13, BitFieldRead32 (Row, 6, 9));
      RankAddress = BitFieldWrite64 (RankAddress, 18, 18, BitFieldRead32 (Row, 10, 10));
      RankAddress = BitFieldWrite64 (RankAddress, 28, 28, BitFieldRead32 (Row, 11, 11));

      if (DimmMtrReg.Bits.ba_shrink) {
        RankAddress = BitFieldWrite64 (RankAddress, 7, 7, BitFieldRead32 (Row, 12, 12));
      } else {
        RankAddress = BitFieldWrite64 (RankAddress, 30, 30, BitFieldRead32 (Row, 12, 12));
      }

    } else { // Open Page

      //
      // Add Bank and BankGroup
      //
      if (BankXorEn == 1) {
        TempBankGroup = (UINT8) (BitFieldRead32 (Row, 3, 4));
        TempBank = (UINT8) (BitFieldRead32 (Row, 5, 6));
      }
      TempBankGroup ^= BankGroup;
      TempBank ^= Bank;
      TDBG("TempBankGroup = %d, TempBank=%d @ OpenPg", TempBankGroup, TempBank);

      if (AmapReg.Bits.cgbg_interleave == 0) {
        RankAddress = BitFieldWrite64 (RankAddress, 13, 13, BitFieldRead32 (TempBankGroup, 0, 0));
        if (!(DimmMtrReg.Bits.ba_shrink)) {
          RankAddress = BitFieldWrite64 (RankAddress, 17, 17, BitFieldRead32 (TempBankGroup, 1, 1));
        }
      } else if (AmapReg.Bits.cgbg_interleave == 1) {
        RankAddress = BitFieldWrite64 (RankAddress, 6, 6, BitFieldRead32 (TempBankGroup, 0, 0));
        if (!(DimmMtrReg.Bits.ba_shrink)) {
          RankAddress = BitFieldWrite64 (RankAddress, 17, 17, BitFieldRead32 (TempBankGroup, 1, 1));
        }
      } else {
        RankAddress = BitFieldWrite64 (RankAddress, 6, 6, BitFieldRead32 (TempBankGroup, 0, 0));
        if (!(DimmMtrReg.Bits.ba_shrink)) {
          RankAddress = BitFieldWrite64 (RankAddress, 7, 7, BitFieldRead32 (TempBankGroup, 1, 1));
       }
      }

      RankAddress = BitFieldWrite64 (RankAddress, 18, 19, BitFieldRead32 (TempBank, 0, 1));

      //
      // Add the Column
      //
      RankAddress = BitFieldWrite64 (RankAddress, 3, 5, BitFieldRead32 (Col, 0, 2));
      if (AmapReg.Bits.cgbg_interleave == 0) {
        RankAddress = BitFieldWrite64 (RankAddress, 6, 12, BitFieldRead32 (Col, 3, 9));
      } else if (AmapReg.Bits.cgbg_interleave == 1) {
        RankAddress = BitFieldWrite64 (RankAddress, 13, 13, BitFieldRead32 (Col, 3, 3));
        RankAddress = BitFieldWrite64 (RankAddress, 7, 12, BitFieldRead32 (Col, 4, 9));
      } else {
        RankAddress = BitFieldWrite64 (RankAddress, 13, 14, BitFieldRead32 (Col, 3, 4));
        RankAddress = BitFieldWrite64 (RankAddress, 8, 12, BitFieldRead32 (Col, 5, 9));
      }


      // Add the Row and ChipId
      if (DimmMtrReg.Bits.ddr4_3dsnumranks_cs == 0) {
        if (AmapReg.Bits.cgbg_interleave == 2) {
          RankAddress = BitFieldWrite64 (RankAddress, 17, 17, BitFieldRead32 (Row, 0, 0));
        } else {
          RankAddress = BitFieldWrite64 (RankAddress, 14, 14, BitFieldRead32 (Row, 0, 0));
        }

      } else {
        if (AmapReg.Bits.cgbg_interleave == 2) {
          RankAddress = BitFieldWrite64 (RankAddress, 17, 17, BitFieldRead32 (ChipId, 0, 0));
        } else {
          RankAddress = BitFieldWrite64 (RankAddress, 14, 14, BitFieldRead32 (ChipId, 0, 0));
        }
      }

      if (DimmMtrReg.Bits.ddr4_3dsnumranks_cs > 1) {
        RankAddress = BitFieldWrite64 (RankAddress, 15, 15, BitFieldRead32 (ChipId, 1, 1));
      } else {
        RankAddress = BitFieldWrite64 (RankAddress, 15, 15, BitFieldRead32 (Row, 1, 1));
      }

      if (DimmMtrReg.Bits.ddr4_3dsnumranks_cs > 2) {
        RankAddress = BitFieldWrite64 (RankAddress, 16, 16, BitFieldRead32 (ChipId, 2, 2));
      } else {
        RankAddress = BitFieldWrite64 (RankAddress, 16, 16, BitFieldRead32 (Row, 2, 2));
      }

      RankAddress = BitFieldWrite64 (RankAddress, 20, 28, BitFieldRead32 (Row, 3, 11));

      if (DimmMtrReg.Bits.ba_shrink) {
        if (AmapReg.Bits.cgbg_interleave == 2) {
          RankAddress = BitFieldWrite64 (RankAddress, 7, 7, BitFieldRead32 (Row, 12, 12));
        } else {
          RankAddress = BitFieldWrite64 (RankAddress, 17, 17, BitFieldRead32 (Row, 12, 12));
        }
      } else {
        RankAddress = BitFieldWrite64 (RankAddress, 30, 30, BitFieldRead32 (Row, 12, 12));
      }
    }

    RankAddress = BitFieldWrite64 (RankAddress, 29, 29, BitFieldRead32 (Row, 13, 13));

    LowPos = (UINT8)(RIR_UNIT + 2 - DimmMtrReg.Bits.ba_shrink + DimmMtrReg.Bits.ddr4_3dsnumranks_cs);
    HighPos = (UINT8)(LowPos + (DimmMtrReg.Bits.ra_width - 2));
    RankAddress = BitFieldWrite64 (RankAddress, LowPos, HighPos, BitFieldRead32 (Row, 14, 14 + HighPos - LowPos));

    TA->RankAddress = RankAddress;
    //TCRIT("GetRankAddress: RankAddress for DDR4:0x%lx", TA->RankAddress);
  } else {
    TA->RankAddress = TA->DPA;
    TCRIT("GetRankAddress: RankAddress for DDRT:0x%llx", TA->RankAddress);
  }

  return EFI_SUCCESS;

}

/**

Translate Dimm Address to System Address

@param  [in]  pTranslatedAddress  - pointer to the structure containing DIMM Address
@param  [out] pTranslatedAddress   -- pointer to the structure containing DIMM Address

@retval EFI_SUCCESS / Error code


**/
EFI_STATUS
EFIAPI
TranslateDimmAddress(
  IN      dimmBDFst *pdimmBdf,
  IN OUT  PTRANSLATED_ADDRESS TA
)
{
  EFI_STATUS Status;

  //TCRIT("The SocketId is 0x%lx", TA->SocketId);
  //TCRIT("The MemoryControllerId is 0x%lx", TA->MemoryControllerId);
  //TCRIT("The ChannelId is 0x%lx", TA->ChannelId);
  //TCRIT("The DimmSlot is 0x%lx", TA->DimmSlot);
  //TCRIT("The PhysicalRankId is 0x%lx", TA->PhysicalRankId);
  //TCRIT("The Row is 0x%lx", TA->Row);
  //TCRIT("The Col is 0x%lx", TA->Col);
  //TCRIT("The Bank is 0x%lx", TA->Bank);
  //TCRIT("The BankGroup is 0x%lx", TA->BankGroup);

  TA->SadId = (UINT8)-1;
  TA->TadId = (UINT8)-1;
  TA->TargetId = (UINT8)-1;
  TA->LogChannelId = (UINT8)-1;
  TA->SystemAddress = (UINT64)-1;
  TA->NmSystemAddress = (UINT64)-1;
  TA->ChannelAddress = (UINT64)-1;
  TA->NmChannelAddress = (UINT64)-1;
  TA->NmRankAddress = (UINT64)-1;

  GetDimmType (pdimmBdf, TA);

  Status = GetRankAddress (pdimmBdf, TA);
  if (EFI_ERROR (Status)) {
    TCRIT("Error occurred in getting the RankAddress\n");
    return EFI_INVALID_PARAMETER;
  }
  TDBG("The RankAddress is 0x%llx", TA->RankAddress);

  Status = GetChannelAddress (pdimmBdf, TA);
  if (EFI_ERROR (Status)) {
    TCRIT("Error occurred in getting the ChannelAddress\n");
    return EFI_INVALID_PARAMETER;
  }
  TDBG("The ChannelAddress is 0x%llx", TA->ChannelAddress);

  Status = GetSystemAddress (pdimmBdf, TA);
  if (EFI_ERROR (Status)) {
    TCRIT("Error occurred in getting the SystemAddress\n");
    return EFI_INVALID_PARAMETER;
  }
  TDBG("The SystemAddress is 0x%llx\n", TA->SystemAddress);
  return Status;

}


/**
  Translate a DIMM address to system address.

  @param[in,out]  TranslatedAddress   Pointer to the DIMM Address structure

  @retval EFI_SUCCESS                 Translation was successful
  @retval EFI_INVALID_PARAMETER       DIMM address was null or invalid
**/
EFI_STATUS
EFIAPI
DimmAddressToSystemAddress (
  IN      dimmBDFst *pdimmBdf,
  IN OUT  PTRANSLATED_ADDRESS TranslatedAddress
  )
{
  EFI_STATUS Status;

  Status = ValidateAddressTranslationParams (TranslatedAddress);

  if (EFI_ERROR (Status)) {
    TCRIT("Input Parameters for Reverse Address Translation are not valid!!\n");
    return EFI_INVALID_PARAMETER;
  }

  Status = TranslateDimmAddress (pdimmBdf, TranslatedAddress);

  if (Status == EFI_SUCCESS) {
    TDBG("Reverse Address Translation is Successful!!\n");
  }
  else {
    TCRIT("Reverse Address Translation is unSuccessful!!\n");
  }

  return Status;
}


/*** BIOS_LB69_BETA/ServerSiliconPkg/Mem/Library/MemMcIpLib/Ras/MemRasPatrolScrubIcx.c ***/
/**
  Get MCMTR register values.

  This routine returns close_page, bank_xor_enable, ddr_xor_mode_enable, ddr_half_xor_mode_enable bit field values

  @param[in]  Skt             Socket index.
  @param[in]  Mc              Memory controller on socket.
  @param[in]  ChOnSkt         Channel on socket.
  @param[out]  ClosePg        close_page value return  OPTIONAL Pass NULL if not needed
  @param[out]  BankXorEn      bank_xor_enable value return   OPTIONAL Pass NULL if not needed
  @param[out]  ChXorModeEn          ddr_xor_mode_enable value return   OPTIONAL Pass NULL if not needed
  @param[out]  ClusterXorModeEn      ddr_half_xor_mode_enable value return   OPTIONAL Pass NULL if not needed
**/
VOID
EFIAPI
GetMcmtr (
  IN  dimmBDFst *pdimmBdf,
  IN  UINT8   Skt,
  IN  UINT8   Mc,
  IN  UINT8   ChOnSkt,
  OUT UINT32   *ClosePg,              OPTIONAL
  OUT UINT32   *BankXorEn,            OPTIONAL
  OUT UINT32   *ChXorModeEn,          OPTIONAL
  OUT UINT32   *ClusterXorModeEn      OPTIONAL
  )
{
  MCMTR_MC_MAIN_STRUCT McmtrMain;
  endPointRegInfo reg = {
          .msgType         = MESSAGE_MMIO,
          .addrType        = BAR_32_BITS,
          .optType         = OPTION_DWORD,
          .dimmBdf.cpuType = pdimmBdf->cpuType,
          .dimmBdf.socket  = pdimmBdf->socket,
          .dimmBdf.channel = pdimmBdf->channel,
          .dimmBdf.bus     = pdimmBdf->bus,
          .device          = pdimmBdf->imc + DEVICE_OF_FIRST_IMC,
          .function        = FUNCTION_IMC_MMIO,
          .offset          = MCMTR_MC_MAIN_REG /* Page-2986 $13.5.84: 4 byte */
  };
  UN_USED(Skt);
  UN_USED(Mc);
  UN_USED(ChOnSkt);

  McmtrMain.Data = ReadCpuCsrMmio(&reg);
  TDBG("MCMTR_MC_MAIN_REG: BDF %d %d %d, data 0x%x", reg.dimmBdf.bus, reg.device, reg.function, McmtrMain.Data);
  if (ClosePg != NULL) {
    *ClosePg   = McmtrMain.Bits.close_pg;
  }
  if (BankXorEn != NULL) {
    *BankXorEn = McmtrMain.Bits.bank_xor_enable;
  }
  if (ChXorModeEn != NULL) {
    *ChXorModeEn = McmtrMain.Bits.ddr_xor_mode_enable;
  }
  if (ClusterXorModeEn != NULL) {
    *ClusterXorModeEn = McmtrMain.Bits.ddr_half_xor_mode_enable;
  }
}


/**

  This function checks if the given System Address falls under NON DRAM SADS

  @param  [in]  SystemAddress


  @retval TRUE  - If the address falls under DRAM ADDRESS
          FALSE = If the address falls under Legacy, MMCFG, MMIOL or MMIOH SADs

**/

BOOLEAN
EFIAPI
IsDramAddress (
  IN UINT64    SystemAddress
  )
{

  if ((SystemAddress >= VGA_RAM_BASE) && (SystemAddress < SIZE_1MB)) {
    TCRIT("The given System Address 0x%llx falls under Legacy Region\n", SystemAddress);
    return FALSE;
  }

  if ((SystemAddress >= mMmCfgBase) && (SystemAddress < mMmCfgLimit)) {
    TCRIT("The given System Address 0x%llx falls under MMCFG Region\n", SystemAddress);
    return FALSE;
  }

  if ((SystemAddress >= mPlatGlobalMmiolBase) && (SystemAddress < mPlatGlobalMmiolLimit)) {
    TCRIT("The given System Address 0x%llx falls under MMIO_L Region\n", SystemAddress);
    return FALSE;
  }

  if ((SystemAddress >= mPlatGlobalMmiohBase) && (SystemAddress < mPlatGlobalMmiohLimit)) {
    TCRIT("The given System Address 0x%llx falls under MMIO_H Region\n", SystemAddress);
    return FALSE;
  }

  return TRUE;
}


/**

  This function checks if the given System Address falls under DDRT Block Address

  @param  [in]  pTranslatedAddress   -- pointer to the structure containing DIMM Address


  @retval TRUE  - If the address falls under DDRT Block
          FALSE = If the address doesn't fall under DDRT Block

**/

BOOLEAN
EFIAPI
IsDdrtBlockAddress (
  IN dimmBDFst *pdimmBdf,
  IN TRANSLATED_ADDRESS * const TranslatedAddress
  )
{

  UINT8                                     Skt;
  BLOCK_DECODER_ADDR_N0_CHABC_SAD1_STRUCT   ChaBlockDecoderAddrN0;
  BLOCK_DECODER_ADDR_N1_CHABC_SAD1_STRUCT   ChaBlockDecoderAddrN1;
  UINT64                                    BlockBase;
  UINT64                                    BlockLimit;
  UINT64                                    SystemAddress;

  endPointRegInfo reg = {
          /*
           * EDS Vol-1 Page-232 Table 10-26:
           * VT-d Register
           * EndPointConfig() MMIO
           * BDF: 0 0 0
           * BAR ID: 0
           */
          .msgType         = MESSAGE_LOCAL_PCI,
          .addrType        = PCI_BDF_REG,
          .optType         = OPTION_DWORD /* OPTION_QWORD: KO */,
          .dimmBdf.cpuType = pdimmBdf->cpuType,
          .dimmBdf.socket  = pdimmBdf->socket,
          .dimmBdf.channel = pdimmBdf->channel,
          .dimmBdf.bus     = BUS_NUMBER_CHA,
          .device          = 29, /* page-3501 $15.4.72 */
          .function        = 1,
          .offset          = BLOCK_DECODER_ADDR_N0_CHABC_SAD1_REG /* CHA Register */
  };

  SystemAddress = TranslatedAddress->SystemAddress;
  for (Skt = 0; Skt < MAX_SOCKET; Skt++) {

    if ((mSocketPresentBitMap & (BIT0  << Skt)) == 0) {
      continue;
    }

    reg.dimmBdf.socket  = Skt;
    reg.offset = BLOCK_DECODER_ADDR_N0_CHABC_SAD1_REG;
    ChaBlockDecoderAddrN0.Data = ReadCpuCsrPciLocal(&reg); // Skt, 0
    reg.offset = BLOCK_DECODER_ADDR_N1_CHABC_SAD1_REG;
    ChaBlockDecoderAddrN1.Data = ReadCpuCsrPciLocal(&reg); // Skt, 0

    if (!(ChaBlockDecoderAddrN1.Bits.block_rule_en)) {
      break;
    }

    BlockBase = (((UINT64)ChaBlockDecoderAddrN0.Bits.block_base) << SAD_UNIT);
    BlockLimit = (((UINT64)ChaBlockDecoderAddrN1.Bits.block_limit + 1) << SAD_UNIT);

    TDBG("%s() BlockBase 0x%llx, SystemAddress 0x%llx, BlockLimit 0x%llx\n",
    		__FUNCTION__, BlockBase, SystemAddress, BlockLimit);
    if ((BlockBase <= SystemAddress) && (SystemAddress < BlockLimit)) {
      TranslatedAddress->MemType = MemType1lmCtrl;
      TranslatedAddress->DimmType = ddrtdimmType;
      TCRIT("System Address 0x%llx belongs to Block Memory\n", SystemAddress);
      return TRUE;
    }
  }

  return FALSE;
}


/**

 This function checks if the address belongs to Remote SAD and returns appropriate SktId

 @param  [in]  systemAddress

 @retval SktId

**/


UINT8
EFIAPI
GetSktIdFromRemoteSad (
  IN dimmBDFst *pdimmBdf,
  IN  UINT64  SystemAddress
  )
{
  UINT8   SktId = 0xFF;
  UINT8   SadIndex = 0;
  UINT64  RemoteSadBase = 0;
  UINT64  RemoteSadLimit = 0;
  REMOTE_DRAM_RULE_CFG_0_N0_CHABC_SAD_STRUCT RemoteDramRuleN0;
  REMOTE_DRAM_RULE_CFG_0_N1_CHABC_SAD_STRUCT RemoteDramRuleN1;

  endPointRegInfo reg0 = {
          .msgType         = MESSAGE_LOCAL_PCI,
          .addrType        = PCI_BDF_REG,
          .optType         = OPTION_DWORD/*OPTION_QWORD*/,
		                  /* Although EDS says that it has 8-byte size,
		                   * it is split to two 4-bytes No and N1 register */
          .dimmBdf.cpuType = pdimmBdf->cpuType,
          .dimmBdf.socket  = pdimmBdf->socket,
          .dimmBdf.channel = pdimmBdf->channel,
          .dimmBdf.bus     = BUS_NUMBER_CHA,
          .device          = 29, /* page-3446 $15.3.66 */
          .function        = 0,
          .offset          = REMOTE_DRAM_RULE_CFG_0_N0_CHABC_SAD_REG /* CHA Register */
  };

  endPointRegInfo reg1 = {
          .msgType         = MESSAGE_LOCAL_PCI,
          .addrType        = PCI_BDF_REG,
          .optType         = OPTION_DWORD/*OPTION_QWORD*/,
          .dimmBdf.cpuType = pdimmBdf->cpuType,
          .dimmBdf.socket  = pdimmBdf->socket,
          .dimmBdf.channel = pdimmBdf->channel,
          .dimmBdf.bus     = BUS_NUMBER_CHA,
          .device          = 29,
          .function        = 0,
          .offset          = REMOTE_DRAM_RULE_CFG_0_N1_CHABC_SAD_REG /* CHA Register */
  };

  for (SadIndex = 0; SadIndex < REMOTE_SAD_RULES_10NM_WAVE1; SadIndex++) {
    reg0.offset = mRemoteDramRuleCfgN0[SadIndex];
    reg1.offset = mRemoteDramRuleCfgN1[SadIndex];
    RemoteDramRuleN0.Data = ReadCpuCsrPciLocal(&reg0); // 0, 0
    RemoteDramRuleN1.Data = ReadCpuCsrPciLocal(&reg1); // 0, 0
    TDBG("%s(): SAD index = %d", __FUNCTION__, SadIndex);
    TDBG("[PECI] - mRemoteDramRuleCfgN0[%d] = 0x%x", SadIndex, RemoteDramRuleN0.Data);
    TDBG("[PECI] - mRemoteDramRuleCfgN1[%d] = 0x%x", SadIndex, RemoteDramRuleN1.Data);

    RemoteSadBase = (((UINT64)RemoteDramRuleN0.Bits.base) << SAD_UNIT);
    RemoteSadLimit = (((UINT64)RemoteDramRuleN1.Bits.limit + 1) << SAD_UNIT);

    if (RemoteDramRuleN0.Bits.rule_enable != 1) {
      break;
    }

    TDBG("%s() RemoteSadBase 0x%llx, SystemAddress 0x%llx, RemoteSadLimit 0x%llx",
    		__FUNCTION__, RemoteSadBase, SystemAddress, RemoteSadLimit);
    if ((RemoteSadBase <= SystemAddress) && (SystemAddress < RemoteSadLimit)) {
      SktId = (UINT8)RemoteDramRuleN1.Bits.pkg;
      break;
    }
  }

  return SktId;

}


/**

  This function provides the the RouteTable number belongs to MC or Ch
  by reading the DRAM_MC_TARGET or DRAM_MC_CHANNEL register

  @param  [in]  SktId       -- Skt Id
  @param  [in]  SadIndex    -- Sad Index
  @parama [in]  TargetType  -- Target Type (MC or Channel)

  @retval RouteTable: 0 - HBM
                      1 - DDR4
                      2 - DDRT

**/
UINT8
EFIAPI
GetRouteTableType (
  IN dimmBDFst *pdimmBdf,
  IN UINT8     Skt,
  IN UINT8     SadIndex,
  IN UINT8     TargetType
  )
{
  UINT8                                    RouteTable;
  UINT32                                   DramMcRtCfg;

  endPointRegInfo reg = {
          .msgType         = MESSAGE_LOCAL_PCI,
          .addrType        = PCI_BDF_REG,
          .optType         = OPTION_DWORD,
          .dimmBdf.cpuType = pdimmBdf->cpuType,
          .dimmBdf.socket  = Skt, /* pdimmBdf->socket */
          .dimmBdf.channel = pdimmBdf->channel,
          .dimmBdf.bus     = BUS_NUMBER_CHA,
          .device          = 29, /* page-3449 $15.3.80 */
          .function        = 0,
          .offset          = DRAM_MC_TARGET_CHABC_SAD_REG /* 4 byte CHA Register */
  };

  if (TargetType == MC_ROUTE_TABLE) {
	TDBG("GetRouteTableType for MC\n");
    DramMcRtCfg = ReadCpuCsrPciLocal(&reg); // Skt, 0,
  } else {
	TDBG("GetRouteTableType for Ch\n");
    reg.offset = DRAM_MC_CHANNEL_CHABC_SAD_REG;
    DramMcRtCfg = ReadCpuCsrPciLocal(&reg); // Skt, 0,
  }

  //
  //  2 Bits per each SAD, extract the TableType for given SAD
  //
  RouteTable = ((DramMcRtCfg) >> (SadIndex * 2)) & 3;
  TDBG("RouteTable is of type: 0x%x", RouteTable);

  return RouteTable;

 }

/**

  This function provides the TgtId based on the InterleaveListCfg Register

  @param  [in]  SktId
  @param  [in]  InterleavelistIndex
  @param  [in]  SadIndex
  @param  [out]  Pointer to TgtId - will be modified in this function

  @retval EFI_SUCCESS  - If the target is local
          EFI_NOT_FOUND - If the target is remote

**/

BOOLEAN
EFIAPI
GetTargetId (
  IN dimmBDFst *pdimmBdf,
  IN  UINT8       Skt,
  IN  UINT8       InterleaveListIndex,
  IN  UINT8       SadIndex,
  OUT UINT8       *TgtId
  )
{
  INTERLEAVE_LIST_CFG_0_CHABC_SAD_STRUCT   InterleaveListCfg;

  endPointRegInfo reg = {
          .msgType         = MESSAGE_LOCAL_PCI,
          .addrType        = PCI_BDF_REG,
          .optType         = OPTION_DWORD,
          .dimmBdf.cpuType = pdimmBdf->cpuType,
          .dimmBdf.socket  = Skt, /* pdimmBdf->socket */
          .dimmBdf.channel = pdimmBdf->channel,
          .dimmBdf.bus     = BUS_NUMBER_CHA,
          .device          = 29, /* page-3437 $15.3.35 */
          .function        = 0,
          .offset          = mInterleaveListCfgCha[SadIndex] /* 4 byte CHA Register */
  };

  InterleaveListCfg.Data = ReadCpuCsrPciLocal(&reg); // Skt, 0
  TDBG("[PECI] - mInterleaveListCfgCha[%d]: 0x%x", SadIndex, InterleaveListCfg.Data);

   switch (InterleaveListIndex) {
    case 0:
      *TgtId = (UINT8)InterleaveListCfg.Bits.package0;
      break;
    case 1:
      *TgtId = (UINT8)InterleaveListCfg.Bits.package1;
      break;
    case 2:
      *TgtId = (UINT8)InterleaveListCfg.Bits.package2;
      break;
    case 3:
      *TgtId = (UINT8)InterleaveListCfg.Bits.package3;
      break;
    case 4:
      *TgtId = (UINT8)InterleaveListCfg.Bits.package4;
      break;
    case 5:
      *TgtId = (UINT8)InterleaveListCfg.Bits.package5;
      break;
    case 6:
      *TgtId = (UINT8)InterleaveListCfg.Bits.package6;
      break;
    case 7:
      *TgtId = (UINT8)InterleaveListCfg.Bits.package7;
      break;
    default:
      *TgtId = 0xFF;
      break;
  } // switch

  if (*TgtId & BIT3) {
    *TgtId &= 0x7;
    TDBG("TgtId 0x%x", *TgtId);
    return TRUE;

  } else {
    TDBG("\nGetTargetId - remote socket\n");
    return FALSE;
  }

}


/**

   Given the Interleave Mode return the interleave index.
   This is equivalent to (systemAddress / interleave granularity) % Posible Nodes

  @param  [in]  interleave_mode
  @param  [in]  systemAddress

  @retval InterleaveList Index


**/


UINT8
EFIAPI
GetInterleaveListIndex(
  IN  UINT8   InterleaveMode,
  IN  UINT64  SystemAddress
  )
{
  UINT8   InterleaveListIndex = 0;

  TDBG("Interleave mode: 0x%x", InterleaveMode);
  switch(InterleaveMode) {
    case 0:
      InterleaveListIndex = (UINT8)(RShiftU64(SystemAddress , 6) & 0x7);
      TDBG("Interleave Index [6,7,8]: 0x%x", InterleaveListIndex);
      break;
    case 1:
      InterleaveListIndex = (UINT8)(RShiftU64(SystemAddress , 8) & 0x7);
      TDBG("Interleave Index [8,9,10]: 0x%x", InterleaveListIndex);
      break;
    case 2:
      InterleaveListIndex = (UINT8)(RShiftU64(SystemAddress , 12) & 0x7);
      TDBG("Interleave Index [12,13,14]: 0x%x", InterleaveListIndex);
      break;
    case 3:
      InterleaveListIndex = (UINT8)(RShiftU64(SystemAddress , 13) & 0x7);
      TDBG("Interleave Index [13,14,15]: 0x%x", InterleaveListIndex);
      break;
    default:
      TCRIT("Invalid Value: 0x%x", InterleaveListIndex);
      break;
  }
  return InterleaveListIndex;
}


EFI_STATUS
EFIAPI
GetChaForSystemAddress (
  IN  dimmBDFst *pdimmBdf,
  IN  UINT8  SktId,
  IN  UINT64 SystemAddress,
  OUT UINT8  *Cha
  )
{
  CHA_SNC_CONFIG_CHA_PMA_STRUCT       SncConfigPma10nm;
  UMA_CLUSTER_CONFIG_IIO_VTD_STRUCT   UmaClusterCfg;
  SNC_BASE_1_IIO_VTD_STRUCT           SncBase;
  SNC_UPPER_BASE_IIO_VTD_STRUCT       SncUpperBase;
  UINT8                               NumClusters;
  UINT8                               Cluster = 0;
  UINT64                              PrevSncLimit;
  UINT64                              ClusterBase;
  UINT64                              ClusterLimit;
  UINT8                               Index;

  endPointRegInfo reg = {
          /*
           * OK with PCI type:  peciapp -t 48 -d 0 -r 5 193 0 3 0 0 0 4 0 32 2 240 1
           * KO with MMIO type: peciapp -t 48 -d 0 -r 5 193 0 5 0 0 1 5 0 208 126 32 2 0 0
           */
          .msgType         = MESSAGE_LOCAL_PCI,
          .addrType        = PCI_BDF_REG,
          .optType         = OPTION_DWORD,
          .dimmBdf.cpuType = pdimmBdf->cpuType,
          .dimmBdf.socket  = SktId, /* pdimmBdf->socket */
          .dimmBdf.channel = pdimmBdf->channel,
          .dimmBdf.bus     = BUS_NUMBER_CHA,
          .device          = 0, /* page-3388 $15.1.33 */
          .function        = 0, /* BIOS: SktId, 0 */
          .offset          = CHA_SNC_CONFIG_CHA_PMA_REG /* 4 byte CHA Register */
  };

  endPointRegInfo reg_vtd = {
          /*
           * OK with PCI type:  peciapp -t 48 -d 0 -r 5 193 0 3 0 0 0 4 0 76 4 0 0
           * KO with MMIO type: peciapp -t 48 -d 0 -r 5 193 0 5 0 0 1 5 0 208 126 76 4 0 0
           */
          .msgType         = MESSAGE_LOCAL_PCI,
          .addrType        = PCI_BDF_REG,
          .optType         = OPTION_DWORD,
          .dimmBdf.cpuType = pdimmBdf->cpuType,
          .dimmBdf.socket  = SktId, /* pdimmBdf->socket */
          .dimmBdf.channel = pdimmBdf->channel,
          .dimmBdf.bus     = BUS_NUMBER_VTD,
          .device          = 0, /* page-773 $6.1.108 */
          .function        = 0, /* BIOS: SktId, 0 */
          .offset          = UMA_CLUSTER_CONFIG_IIO_VTD_REG /* 4 byte Mesh2IIO MMAP/Intel VT-d */
  };

  SncConfigPma10nm.Data = ReadCpuCsrPciLocal(&reg); // SktId, 0
  UmaClusterCfg.Data = ReadCpuCsrPciLocal(&reg_vtd); // SktId, 0

  NumClusters = 1;
  *Cha = 0;

  if (SncConfigPma10nm.Bits.snc_ind_en) {
    NumClusters = (UINT8)SncConfigPma10nm.Bits.num_snc_clu + 1;
    TDBG("Clustering/Sectioning is enabled.  NumClusters = 0x%d\n", NumClusters);

    if (UmaClusterCfg.Bits.uma_clustering_enable) {      // Sectoring Mode (Hemi/Quad)
      TDBG("Sectioning is enabled\n");
      if (UmaClusterCfg.Bits.uma_defeature_xor) {
        Cluster = (UINT8)BitFieldRead64(SystemAddress, 11, 12);
      } else {
        Cluster = (UINT8)(BitFieldRead64(SystemAddress, 8, 9) ^ BitFieldRead64(SystemAddress, 11, 12)
                        ^ BitFieldRead64(SystemAddress, 17, 18) ^ BitFieldRead64(SystemAddress, 25, 26));
      }
      Cluster = Cluster & (NumClusters / 2);
      TDBG("Cluster in Sectioning mode = 0x%x", Cluster);

    } else {
      if (SncConfigPma10nm.Bits.full_snc_en) {
        reg_vtd.offset = SNC_UPPER_BASE_IIO_VTD_REG; /* Mesh2IIO MMAP/Intel VT-d */
        SncUpperBase.Data = ReadCpuCsrPciLocal(&reg_vtd); // SktId, 0
        reg_vtd.offset = SNC_BASE_1_IIO_VTD_REG; /* Mesh2IIO MMAP/Intel VT-d */
        SncBase.Data = ReadCpuCsrPciLocal(&reg_vtd); // SktId, 0
        PrevSncLimit = (LShiftU64 (SncUpperBase.Bits.starting_addr_upper_1, 46) | LShiftU64 (SncBase.Bits.starting_addr_0, 30)) - 1;
        TDBG("PrevSncLimit = 0x%llx\n", PrevSncLimit);

        for (Index = 0; Index < NumClusters; Index++) {
          ClusterBase = PrevSncLimit + 1;
          reg_vtd.offset = mIioSncBaseRegisterOffset[Index + 1]; // Dev 0, Func 0
          SncBase.Data = ReadCpuCsrPciLocal(&reg_vtd); // SktId, 0
          ClusterLimit = (LShiftU64 (RShiftU64 (SncUpperBase.Data, (Index + 1) * 6) & 0x3f, 46) |
                         LShiftU64 (SncBase.Data & 0xffff, 30)) - 1;
          if (PrevSncLimit == ClusterLimit + 1) {
            TCRIT("Cluster has no range setup = 0x%x\n", Index);
            return EFI_INVALID_PARAMETER;
          }

          if ((ClusterBase <= SystemAddress) && (SystemAddress < ClusterLimit)) {
            Cluster = Index;
            TDBG("Cluster in Clustering mode = 0x%x", Cluster);
            break;
          }
          PrevSncLimit = ClusterLimit;
        }
      }
    }

    *Cha = Cluster * (mTotCha / NumClusters);
  }

  TDBG("Cha = 0x%x\n", *Cha);
  return EFI_SUCCESS;
}



/**

  This function gets the McTargetId reading DRAMTgtRouteTable

  @param  [in]  SktId           -- Skt Id
  @param  [in]  TargetHalf      -- Target Half
  @param  [in]  RouteTableType  -- Route Table Type
  @param  [in]  RouteTableIndex -- Index into the Route Table

  @retval McTargetId

**/

UINT8
EFIAPI
GetMcTargetId (
  IN  dimmBDFst *pdimmBdf,
  IN  UINT8     Skt,
  IN  UINT8     TargetHalf,
  IN  UINT8     Cha,
  IN  UINT8     RouteTableType,
  IN  UINT8     RouteTableIndex
  )
{
  H0_TGT_ROUTE_TABLE_0_CHA_MISC_STRUCT  H0TgtRt0;
  UINT8                                 McTgt = 0;

  endPointRegInfo reg = {
          .msgType         = MESSAGE_LOCAL_PCI,
          .addrType        = PCI_BDF_REG,
          .optType         = OPTION_DWORD,
          .dimmBdf.cpuType = pdimmBdf->cpuType,
          .dimmBdf.socket  = Skt, /* pdimmBdf->socket */
          .dimmBdf.channel = pdimmBdf->channel,
          .dimmBdf.bus     = BUS_NUMBER_CHA,
          .device          = 10, /* page-3413 $15.2.36 */
          .function        = 0,
          .offset          = mDramTgtRouteTable[RouteTableType][TargetHalf] /* 4 byte CHA Register */
  };
  UN_USED(Cha);

  H0TgtRt0.Data = ReadCpuCsrPciLocal(&reg); // Skt, Cha

  switch (RouteTableIndex) {
    case 0:
      McTgt = (UINT8)H0TgtRt0.Bits.target_id_0;
      break;
    case 1:
      McTgt = (UINT8)H0TgtRt0.Bits.target_id_1;
      break;
    case 2:
      McTgt = (UINT8)H0TgtRt0.Bits.target_id_2;
      break;
    case 3:
      McTgt = (UINT8)H0TgtRt0.Bits.target_id_3;
      break;
    case 4:
      McTgt = (UINT8)H0TgtRt0.Bits.target_id_4;
      break;
    case 5:
      McTgt = (UINT8)H0TgtRt0.Bits.target_id_5;
      break;
    case 6:
      McTgt = (UINT8)H0TgtRt0.Bits.target_id_6;
      break;
    case 7:
      McTgt = (UINT8)H0TgtRt0.Bits.target_id_7;
      break;
    default:
      McTgt = 0xff;
      break;

  }
  TDBG("McTgt is %x\n", McTgt);
  return McTgt;
}

/**

  This function gets the ChID reading DRAMChRouteTable

  @param  [in]  SktId           -- Skt Id
  @param  [in]  TargetHalf      -- Target Half
  @param  [in]  RouteTableType  -- Route Table Type
  @param  [in]  RouteTableIndex -- Index into the Route Table

  @retval McChannelId

**/

UINT8
EFIAPI
GetMcChId (
  IN  dimmBDFst *pdimmBdf,
  IN  UINT8     Skt,
  IN  UINT8     TargetHalf,
  IN  UINT8     Cha,
  IN  UINT8     RouteTableType,
  IN  UINT8     RouteTableIndex
  )
{
  H0_CH_ROUTE_TABLE_0_CHA_MISC_STRUCT   H0ChRt0;
  UINT8                                 McCh = 0;

  endPointRegInfo reg = {
          .msgType         = MESSAGE_LOCAL_PCI,
          .addrType        = PCI_BDF_REG,
          .optType         = OPTION_DWORD,
          .dimmBdf.cpuType = pdimmBdf->cpuType,
          .dimmBdf.socket  = Skt, /* pdimmBdf->socket */
          .dimmBdf.channel = pdimmBdf->channel,
          .dimmBdf.bus     = BUS_NUMBER_CHA,
          .device          = 10, /* page-3412 $15.2.36 */
          .function        = 0,
          .offset          = mDramChRouteTable[RouteTableType][TargetHalf] /* 4 byte CHA Register */
  };
  UN_USED(Cha);

  H0ChRt0.Data = ReadCpuCsrPciLocal(&reg); // Skt, Cha

  switch (RouteTableIndex) {
    case 0:
      McCh = (UINT8)H0ChRt0.Bits.channel_id_0;
      break;
    case 1:
      McCh = (UINT8)H0ChRt0.Bits.channel_id_1;
      break;
    case 2:
      McCh = (UINT8)H0ChRt0.Bits.channel_id_2;
      break;
    case 3:
      McCh = (UINT8)H0ChRt0.Bits.channel_id_3;
      break;
    case 4:
      McCh = (UINT8)H0ChRt0.Bits.channel_id_4;
      break;
    case 5:
      McCh = (UINT8)H0ChRt0.Bits.channel_id_5;
      break;
    case 6:
      McCh = (UINT8)H0ChRt0.Bits.channel_id_6;
      break;
    case 7:
      McCh = (UINT8)H0ChRt0.Bits.channel_id_7;
      break;
    default:
      McCh = 0xff;
      break;
  }
  TDBG("McChId is %x", McCh);
  return McCh;
}


EFI_STATUS
EFIAPI
Correct2LMTgtIdChId (
  IN  dimmBDFst *pdimmBdf,
  IN  UINT64  SystemAddress,
  IN  UINT8   SktId,
  IN OUT UINT8 *McId,
  IN OUT UINT8 *ChId
  )
{

  DRAM_RULE_CFG0_N0_MC_MAIN_STRUCT              McDramRuleN0;
  DRAM_RULE_CFG0_N1_MC_MAIN_STRUCT              McDramRuleN1;
  UINT8                                         FmChWays;
  UINT8                                         FmChLid;
  UINT8                                         FmChInterleaveBit;
  UINT8                                         Index;
  //UINT8                                         SktCh;
  UINT64                                        SadLimit;
  UINT64                                        PrevSadLimit;

  endPointRegInfo reg = {
          .msgType         = MESSAGE_MMIO,
          .addrType        = BAR_32_BITS,
          .optType         = OPTION_DWORD/*OPTION_QWORD*/,
          .dimmBdf.cpuType = pdimmBdf->cpuType,
          .dimmBdf.socket  = SktId, /* pdimmBdf->socket */
          .dimmBdf.channel = pdimmBdf->channel,
          .dimmBdf.bus     = pdimmBdf->bus,
          .device          = pdimmBdf->imc + DEVICE_OF_FIRST_IMC,
          .function        = FUNCTION_IMC_MMIO,
          .offset          = DRAM_RULE_CFG0_N0_MC_MAIN_REG /* Page-2950 $13.5.16: 8 byte size */
  };

  //SktCh         = (*McId * MAX_MC_CH) + *ChId;
  PrevSadLimit = 0;

  TDBG("%s() SktCh: %d, Ch: %d\n", __FUNCTION__, (*McId * MAX_MC_CH) + *ChId, pdimmBdf->channel);
  //reg.dimmBdf.channel = SktCh;

  for (Index = 0; Index < TAD_RULES; Index++) {
    reg.offset = mMcDramRuleCfgN0[Index];
    McDramRuleN0.Data = ReadCpuCsrMmio(&reg); // SktId, SktCh
    TDBG("ChaDramRule.Data for SadIndex 0x%x in Skt 0x%x is 0x%x\n", Index, SktId, McDramRuleN0.Data);

    if (McDramRuleN0.Bits.rule_enable != 1) {
      continue;
    }

    SadLimit = (((UINT64)McDramRuleN0.Bits.limit + 1) << SAD_UNIT);
      //
      // Find the SAD rule that holds the given SystemAddress
      //
    TDBG("%s() PrevSadLimit 0x%llx, SystemAddress 0x%llx, SadLimit 0x%llx",
    		__FUNCTION__, PrevSadLimit, SystemAddress, SadLimit);
    if ((PrevSadLimit <= SystemAddress) && (SystemAddress < SadLimit))
    {
        TDBG("FM DRAM Rule 0x%lx Match\n", Index);
        reg.offset = mMcDramRuleCfgN1[Index];
        McDramRuleN1.Data = ReadCpuCsrMmio(&reg); // SktId, SktCh


        if (McDramRuleN0.Bits.fm_target != *McId) {
          TDBG("FM Mc %d is changing to %d\n", *McId, McDramRuleN0.Bits.fm_target);
          *McId = (UINT8)McDramRuleN0.Bits.fm_target;
        }

        FmChWays = (UINT8)McDramRuleN0.Bits.fm_chn_ways + 1;

        if (FmChWays == 1) {
          FmChLid = 0;
        } else {
          FmChInterleaveBit = GetInterleaveBit ((UINT8)McDramRuleN1.Bits.fm_chn_gran);

          if (McDramRuleN1.Bits.fm_gran_eq) {
            FmChInterleaveBit += (UINT8)McDramRuleN1.Bits.fm_target_ways;
          }

          FmChLid = (UINT8)ModU64x32 (RShiftU64 (SystemAddress, FmChInterleaveBit) , FmChWays);
        }

        *ChId = (UINT8)(BitFieldRead64(McDramRuleN1.Bits.fm_chn_l2pid, (FmChLid * 2), (FmChLid * 2 + 1)));
        TDBG("FM ChId is %d\n", *ChId);
        return EFI_SUCCESS;

      }

      PrevSadLimit = SadLimit;
  }

  return EFI_INVALID_PARAMETER;
}



/**

  This function calculates the Index to the Route Table

  @param  [in]  SktId               -- Socket Id
  @param  [in]  SadIndex            -- Sad Index
  @param  [in]  TargetId            -- Target Id
  @param  [in]  RouteTableType      -- Route Table Type
  @param  [in]  SystemAddress       -- System Address

  @retval IndexToRouteTable

**/
UINT8
EFIAPI
GetIndexToRouteTable (
  IN dimmBDFst *pdimmBdf,
  IN  UINT8     Skt,
  IN  UINT8     SadIndex,
  IN  UINT8     TargetId,
  IN  UINT8     RouteTableType,
  IN  UINT64    SystemAddress
  )
{
  DRAM_GLOBAL_INTERLEAVE_CHABC_SAD_STRUCT  GlobalDramIntlv;
  DRAM_H0_RT0_MODE0_CHABC_SAD_STRUCT       DramRt0ModeCfg;
  DRAM_H0_RT1_MODE0_CHABC_SAD_STRUCT       DramRt1ModeCfg;
  DRAM_H0_RT2_MODE0_CHABC_SAD_STRUCT       DramRt2ModeCfg;
  UINT8                                    RtModeCfg;
  UINT8                                    RouteTableIndex;
  UINT8                                    TargetHalf = 0;
  UINT64                                   RtShiftAddress;

  endPointRegInfo reg = {
          .msgType         = MESSAGE_LOCAL_PCI,
          .addrType        = PCI_BDF_REG,
          .optType         = OPTION_DWORD,
          .dimmBdf.cpuType = pdimmBdf->cpuType,
          .dimmBdf.socket  = Skt, /* pdimmBdf->socket */
          .dimmBdf.channel = pdimmBdf->channel,
          .dimmBdf.bus     = BUS_NUMBER_CHA,
          .device          = 29, /* page-3454 $15.3.82 */
          .function        = 0,
          .offset          = DRAM_GLOBAL_INTERLEAVE_CHABC_SAD_REG /* 4 byte CHA Register */
  };

  RouteTableIndex = 0;
  TDBG("GetIndexToRouteTable for RouteTableType:0x%x and targetHalf:0x%x \n", RouteTableType, TargetHalf);
  GlobalDramIntlv.Data = ReadCpuCsrPciLocal(&reg); // Skt, 0

  if (RouteTableType > 3) {
    return 0xFF;
  }

  if (TargetId & 1) {
    TargetHalf = 1;
  }

  if (SadIndex >= SAD_RULES_10NM) {
    TCRIT("ERROR: SadIndex out of range \n");
    return 0xFF;
  }

  //
  // For HBM Route Table (RT0)
  //
  if (RouteTableType == 0) {
    //
    // For the case where RT0 is used for AppDirect
    //
    reg.offset = mDramRt0ModeOffset[TargetHalf][SadIndex / 9];// Func 0
    DramRt0ModeCfg.Data = ReadCpuCsrPciLocal(&reg);
    RtModeCfg = RShiftU64(DramRt0ModeCfg.Data, (SadIndex * 2)) & 0x3;
    RtShiftAddress = RShiftU64(SystemAddress, 6 + GlobalDramIntlv.Bits.rt2_interleave_shift);

    switch (RtModeCfg) {
    case 0:            //Single Target
      RouteTableIndex = (UINT8)RShiftU64(TargetId, 1);
      break;
    case 1:             //2-Way
      RouteTableIndex = (UINT8)(BitFieldRead64(RtShiftAddress, 0, 0));
      break;
    case 2:             //3-Way
      RouteTableIndex = (UINT8)(BitFieldRead64(ModU64x32(RtShiftAddress, 3), 0, 1));
      break;
    case 3:             //4-Way
      RouteTableIndex = (UINT8)(BitFieldRead64(RtShiftAddress, 0, 1));
      break;
    default:
      RouteTableIndex = 0xFF;
      break;
    }

    TDBG("RouteTableIndex: 0x%x \n", RouteTableIndex);
    return RouteTableIndex;
  } else {

    //
    // For DDR4 Route Table (RT1)
    //
    if (RouteTableType == 1) {
      reg.offset = mDramRt1ModeOffset[TargetHalf][SadIndex/9];// Func 0
      DramRt1ModeCfg.Data = ReadCpuCsrPciLocal(&reg); // Skt, 0
      RtModeCfg = RShiftU64 (DramRt1ModeCfg.Data, (SadIndex * 3)) & 0x7;

      if (GlobalDramIntlv.Bits.rt1_interleave_shift == INTERLEAVE_MODE_256B_XOR) {
        //
        // Use an XOR of SA bits: [10:8] ^ [16:14] ^ [24:22]
        // Note: 3-way is invalid for this mode.
        //
        RtShiftAddress = (BitFieldRead64 (SystemAddress, 8, 10) ^
                          BitFieldRead64 (SystemAddress, 14, 16) ^
                          BitFieldRead64 (SystemAddress, 22, 24));
      } else {
        RtShiftAddress = RShiftU64 (SystemAddress, 6 + GlobalDramIntlv.Bits.rt1_interleave_shift);
      }
      switch (RtModeCfg) {
        case 0:                 //Single Target
          RouteTableIndex = (UINT8) RShiftU64 (TargetId, 1);
          break;
        case 1:                 //2-Way
          RouteTableIndex = (UINT8)(BitFieldRead64(RtShiftAddress, 0, 0));
          break;
        case 2:                 //3-Way
          RouteTableIndex = (UINT8)(BitFieldRead64(ModU64x32 (RtShiftAddress, 3), 0, 1));
          break;
        case 3:                 //4-Way
          RouteTableIndex = (UINT8)(BitFieldRead64(RtShiftAddress, 0, 1));
          break;
        case 6:                 //8-Way
          RouteTableIndex = (UINT8)(BitFieldRead64(RtShiftAddress, 0, 2));
          break;
        default:
          RouteTableIndex = 0xFF;
          break;
      }
      TDBG("RouteTableIndex: 0x%x \n", RouteTableIndex);
      return RouteTableIndex;
    } else {

      //
      // For DDRT Route Table (RT2)
      //
      reg.offset = mDramRt2ModeOffset[TargetHalf];// Func 0
      DramRt2ModeCfg.Data = ReadCpuCsrPciLocal(&reg);
      RtModeCfg = RShiftU64 (DramRt2ModeCfg.Data, (SadIndex * 2)) & 0x3;
      RtShiftAddress = RShiftU64 (SystemAddress, 6 + GlobalDramIntlv.Bits.rt2_interleave_shift);

      switch (RtModeCfg) {
         case 0:            //Single Target
          RouteTableIndex = (UINT8) RShiftU64 (TargetId, 1);
          break;
        case 1:             //2-Way
          RouteTableIndex = (UINT8)(BitFieldRead64(RtShiftAddress, 0, 0));
          break;
        case 2:             //3-Way
          RouteTableIndex = (UINT8)(BitFieldRead64(ModU64x32 (RtShiftAddress, 3), 0, 1));
          break;
        case 3:             //4-Way
          RouteTableIndex = (UINT8)(BitFieldRead64(RtShiftAddress, 0, 1));
          break;
        default:
          RouteTableIndex = 0xFF;
          break;
      }
      TDBG("RouteTableIndex: 0x%x \n", RouteTableIndex);
      return RouteTableIndex;
    }
  }
}


/**
  Determines TargetId and ChannelId that maps given system Address
  Modifies the following fields in ADDRESS_TRANSATION structure:
    SocketId
    MemoryControllerId
    ChannelId

 @param  [in/out] pTranslatedAddress   -- pointer to the structure containing DIMM Address

  @retval EFI_SUCCESS                   SAD translation is successful
          EFI_INVALID_PARAMETER         SAD translation is not successful


**/
EFI_STATUS
EFIAPI
TranslateSad (
  IN dimmBDFst *pdimmBdf,
  IN TRANSLATED_ADDRESS * const TranslatedAddress,
  OUT UINT8 *SktId,
  OUT UINT8 *McId,
  OUT UINT8 *ChId
 )
{
  UINT64    SystemAddress;
  UINT64    SadLimit;
  UINT64    PrevSadLimit;
  BOOLEAN   Match = FALSE;
  UINT8     Skt;
  UINT8     SadIndex;
  UINT8     InterleaveListIndex;
  UINT8     TargetId;
  UINT8     TargetHalf;
  UINT8     McRouteTableType;
  UINT8     McRouteTableIndex;
  UINT8     McTargetId;
  UINT8     McChId;
  UINT8     ChRouteTableType;
  UINT8     ChRouteTableIndex;
  UINT8     SktFromRemoteSad;
  UINT8     Cha;
  DRAM_RULE_CFG_0_CHABC_SAD_STRUCT ChaDramRule;
  BLOCK_DECODER_EN_CFG_0_CHABC_SAD1_STRUCT   BlockDecoderEnCfg;
  endPointRegInfo reg = {
          .msgType         = MESSAGE_LOCAL_PCI,
          .addrType        = PCI_BDF_REG,
          .optType         = OPTION_DWORD, /* 4 bytes */
          .dimmBdf.cpuType = pdimmBdf->cpuType,
          .dimmBdf.socket  = pdimmBdf->socket,
          .dimmBdf.channel = pdimmBdf->channel,
          .dimmBdf.bus     = BUS_NUMBER_CHA,
          .device          = 29, /* page-3502 $15.4.73 */
          .function        = 0, /* Function = 0 @ CHA $15.3.33-64 registers */
          .offset          = DRAM_RULE_CFG_0_CHABC_SAD_REG /* 4 byte size CHA Register */
  };

  /*
   * Refer to log of mc.addTRan()
   * 3) Show core address map
   */

  SystemAddress = TranslatedAddress->SystemAddress;
  SadIndex = 0;
  TargetId = 0;
  McTargetId = 0;
  McChId = 0;
  SktFromRemoteSad = 0;
  Cha = 0;

  if (!(IsDramAddress (SystemAddress))) {
    TCRIT("Address doesn't belong to DRAM SAD\n");
    return EFI_INVALID_PARAMETER;
  }

  if (IsDdrtBlockAddress (pdimmBdf, TranslatedAddress)) {

    *SktId = (UINT8)BitFieldRead64(SystemAddress, 16, 18);
    *McId = (UINT8)BitFieldRead64(SystemAddress, 12, 13);
    *ChId = (UINT8)BitFieldRead64(SystemAddress, 14, 15);

    if ((mSocketPresentBitMap & (BIT0  << *SktId))  && (*McId < MAX_IMC) && (*ChId < MAX_MC_CH)) {
        reg.optType         = OPTION_WORD;
        reg.function        = 1;
        reg.offset          = BLOCK_DECODER_EN_CFG_0_CHABC_SAD1_REG; /* 2 byte size CHA Register */
        BlockDecoderEnCfg.Data = (UINT16) ReadCpuCsrPciLocal(&reg);

        if (BlockDecoderEnCfg.Bits.block_mc_ch_en & (1 << (*ChId * 4 + *McId))) {
        return EFI_SUCCESS;
      }
        /* Restore configuration for DRAM_RULE_CFG_0_CHABC_SAD_REG */
        reg.optType         = OPTION_DWORD;
        reg.function        = 0;
    }

    TCRIT("SktId %x, McId %x, ChId%x doesn't exist\n", *SktId, *McId, *ChId);
    return EFI_INVALID_PARAMETER;
  }

  SktFromRemoteSad = GetSktIdFromRemoteSad (pdimmBdf, SystemAddress);
  TDBG("SktFromRemoteSad = %d\n", SktFromRemoteSad);

  //for (Skt = 0; Skt < MAX_SOCKET; Skt++)
  Skt = pdimmBdf->socket; /* changl change */
  {

    if (SktFromRemoteSad != 0xFF) {
      Skt = SktFromRemoteSad;
    }

    if ((mSocketPresentBitMap & (BIT0  << Skt)) == 0) {
      //continue;
      TCRIT("Socket 0x%x is not present\n", Skt);
      return EFI_INVALID_PARAMETER;

    }
    TDBG("Socket 0x%x is present\n", Skt);

    PrevSadLimit = 0;
    reg.dimmBdf.socket = Skt;
    for (SadIndex = 0; SadIndex < SAD_RULES_10NM; SadIndex++) {
      //reg.offset = mDramRuleCfgCha[SadIndex];
      //ChaDramRule.Data = ReadCpuCsrPciLocal(&reg);
      ChaDramRule.Data = mDramRuleCfgChaCache[Skt][pdimmBdf->channel%MAX_MC_CH][SadIndex];

      TDBG("ChaDramRule.Data for SadIndex 0x%x in Skt 0x%x is 0x%x", SadIndex, Skt, ChaDramRule.Data);
      TDBG("[PECI] - mDramRuleCfgCha[%d]: 0x%x", SadIndex, ChaDramRule.Data);

      if (ChaDramRule.Bits.rule_enable != 1) {
        continue;
      }

      SadLimit = (((UINT64)ChaDramRule.Bits.limit + 1) << SAD_UNIT);
      TDBG("ChaDramRule.Bits.limit+1 is 0x%x\n", ChaDramRule.Bits.limit+1);
      //
      // Find the SAD rule that holds the given SystemAddress
      //
      TDBG("%s() PrevSadLimit 0x%llx, SystemAddress 0x%llx, SadLimit 0x%llx\n",
    		  __FUNCTION__, PrevSadLimit, SystemAddress, SadLimit);
      if ((PrevSadLimit <= SystemAddress) && (SystemAddress < SadLimit))
      {
        //
        // Get the InterleaveList index based on interleave_mode
        //
        InterleaveListIndex = GetInterleaveListIndex ((UINT8)ChaDramRule.Bits.interleave_mode, SystemAddress);
        TDBG("InterleaveListIndex is 0x%x\n", InterleaveListIndex);

        //
        // Get the TargetId from InterleaveListCfg register
        //
        Match = GetTargetId (pdimmBdf, Skt, InterleaveListIndex, SadIndex, &TargetId);
        TranslatedAddress->SadId = SadIndex;
        if (Match == TRUE) {
          if(Skt == pdimmBdf->socket) /* changl add */
          break;
        }
      }

      PrevSadLimit = SadLimit;
    }
#if 0
    if ((SktFromRemoteSad != 0xFF) || (Match == TRUE)) {
      if(Skt == pdimmBdf->socket) /* changl add */
      break;
    }
#endif
  }

  if (!Match) {
    TCRIT("0x%llx doesn't belong to any of the sockets\n", SystemAddress);
    return EFI_INVALID_PARAMETER;
  }

  //
  // Get the half_id
  //
  TargetHalf  = TargetId & 1;

  //
  // Get the RouteTableType for MC Target
  //
  McRouteTableType = GetRouteTableType (pdimmBdf, Skt, SadIndex, MC_ROUTE_TABLE);

   switch (McRouteTableType) {
    case RT0:
    case RT2:
      TranslatedAddress->MemType = MemType1lmAppDirect;
      TranslatedAddress->DimmType = ddrtdimmType;
      break;
    case RT1:
      TranslatedAddress->MemType = MemType1lmDdr;
      TranslatedAddress->DimmType = ddr4dimmType;
      break;
    case RT2LM:
      TranslatedAddress->MemType = MemType2lmDdrCacheMemoryMode;
      TranslatedAddress->DimmType = ddrtdimmType;
      break;
    default:
      break;
   }

  //
  //  Get the Index to RouteTable
  //
  McRouteTableIndex = GetIndexToRouteTable (pdimmBdf, Skt, SadIndex, TargetId, McRouteTableType, SystemAddress);

  if (McRouteTableIndex == 0xFF) {
    TCRIT("Error in calculating McRouteTableIndex for 0x%llx", SystemAddress);
    return EFI_INVALID_PARAMETER;
  }

  //
  //
  // Get the SNC/Cluster

  if (GetChaForSystemAddress (pdimmBdf, Skt, SystemAddress, &Cha) != EFI_SUCCESS) {
    TCRIT("Error in finding Cluster for 0x%llx", SystemAddress);
    return EFI_INVALID_PARAMETER;
  }

  //
  // Get McTargetId based on TargetHalf, RouteTable and RouteTable Index
  //
  McTargetId = GetMcTargetId (pdimmBdf, Skt, TargetHalf, Cha, McRouteTableType, McRouteTableIndex);

  //
  // Get the RouteTable Type for Channel
  //
  ChRouteTableType = GetRouteTableType (pdimmBdf, Skt, SadIndex, CH_ROUTE_TABLE);

  //
  // Get the RouteTableIndex for Channel
  //
  if (ChRouteTableType == McRouteTableType) {
    ChRouteTableIndex = McRouteTableIndex;
  } else {
    ChRouteTableIndex = GetIndexToRouteTable (pdimmBdf, Skt, SadIndex, TargetId, ChRouteTableType, SystemAddress);
  }

  if (ChRouteTableIndex == 0xFF) {
    TCRIT("Error in calculating ChRouteTableIndex for 0x%llx", SystemAddress);
    return EFI_INVALID_PARAMETER;
  }

  //
  //Get the ChannelId based on TargettHalf,RouteTable and RouteTableIndex
  //
  McChId = GetMcChId (pdimmBdf, Skt, TargetHalf, Cha, ChRouteTableType, ChRouteTableIndex);

  //
  // Correct the McTargetId and McChannelId for FM
  //
  if (ChaDramRule.Bits.nm_cacheable) { //nm_cacheable: Specifies whether or not this address range is cacheable in near memory
    TranslatedAddress->NmMemoryControllerId = McTargetId;
    TranslatedAddress->NmChannelId = McChId;
    Correct2LMTgtIdChId (pdimmBdf, SystemAddress, Skt, &McTargetId, &McChId);
  }

  *SktId           = Skt;
  *McId            = McTargetId;
  *ChId            = McChId;
  TDBG("SktId: %d, McId; %d, ChId: %d", Skt, McTargetId, McChId);

  return EFI_SUCCESS;
}


/**

  The constructor function initialize Address Decode Libary.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
InitAddressDecodeLib (
  char nrCPU
  )
{
	FILE *fKey = NULL;
	struct addrTrParam_T addrTr = {0};
	int to = BIOS_ADDR_TRAN_TIMEOUT;
	int data_rx = 0;
	EFI_STATUS ret;

	endPointRegInfo reg = {
	        .msgType         = MESSAGE_LOCAL_PCI,
	        .addrType        = PCI_BDF_REG,
	        .optType         = OPTION_DWORD, /* 4 bytes */
	        .dimmBdf.cpuType = 0,
	        .dimmBdf.socket  = 0,
	        .dimmBdf.channel = 0,
	        .dimmBdf.bus     = BUS_NUMBER_CHA,
	        .device          = 29, /* page-3502 $15.4.73 */
	        .function        = 0, /* Function = 0 @ CHA $15.3.33-64 registers */
	        .offset          = DRAM_RULE_CFG_0_CHABC_SAD_REG /* 4 byte size CHA Register */
	};

    while (to > 0) {
		if ( access( ADDR_TRANS_INFO, F_OK ) == 0 ) {
			TDBG("File: %s is available\n", ADDR_TRANS_INFO);
			fKey = fopen(ADDR_TRANS_INFO,"rb");
			if(fKey == NULL) {
				TCRIT("Unable to create %s\n", ADDR_TRANS_INFO);
				sleep(1);;
			} else {
				if(sizeof(struct addrTrParam_T) ==
					fread(&addrTr, 1, sizeof(struct addrTrParam_T), fKey))
				{
					data_rx = 1;
				} else {
					TCRIT("Unable to read %s\n", ADDR_TRANS_INFO);
				}
				/* mMmCfgLimit = mMmCfgBase + UdsHobPtr->PlatformData.PciExpressSize */
				/* mTotCha     = UdsHobPtr->PlatformData.CpuQpiInfo[0].TotCha */
				fclose(fKey);
				break ;
			}
		} else {
			TWARN("Elapsed time to read Address translation parameter from IPMI: %d sec", BIOS_ADDR_TRAN_TIMEOUT - to);
			sleep(5);
			to -= 5;
		}
    }

    if(data_rx) {
		TCRIT("Succeeded in reading %s\n", ADDR_TRANS_INFO);
		ret = EFI_SUCCESS;
    mSocketPresentBitMap  = addrTr.mSocketPresentBitMap;
    mMmCfgBase            = addrTr.mMmCfgBase;
    mMmCfgLimit           = addrTr.mMmCfgLimit;
    mPlatGlobalMmiolBase  = addrTr.mPlatGlobalMmiolBase;
    mPlatGlobalMmiolLimit = addrTr.mPlatGlobalMmiolLimit;
    mPlatGlobalMmiohBase  = addrTr.mPlatGlobalMmiohBase;
    mPlatGlobalMmiohLimit = addrTr.mPlatGlobalMmiohLimit;
    mTotCha               = addrTr.mTotCha;
    } else{
		TCRIT("Failed to read %s. Use default data\n", ADDR_TRANS_INFO);
		ret = EFI_LOAD_ERROR;
        mSocketPresentBitMap  = DEFAULT_BIOS_SKT_PRESENT;
        mMmCfgBase            = DEFAULT_BIOS_MM_CFG_BASE;
        mMmCfgLimit           = DEFAULT_BIOS_MM_CFG_LIMIT;
        mPlatGlobalMmiolBase  = DEFAULT_BIOS_MMIO_L_BASE;
        mPlatGlobalMmiolLimit = DEFAULT_BIOS_MMIO_L_LIMIT;
        mPlatGlobalMmiohBase  = DEFAULT_BIOS_MMIO_H_BASE;
        mPlatGlobalMmiohLimit = DEFAULT_BIOS_MMIO_H_LIMIT;
        mTotCha               = DEFAULT_BIOS_TOT_CHA;
    }

    TDBG("mMmCfgBase = 0x%llx, \t", mMmCfgBase);
    TDBG("mMmCfgLimit = 0x%llx\n", mMmCfgLimit);
    TDBG("mPlatGlobalMmiolBase = 0x%llx, \t", mPlatGlobalMmiolBase);
    TDBG("mPlatGlobalMmiolLimit = 0x%llx\n", mPlatGlobalMmiolLimit);
    TDBG("mPlatGlobalMmiohBase = 0x%llx, \t", mPlatGlobalMmiohBase);
    TDBG("mPlatGlobalMmiohLimit = 0x%llx\n", mPlatGlobalMmiohLimit);
    TDBG("mSocketPresentBitMap = 0x%lx", mSocketPresentBitMap);
    TDBG("mTotCha = 0x%lx\n", mTotCha);

    for(int i=0; i<nrCPU/*MAX_SOCKET*/; ++i) {
        reg.dimmBdf.socket  = i;
    	for(int j=0; j<MAX_MC_CH; ++j) {
            reg.dimmBdf.channel = j;
            for(int k=0; k<SAD_RULES_10NM; ++k) {
                reg.offset = mDramRuleCfgCha[k];
                mDramRuleCfgChaCache[i][j][k] = ReadCpuCsrPciLocal(&reg);
            }
    	}
    }
#if defined(DEBUG)
    for(int i=0; i<nrCPU/*MAX_SOCKET*/; ++i) {
        printf("socket: %d\n", i);
    	for(int j=0; j<MAX_MC_CH; ++j) {
            printf("\tchannel: %d\n\t\t", j);
            for(int k=0; k<SAD_RULES_10NM; ++k) {
                printf("0x%04x ", mDramRuleCfgChaCache[i][j][k]);
            }
            printf("\n");
    	}
    }
#endif
    return ret;
}


#ifdef ADDRESS_TRANSLATION_FORWARD
/**
  Address calculation for XOR mode. Calculation formula,
    a. In case cluster XOR enabled by checking ddr_half_xor_mode_enable [21:21].
      i. In case two-way interleave:
          B[11] = A[11] ^ A[8] ^ A[17] ^ A[25]
      ii.In case four-way interleave:
          B[11] = A[11] ^ A[8] ^ A[17] ^ A[25]
          B[12] = A[12] ^ A[9] ^ A[18] ^ A[26]
    b. In case channel XOR enabled by checking ddr_xor_mode_enable [20:20]
      i. In case two-way interleave:
        B[8] = A[8] ^ A[14] ^ A[22]
      ii. In case four-way interleave:
        B[8] = A[8] ^ A[14] ^ A[22]
        B[9] = A[9] ^ A[15] ^ A[23]
      iii. In case eight-way interleave:
        B[8] = A[8] ^ A[14] ^ A[22]
        B[9] = A[9] ^ A[15] ^ A[23]
        B[10] = A[10] ^ A[16] ^ A[24]

  @param[in]  PhyAddr   Original physical address
  @param[in]  ClusterXorModeEn   Cluster XOR mode enable
  @param[in]  ChXorModeEn        Channel XOR mode enable
  @param[in]  TargetWays         Target interleave ways
  @param[in]  ChannelWays        Channel interleave ways

  @retval       calculated address

**/
UINT64
EFIAPI
XorModeAddressCalc (
  IN  UINT64      PhyAddr,
  IN  UINT32      ClusterXorModeEn,
  IN  UINT32      ChXorModeEn,
  IN  UINT8       TargetWays,
  IN  UINT8       ChannelWays
  )
{
  UINT64 PhyAddrB;

  PhyAddrB = PhyAddr;

  if (ClusterXorModeEn) {
    switch (TargetWays) {
      case 2:                 //2-Way
        PhyAddrB = BitFieldWrite64 (PhyAddrB, 11, 11,
                (BitFieldRead64 (PhyAddr, 11, 11) ^
                BitFieldRead64 (PhyAddr, 8, 8) ^
                BitFieldRead64 (PhyAddr, 17, 17) ^
                BitFieldRead64 (PhyAddr, 25, 25)));
        break;
      case 4:                 //4-Way
        PhyAddrB = BitFieldWrite64 (PhyAddrB, 11, 11,
                (BitFieldRead64 (PhyAddr, 11, 11) ^
                BitFieldRead64 (PhyAddr, 8, 8) ^
                BitFieldRead64 (PhyAddr, 17, 17) ^
                BitFieldRead64 (PhyAddr, 25, 25)));
        PhyAddrB = BitFieldWrite64 (PhyAddrB, 12, 12,
                (BitFieldRead64 (PhyAddr, 12, 12) ^
                BitFieldRead64 (PhyAddr, 9, 9) ^
                BitFieldRead64 (PhyAddr, 18, 18) ^
                BitFieldRead64 (PhyAddr, 26, 26)));
        break;
      default:
        break;
    }
  }
  if (ChXorModeEn) {
    switch (ChannelWays) {
      case 2:                 //2-Way
        PhyAddrB = BitFieldWrite64 (PhyAddrB, 8, 8,
                (BitFieldRead64 (PhyAddr, 8, 8) ^
                BitFieldRead64 (PhyAddr, 14, 14) ^
                BitFieldRead64 (PhyAddr, 22, 22)));
        break;
      case 4:                 //4-Way
        PhyAddrB = BitFieldWrite64 (PhyAddrB, 8, 8,
                (BitFieldRead64 (PhyAddr, 8, 8) ^
                BitFieldRead64 (PhyAddr, 14, 14) ^
                BitFieldRead64 (PhyAddr, 22, 22)));
        PhyAddrB = BitFieldWrite64 (PhyAddrB, 9, 9,
                (BitFieldRead64 (PhyAddr, 9, 9) ^
                BitFieldRead64 (PhyAddr, 15, 15) ^
                BitFieldRead64 (PhyAddr, 23, 23)));
        break;
      case 8:                 //8-Way
        PhyAddrB = BitFieldWrite64 (PhyAddrB, 8, 8,
                (BitFieldRead64 (PhyAddr, 8, 8) ^
                BitFieldRead64 (PhyAddr, 14, 14) ^
                BitFieldRead64 (PhyAddr, 22, 22)));
        PhyAddrB = BitFieldWrite64 (PhyAddrB, 9, 9,
                (BitFieldRead64 (PhyAddr, 9, 9) ^
                BitFieldRead64 (PhyAddr, 15, 15) ^
                BitFieldRead64 (PhyAddr, 23, 23)));
        PhyAddrB = BitFieldWrite64 (PhyAddrB, 10, 10,
                (BitFieldRead64 (PhyAddr, 10, 10) ^
                BitFieldRead64 (PhyAddr, 16, 16) ^
                BitFieldRead64 (PhyAddr, 24, 24)));
        break;
      default:
        break;
    }
  }

  return PhyAddrB;
}

EFI_STATUS
EFIAPI
GetNmChannelAddress (
  IN     dimmBDFst *pdimmBdf,
  IN OUT TRANSLATED_ADDRESS * const TranslatedAddress
  )
{
  
  MCNMCACHINGINTLV_MC_2LM_STRUCT    NmCachingIntLv;
  MCNMCACHINGOFFSET_MC_2LM_STRUCT   NmCachingOffsetData;
  MCNMCACHINGCFG_MC_2LM_STRUCT      NmCachingCfg;
  //UINT8     SktId;
  UINT8     McTgtId;
  UINT8     McChId;
  UINT8     SktCh;
  UINT8     NmTgtWays;
  UINT8     NmChnWays;
  UINT8     NmTgtGran;
  UINT8     NmChGran;
  UINT64    NmChOffset;
  UINT64    NmChCap;
  UINT8     NmRatioChnCap;
  UINT8     TagIndex;
  UINT8     NmTargetBitWide;
  UINT8     NmChnBitWide;
  UINT8     NmCapBitWide;
  UINT64    NmCap = 0;
  UINT8     BlockNum;
  UINT64    SystemAddress;
  UINT64    NmChannelAddress;
  UINT64    TempAddr;

  endPointRegInfo reg = {
          .msgType         = MESSAGE_MMIO,
          .addrType        = BAR_32_BITS,
          .optType         = OPTION_DWORD,
          .dimmBdf.cpuType = pdimmBdf->cpuType,
          .dimmBdf.socket  = pdimmBdf->socket,
          .dimmBdf.channel = pdimmBdf->channel,
          .dimmBdf.bus     = pdimmBdf->bus,
          .device          = pdimmBdf->imc + DEVICE_OF_FIRST_IMC,
          .function        = FUNCTION_IMC_MMIO,
          .offset          = MCNMCACHINGINTLV_MC_2LM_REG /* Page-2863 $13.3.8: 4 byte size */
  };

  SystemAddress = TranslatedAddress->SystemAddress;
  //SktId         = TranslatedAddress->SocketId;
  McTgtId       = TranslatedAddress->NmMemoryControllerId;
  McChId        = TranslatedAddress->NmChannelId;

  SktCh = (McTgtId * MAX_MC_CH) + McChId;
  reg.dimmBdf.channel = SktCh;

  //TCRIT("\nTranslatedAddress->NmMemoryControllerId is 0x%lx",TranslatedAddress->NmMemoryControllerId);
  //TCRIT("TranslatedAddress->NmChannelId is 0x%lx",TranslatedAddress->NmChannelId);

  NmCachingIntLv.Data = ReadCpuCsrMmio(&reg); // SktId, SktCh
  reg.offset = MCNMCACHINGOFFSET_MC_2LM_REG;
  NmCachingOffsetData.Data = ReadCpuCsrMmio(&reg);
  reg.offset = MCNMCACHINGCFG_MC_2LM_REG;
  NmCachingCfg.Data = ReadCpuCsrMmio(&reg);
  //TDBG("[GetNmCaching] NM caching Cfg:%x  OffData:%x  IntLv:%x \n", NmCachingCfg.Data, NmCachingOffsetData.Data, NmCachingIntLv.Data);
  
  NmTgtWays = 1 << NmCachingIntLv.Bits.nm_target_ways;
  NmChnWays = 1 << NmCachingIntLv.Bits.nm_chn_ways;
  NmTgtGran = GetInterleaveBit ((UINT8)NmCachingIntLv.Bits.nm_target_gran);
  NmChGran =  GetInterleaveBit ((UINT8)NmCachingIntLv.Bits.nm_chn_gran);
  //TDBG("GetNmChannelAddress: NmTgtWays:%d NmChnWays:%d \n", NmTgtWays, NmChnWays);
  //TDBG("GetNmChannelAddress: NmTgtGran:%d NmChGran:%d \n", NmTgtGran, NmChGran);

  if (NmCachingOffsetData.Bits.mcnmcachingoffseten) {
    NmChOffset = NmCachingOffsetData.Bits.mcnmcachingoffset << CONVERT_64MB_TO_32GB_GRAN;
  } else {
    NmChOffset = 0;
  }
  //TDBG("GetNmChannelAddress: NmChOffset:0x%lx\n",NmChOffset);

  switch (NmCachingCfg.Bits.chn_cap) {
    case 0:
      NmChCap = 0x100000000;
      break;
    case 1:
      NmChCap = 0x200000000;
      break;
    case 2:
      NmChCap = 0x400000000;
      break;
    case 3:
      NmChCap = 0x800000000;
      break;
    case 4:
      NmChCap = 0x1000000000;
      break;
    case 5:
      NmChCap = 0x2000000000;
      break;
    case 6:
      NmChCap = 0x4000000000;
      break;
    case 7:
      NmChCap = 0x8000000000;
      break;
    case 8:
      NmChCap = 0x10000000;
      break;
    case 9:
      NmChCap = 0x20000000;
      break;
    case 10:
      NmChCap = 0x40000000;
      break;
    default:
      return EFI_INVALID_PARAMETER;
  }

  //TDBG("GetNmChannelAddress: NmChCap:0x%lx\n",NmChCap);

  if (NmCachingCfg.Bits.nm_ratio_chn_cap == 0 ) {
    NmRatioChnCap = 1;
  } else if (NmCachingCfg.Bits.nm_ratio_chn_cap == 1 ) {
    NmRatioChnCap = 32;
  } else if (NmCachingCfg.Bits.nm_ratio_chn_cap == 2 ) {
    NmRatioChnCap = 16;
  } else if (NmCachingCfg.Bits.nm_ratio_chn_cap == 3 ) {
    NmRatioChnCap = 8;
  } else if (NmCachingCfg.Bits.nm_ratio_chn_cap == 4 ) {
    NmRatioChnCap = 4;
  } else if (NmCachingCfg.Bits.nm_ratio_chn_cap == 5 ) {
    NmRatioChnCap = 2;
  } else if ((NmCachingCfg.Bits.nm_ratio_chn_cap >= 33 ) && ( NmCachingCfg.Bits.nm_ratio_chn_cap <= 63)) {
    NmRatioChnCap = (UINT8)NmCachingCfg.Bits.nm_ratio_chn_cap;
  } else {
    NmRatioChnCap = 1;
    //TCRIT("GetNmChannelAddress: Wrong NM cap ratio:%d \n", NmCachingCfg.Bits.nm_ratio_chn_cap);
    return EFI_INVALID_PARAMETER;
  }

  //TDBG("GetNmChannelAddress: NmRatioChnCap:0x%lx\n",NmRatioChnCap);
  
  NmChannelAddress = SystemAddress - NmChOffset;
  
  //TDBG("GetNmChannelAddress: NmChannelAddress - 0ffset:0x%lx\n",NmChannelAddress);


  NmTargetBitWide = (UINT8)HighBitSet64 (NmTgtWays);
  NmChnBitWide = (UINT8)HighBitSet64 (NmChnWays);

  if (NmRatioChnCap <= 32) {
    NmCap = DivU64x32 (NmChCap, NmRatioChnCap);  //it is power_of_2.
    NmCapBitWide = (UINT8)HighBitSet64 (NmCap);
    TagIndex = NmCapBitWide + NmTargetBitWide + NmChnBitWide;
  } else {
    NmCapBitWide = (UINT8)HighBitSet64 (NmChCap);
    TagIndex = NmCapBitWide + NmTargetBitWide + NmChnBitWide;
    BlockNum = (UINT8) BitFieldRead64 (NmChannelAddress, TagIndex - 5, TagIndex - 1);
    if (BlockNum <= NmRatioChnCap - 32) {
      TagIndex --;
    }
  }

  //TDBG("GetNmChannelAddress: TagIndex:%d NmWide:%d   NmChannelAddress:%lx\n", TagIndex, NmCapBitWide, NmChannelAddress);
  NmChannelAddress = BitFieldRead64 (NmChannelAddress, 0, TagIndex -1);
  //TDBG("GetNmChannelAddress: NmChannelAddress extracted for TagIndex:0x%lx\n",NmChannelAddress);

  TempAddr = DivU64x32 (RShiftU64 (NmChannelAddress, NmTgtGran), NmTgtWays);
  NmChannelAddress = LShiftU64 (TempAddr, NmTgtGran) | BitFieldRead64 (NmChannelAddress, 0, NmTgtGran-1);

  TempAddr = DivU64x32 (RShiftU64 (NmChannelAddress, NmChGran), NmChnWays);
  NmChannelAddress = LShiftU64 (TempAddr, NmChGran) | BitFieldRead64 (NmChannelAddress, 0, NmChGran-1);

  TranslatedAddress->NmChannelAddress = NmChannelAddress;
  //TCRIT("TranslatedAddress->NmChannelAddress is 0x%lx\n",NmChannelAddress);
  return EFI_SUCCESS;
}



EFI_STATUS
EFIAPI
TranslateTad (
  IN     dimmBDFst *pdimmBdf,
  IN OUT TRANSLATED_ADDRESS * const TranslatedAddress
  )
{

  UINT64    SystemAddress;
  UINT64    ChannelAddress;
  UINT64    TempAddr;
  UINT64    TadOffset;
  UINT8     TgtInterleaveBit;
  UINT8     ChInterleaveBit;
  UINT8     TgtWays;
  UINT8     ChWays;
  //UINT8     SktId;
  UINT8     McTgtId;
  UINT8     McChId;
  UINT8     SktCh;
  UINT8     TadIndex;
  BOOLEAN   Match;
  UINT8     Index;
  UINT64    PrevSad2TadLimit;
  UINT64    Sad2TadLimit;
  UINT8     MirrPri;
  UINT8     SecCh;

  TADCHNILVOFFSET_0_N0_MC_MAIN_STRUCT               TadChnIlvData0;
  TADCHNILVOFFSET_0_N1_MC_MAIN_STRUCT               TadChnIlvData1;
  TAD_RD_N0_M2MEM_MAIN_STRUCT                       MeshN0TadRd;
  TAD_RD_N1_M2MEM_MAIN_STRUCT                       MeshN1TadRd;
  MIRRORFAILOVER_M2MEM_MAIN_STRUCT                  MirrorFailover;
  MIRRORCHNLS_M2MEM_MAIN_STRUCT                     MirrorChannels;


  UINT32 TadChnIlvOffsetN0[MAX_TAD_RULES_10NM] = {
    TADCHNILVOFFSET_0_N0_MC_MAIN_REG, TADCHNILVOFFSET_1_N0_MC_MAIN_REG, TADCHNILVOFFSET_2_N0_MC_MAIN_REG, TADCHNILVOFFSET_3_N0_MC_MAIN_REG,
    TADCHNILVOFFSET_4_N0_MC_MAIN_REG, TADCHNILVOFFSET_5_N0_MC_MAIN_REG, TADCHNILVOFFSET_6_N0_MC_MAIN_REG, TADCHNILVOFFSET_7_N0_MC_MAIN_REG,
    TADCHNILVOFFSET_8_N0_MC_MAIN_REG, TADCHNILVOFFSET_9_N0_MC_MAIN_REG, TADCHNILVOFFSET_10_N0_MC_MAIN_REG, TADCHNILVOFFSET_11_N0_MC_MAIN_REG
  };

  UINT32 TadChnIlvOffsetN1[MAX_TAD_RULES_10NM] = {
    TADCHNILVOFFSET_0_N1_MC_MAIN_REG, TADCHNILVOFFSET_1_N1_MC_MAIN_REG, TADCHNILVOFFSET_2_N1_MC_MAIN_REG, TADCHNILVOFFSET_3_N1_MC_MAIN_REG,
    TADCHNILVOFFSET_4_N1_MC_MAIN_REG, TADCHNILVOFFSET_5_N1_MC_MAIN_REG, TADCHNILVOFFSET_6_N1_MC_MAIN_REG, TADCHNILVOFFSET_7_N1_MC_MAIN_REG,
    TADCHNILVOFFSET_8_N1_MC_MAIN_REG, TADCHNILVOFFSET_9_N1_MC_MAIN_REG, TADCHNILVOFFSET_10_N1_MC_MAIN_REG, TADCHNILVOFFSET_11_N1_MC_MAIN_REG
  };

  endPointRegInfo reg_pci = {
          .msgType         = MESSAGE_LOCAL_PCI,
          .addrType        = PCI_BDF_REG,
          .optType         = OPTION_DWORD/*OPTION_QWORD*/,
          .dimmBdf.cpuType = pdimmBdf->cpuType,
          .dimmBdf.socket  = pdimmBdf->socket,
          .dimmBdf.channel = pdimmBdf->channel,
          .dimmBdf.bus     = BUS_NUMBER_PCI30, /* differ from the bus of IMC */
          .device          = pdimmBdf->imc + DEVICE_IMC_PCI, /* page-2718 $13.1.43 */
          .function        = 0,
          .offset          = TAD_RD_N0_M2MEM_MAIN_REG
  };

  endPointRegInfo reg_mmio = {
          .msgType         = MESSAGE_MMIO,
          .addrType        = BAR_32_BITS, /* BAR_64_BITS: KO */
          .optType         = OPTION_QWORD /* peciapp: OK */,
          .dimmBdf.cpuType = pdimmBdf->cpuType,
          .dimmBdf.socket  = pdimmBdf->socket,
          .dimmBdf.channel = pdimmBdf->channel,
          .dimmBdf.bus     = pdimmBdf->bus,
          .device          = pdimmBdf->imc + DEVICE_OF_FIRST_IMC,
          .function        = FUNCTION_IMC_MMIO,
          .offset          = TADCHNILVOFFSET_0_N0_MC_MAIN_REG /* Page-2898 $13.4.7: 8 byte size */
  };

  SystemAddress = TranslatedAddress->SystemAddress;
  //SktId         = TranslatedAddress->SocketId;
  McTgtId       = TranslatedAddress->MemoryControllerId;
  McChId        = TranslatedAddress->ChannelId;
  Match         = FALSE;

  TranslatedAddress->SecChannelId = 0xFF;
  SecCh = 0;

  PrevSad2TadLimit = 0;
  ChWays = 1;

  for (Index = 0; Index < INTERNAL_TAD_RULES; Index++) {

    //
    // Read Internal Tad Table
    //
    MeshN0TadRd.Data = 0;
    MeshN0TadRd.Bits.tadid = Index;
    MeshN0TadRd.Bits.tadrden = 1;
    reg_pci.offset = TAD_RD_N0_M2MEM_MAIN_REG;
    WriteCpuCsrPciLocal(&reg_pci, MeshN0TadRd.Data); // SktId, McTgtId
    MeshN0TadRd.Data = ReadCpuCsrPciLocal(&reg_pci);
    reg_pci.offset = TAD_RD_N1_M2MEM_MAIN_REG;
    MeshN1TadRd.Data = ReadCpuCsrPciLocal(&reg_pci);

    //TDBG("\nMeshN1TadRd.Bits.addresslimit:0x%x", MeshN1TadRd.Bits.addresslimit);
    //TDBG("\nMeshN0TadRd.Bits.addresslimit:0x%x", MeshN0TadRd.Bits.addresslimit);

    Sad2TadLimit = LShiftU64 ((LShiftU64 (MeshN1TadRd.Bits.addresslimit, 6) | MeshN0TadRd.Bits.addresslimit) + 1, SAD_UNIT);
    //TDBG("\nPrevSad2TadLimit is 0x%lx for Tad 0x%x", PrevSad2TadLimit, Index);
    //TDBG("\nSad2TadLimit is 0x%lx for Tad 0x%x\n", Sad2TadLimit, Index);

    if (((TranslatedAddress->DimmType == ddr4dimmType) && !(MeshN0TadRd.Bits.ddr4)) || ((TranslatedAddress->DimmType == ddrtdimmType) && (MeshN0TadRd.Bits.ddr4))) {
      //TDBG("\nDimmType doesn't match to memory type from TAD Rule\n");
      PrevSad2TadLimit = Sad2TadLimit;
      continue;
    }

    if (PrevSad2TadLimit == Sad2TadLimit) {
      //TCRIT("System Address cannot be translated\n");
      break;
    }

    if ((PrevSad2TadLimit <= SystemAddress) && (SystemAddress < Sad2TadLimit)) {
      TadIndex =  (UINT8)MeshN0TadRd.Bits.ddrtadid;
      Match = TRUE;
      break;
    }
  }

  if (Match == FALSE) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Add support for Mirroring
  //
  if (MeshN0TadRd.Bits.mirror) {
    reg_pci.offset = MIRRORFAILOVER_M2MEM_MAIN_REG; // dev 12, func 0
    MirrorFailover.Data = ReadCpuCsrPciLocal(&reg_pci);
    reg_pci.offset = MIRRORCHNLS_M2MEM_MAIN_REG; // dev 12, func 0
    MirrorChannels.Data = ReadCpuCsrPciLocal(&reg_pci);

    if (McChId == 0)  {
      SecCh = (UINT8)MirrorChannels.Bits.ddr4chnl0secondary;
    }
    if (McChId == 1)  {
      SecCh = (UINT8)MirrorChannels.Bits.ddr4chnl1secondary;
    }
    if (McChId == 2)  {
      SecCh = (UINT8)MirrorChannels.Bits.ddr4chnl2secondary;
    }

    //
    // Check if mirror failed over
    //
    if ((MirrorFailover.Bits.ddr4chnlfailed >> McChId) & 1) {
      TranslatedAddress->ChannelId = SecCh;
      MirrPri = 0;
    } else {
      TranslatedAddress->SecChannelId = SecCh;
      MirrPri = 1;
    }
  }

  for (Index = 0; Index < 2; Index++) {
    if (Index == 0) {
      SktCh = (McTgtId * MAX_MC_CH) + TranslatedAddress->ChannelId;
    } 
    
    if (Index == 1) {
      if (TranslatedAddress->SecChannelId != 0xFF) {
        //TCRIT("TranslatedAddress->SecChannelId is 0x%lx", TranslatedAddress->SecChannelId);
        SktCh = (McTgtId * MAX_MC_CH) + TranslatedAddress->SecChannelId;
        MirrPri = 0;
      } else {
        break;
      }
    }

    reg_mmio.dimmBdf.channel = SktCh;
    reg_mmio.offset          = TadChnIlvOffsetN0[TadIndex];
    TadChnIlvData0.Data = ReadCpuCsrMmio(&reg_mmio); // SktId, SktCh
    reg_mmio.offset          = TadChnIlvOffsetN1[TadIndex];
    TadChnIlvData1.Data = ReadCpuCsrMmio(&reg_mmio); // SktId, SktCh

    TadOffset = LShiftU64 ((LShiftU64 (TadChnIlvData1.Bits.tad_offset, 18) | TadChnIlvData0.Bits.tad_offset), SAD_UNIT);
    //TDBG("\nTadOffset is 0x%lx\n", TadOffset);

    if (TadChnIlvData1.Bits.tad_offset_sign == 0) {
      ChannelAddress = SystemAddress - TadOffset;
    } else {
      ChannelAddress = SystemAddress + TadOffset;
    }

    TgtInterleaveBit = GetInterleaveBit ((UINT8)TadChnIlvData0.Bits.target_gran);
    ChInterleaveBit =  GetInterleaveBit ((UINT8)TadChnIlvData0.Bits.chn_gran);
    TgtWays = (UINT8) (1 << TadChnIlvData0.Bits.target_ways);
    switch (TadChnIlvData0.Bits.chn_ways) {
      case 0: ChWays = 1; break;
      case 1: ChWays = 2; break;
      case 2: ChWays = 3; break;
      case 3: ChWays = 8; break;
      case 4: ChWays = 4; break;
      default: break;
    }

    TranslatedAddress->TargetWays = TgtWays;
    TranslatedAddress->ChannelWays = ChWays;

    //TDBG("\nTgtInterleaveBit:0x%x , ChInterleaveBit:0x%x", TgtInterleaveBit, ChInterleaveBit);
    //TDBG("\nTgtWays:0x%x, ChWays:0x%x\n", TgtWays, ChWays);

    TempAddr = DivU64x32 (RShiftU64 (ChannelAddress, TgtInterleaveBit), TgtWays);
    ChannelAddress = LShiftU64 (TempAddr, TgtInterleaveBit) | BitFieldRead64 (ChannelAddress, 0, TgtInterleaveBit-1);

    TempAddr = DivU64x32 (RShiftU64 (ChannelAddress, ChInterleaveBit), ChWays);
    ChannelAddress = LShiftU64 (TempAddr, ChInterleaveBit) | BitFieldRead64 (ChannelAddress, 0, ChInterleaveBit-1);

    if (MeshN0TadRd.Bits.mirror) {
      TempAddr = BitFieldRead64 (ChannelAddress, 0, 17);
      ChannelAddress = RShiftU64 (ChannelAddress, 18);
      ChannelAddress = LShiftU64 (ChannelAddress , 1);
      ChannelAddress   |= (UINT64) (MirrPri ^ 1);  // 0 for primary, 1 for secondary
      ChannelAddress = LShiftU64 (ChannelAddress , 18) | TempAddr;
    }
    if (Index == 0) {
      TranslatedAddress->ChannelAddress = ChannelAddress;
      TranslatedAddress->SecChannelAddress = (UINT64)-1;
      //TCRIT("\nTranslatedAddress->ChannelAddress is 0x%lx", ChannelAddress);
    } else {
      TranslatedAddress->SecChannelAddress = ChannelAddress;
      //TCRIT("TranslatedAddress->SecChannelAddress 0x%lx", ChannelAddress);
    }
  }

  if (TranslatedAddress->MemType == MemType2lmDdrCacheMemoryMode) {
    GetNmChannelAddress (pdimmBdf, TranslatedAddress);
  }

  return EFI_SUCCESS;
}


 /**
 Translates ChannelAddress to get RankAddress and PhysicalRankId

 @param  [in/out] pTranslatedAddress   -- pointer to the structure containing DIMM Address

  @retval EFI_SUCCESS                   RIR translation is successful
          EFI_INVALID_PARAMETER         RIR translation is not successful


**/
EFI_STATUS
EFIAPI
TranslateRir (
  IN     dimmBDFst *pdimmBdf,
  IN OUT TRANSLATED_ADDRESS * const TranslatedAddress
 )
{

  UINT64    RankAddress;
  UINT64    RirOffset;
  UINT64    PrevRirLimit;
  UINT64    RirLimit;
  UINT8     SktId;
  UINT8     McTgtId;
  UINT8     SktCh;
  UINT8     RirIndex;
  UINT8     RirWays;
  UINT8     PageShift;
  UINT8     RankInterleave;
  BOOLEAN   Match;
  UINT32    ClosePg;
  UINT64    ChannelAddress;
  UINT8     Index;

  RIRWAYNESSLIMIT_0_MC_MAIN_STRUCT  RirWayness;
  RIRILV0OFFSET_0_MC_MAIN_STRUCT    RirIlv0Offset;
  FMRIRWAYNESSLIMIT_0_MC_2LM_STRUCT FmRirWayness;
  FMRIRILV0OFFSET_0_MC_2LM_STRUCT   FmRirIlv0Offset;

  UINT32 RirWaynessLimit[MAX_RIR] =
    { RIRWAYNESSLIMIT_0_MC_MAIN_REG, RIRWAYNESSLIMIT_1_MC_MAIN_REG, RIRWAYNESSLIMIT_2_MC_MAIN_REG, RIRWAYNESSLIMIT_3_MC_MAIN_REG } ;

  UINT32 RirIlvOffset[MAX_RIR][MAX_RIR_WAYS] =
    { { RIRILV0OFFSET_0_MC_MAIN_REG, RIRILV1OFFSET_0_MC_MAIN_REG, RIRILV2OFFSET_0_MC_MAIN_REG, RIRILV3OFFSET_0_MC_MAIN_REG, RIRILV4OFFSET_0_MC_MAIN_REG, RIRILV5OFFSET_0_MC_MAIN_REG, RIRILV6OFFSET_0_MC_MAIN_REG, RIRILV7OFFSET_0_MC_MAIN_REG },
      { RIRILV0OFFSET_1_MC_MAIN_REG, RIRILV1OFFSET_1_MC_MAIN_REG, RIRILV2OFFSET_1_MC_MAIN_REG, RIRILV3OFFSET_1_MC_MAIN_REG, RIRILV4OFFSET_1_MC_MAIN_REG, RIRILV5OFFSET_1_MC_MAIN_REG, RIRILV6OFFSET_1_MC_MAIN_REG, RIRILV7OFFSET_1_MC_MAIN_REG },
      { RIRILV0OFFSET_2_MC_MAIN_REG, RIRILV1OFFSET_2_MC_MAIN_REG, RIRILV2OFFSET_2_MC_MAIN_REG, RIRILV3OFFSET_2_MC_MAIN_REG, RIRILV4OFFSET_2_MC_MAIN_REG, RIRILV5OFFSET_2_MC_MAIN_REG, RIRILV6OFFSET_2_MC_MAIN_REG, RIRILV7OFFSET_2_MC_MAIN_REG },
      { RIRILV0OFFSET_3_MC_MAIN_REG, RIRILV1OFFSET_3_MC_MAIN_REG, RIRILV2OFFSET_3_MC_MAIN_REG, RIRILV3OFFSET_3_MC_MAIN_REG, RIRILV4OFFSET_3_MC_MAIN_REG, RIRILV5OFFSET_3_MC_MAIN_REG, RIRILV6OFFSET_3_MC_MAIN_REG, RIRILV7OFFSET_3_MC_MAIN_REG } };

  UINT32 FmRirWaynessLimit[MAX_RIR_DDRT] =
    { FMRIRWAYNESSLIMIT_0_MC_2LM_REG, FMRIRWAYNESSLIMIT_1_MC_2LM_REG, FMRIRWAYNESSLIMIT_2_MC_2LM_REG, FMRIRWAYNESSLIMIT_3_MC_2LM_REG };

  UINT32 FmRirIlvOffset[MAX_RIR_DDRT][MAX_RIR_DDRT_WAYS] =
    { { FMRIRILV0OFFSET_0_MC_2LM_REG, FMRIRILV1OFFSET_0_MC_2LM_REG },
    { FMRIRILV0OFFSET_1_MC_2LM_REG, FMRIRILV1OFFSET_1_MC_2LM_REG },
    { FMRIRILV0OFFSET_2_MC_2LM_REG, FMRIRILV1OFFSET_2_MC_2LM_REG },
    { FMRIRILV0OFFSET_3_MC_2LM_REG, FMRIRILV1OFFSET_3_MC_2LM_REG } };

  endPointRegInfo reg_mmio = {
          .msgType         = MESSAGE_MMIO,
          .addrType        = BAR_32_BITS, /* BAR_64_BITS: KO */
          .optType         = OPTION_QWORD /* peciapp: OK */,
          .dimmBdf.cpuType = pdimmBdf->cpuType,
          .dimmBdf.socket  = pdimmBdf->socket,
          .dimmBdf.channel = pdimmBdf->channel,
          .dimmBdf.bus     = pdimmBdf->bus,
          .device          = pdimmBdf->imc + DEVICE_OF_FIRST_IMC,
          .function        = FUNCTION_IMC_MMIO,
          .offset          = TADCHNILVOFFSET_0_N0_MC_MAIN_REG /* Page-2898 $13.4.7: 8 byte size */
  };
  dimmBDFst dimmBdf;

  SktId         = TranslatedAddress->SocketId;
  McTgtId       = TranslatedAddress->MemoryControllerId;
  SktCh = (McTgtId * MAX_MC_CH) + TranslatedAddress->ChannelId;
  ChannelAddress = TranslatedAddress->ChannelAddress;
  reg_mmio.dimmBdf.channel = SktCh;

  if (TranslatedAddress->MemType == MemType1lmDdr || TranslatedAddress->MemType == MemType2lmDdrCacheMemoryMode) {

    for (Index = 0; Index < 2; Index++) {

      if (TranslatedAddress->MemType == MemType2lmDdrCacheMemoryMode) {
        McTgtId       = TranslatedAddress->NmMemoryControllerId;
        SktCh = (McTgtId * MAX_MC_CH) + TranslatedAddress->NmChannelId;
        ChannelAddress = TranslatedAddress->NmChannelAddress;
        reg_mmio.dimmBdf.channel = SktCh;
      }

      if (Index == 1) {
        if (TranslatedAddress->SecChannelAddress != (UINT64)-1) {
          //TDBG("\nDecoding for mirroring support TranslatedAddress->SecChannelId: 0x%x\n", TranslatedAddress->SecChannelId);
          //TDBG("\nDecoding for mirroring support TranslatedAddress->SecChannelAddress 0x%x\n", TranslatedAddress->SecChannelAddress);
          SktCh = (McTgtId * MAX_MC_CH) + TranslatedAddress->SecChannelId;
          ChannelAddress = TranslatedAddress->SecChannelAddress;
          reg_mmio.dimmBdf.channel = SktCh;
        } else {
          break;
        }
      } 

      Match         = FALSE;
      RirWays       = 0;
      PrevRirLimit  = 0;

      for (RirIndex = 0; RirIndex < MAX_RIR; RirIndex++) {
        reg_mmio.offset = RirWaynessLimit[RirIndex];
        RirWayness.Data = ReadCpuCsrMmio(&reg_mmio);
        RirLimit = (((UINT64)RirWayness.Bits.rir_limit + 1) << RIR_UNIT);
        //TDBG("\nRIRWaynessLimit.Data is 0x%x\n", RirWayness.Data);
        //TDBG("\nRirLimit is 0x%lx\n", RirLimit);

        if ((PrevRirLimit <= ChannelAddress) && (ChannelAddress < RirLimit)) {
          RirWays = 1 << RirWayness.Bits.rir_way;
          Match = TRUE;
          break;
        }
        PrevRirLimit = RirLimit;
      }

      if (Match == FALSE) {
        return EFI_INVALID_PARAMETER;
      }
      memcpy(&dimmBdf, pdimmBdf, sizeof(dimmBDFst));
      dimmBdf.channel     = SktCh;
      GetMcmtr (&dimmBdf, SktId, McTgtId, SktCh, &ClosePg, NULL, NULL, NULL); //Socket, Mc, ChOnSkt, mcmtr.close_page, mcmtr.bank_xor_enable

      PageShift = (UINT8)(ClosePg == 1 ? RANK_INTL_MIN_CLOSE : RANK_INTL_MIN_OPEN);
      //TDBG("\nMcMtr.Bits.close_pg is 0x%x. PageShift = 0x%x", ClosePg, PageShift);

      RankInterleave = (UINT8)(ModU64x32 (RShiftU64(ChannelAddress , PageShift) , RirWays));
      //TDBG("\nRirWays = 0x%x. RankInterleave = 0x%x", RirWays, RankInterleave);

      reg_mmio.offset = RirIlvOffset[RirIndex][RankInterleave];
      RirIlv0Offset.Data = ReadCpuCsrMmio(&reg_mmio);
      RirOffset = (UINT64)(RirIlv0Offset.Bits.rir_offset0) << 26;
      //TDBG("\nRirOffset is 0x%lx\n", RirOffset);

      RankAddress = DivU64x32 (RShiftU64 (ChannelAddress , PageShift) , RirWays);
      RankAddress = LShiftU64 (RankAddress , PageShift) | BitFieldRead64 (ChannelAddress, 0, PageShift-1);
      RankAddress = RankAddress - RirOffset;
      

      if (Index == 0) {
        if (TranslatedAddress->MemType == MemType2lmDdrCacheMemoryMode) {
          TranslatedAddress->NmRankAddress = RankAddress;
          TranslatedAddress->NmChipSelect = (UINT8)RirIlv0Offset.Bits.rir_rnk_tgt0;
          TranslatedAddress->NmPhysicalRankId = TranslatedAddress->NmChipSelect % MAX_RANK_DIMM;
          TranslatedAddress->NmDimmSlot = TranslatedAddress->NmChipSelect >> 2;
          //TCRIT("TranslatedAddress->NmRankAddress is 0x%lx", TranslatedAddress->NmRankAddress);
          //TCRIT("TranslatedAddress->NmChipSelect is 0x%x", TranslatedAddress->NmChipSelect);
          //TCRIT("TranslatedAddress->NmPhysicalRankId is 0x%x", TranslatedAddress->NmPhysicalRankId);
          //TCRIT("TranslatedAddress->NmDimmSlot is 0x%x", TranslatedAddress->NmDimmSlot);
        } else {
          TranslatedAddress->RankAddress = RankAddress;
          TranslatedAddress->ChipSelect = (UINT8)RirIlv0Offset.Bits.rir_rnk_tgt0;
          TranslatedAddress->PhysicalRankId = TranslatedAddress->ChipSelect % MAX_RANK_DIMM;
          TranslatedAddress->DimmSlot = TranslatedAddress->ChipSelect >> 2;
          //TCRIT("TranslatedAddress->RankAddress is 0x%lx", TranslatedAddress->RankAddress);
          //TCRIT("TranslatedAddress->ChipSelect is 0x%x", TranslatedAddress->ChipSelect);
          //TCRIT("TranslatedAddress->PhysicalRankId is 0x%x", TranslatedAddress->PhysicalRankId);
          //TCRIT("TranslatedAddress->DimmSlot is 0x%x", TranslatedAddress->DimmSlot);
        }
      } else {
        TranslatedAddress->SecRankAddress = RankAddress;
        TranslatedAddress->SecChipSelect = (UINT8)RirIlv0Offset.Bits.rir_rnk_tgt0;
        TranslatedAddress->SecPhysicalRankId = TranslatedAddress->SecChipSelect % MAX_RANK_DIMM;
        TranslatedAddress->SecDimmSlot = TranslatedAddress->SecChipSelect >> 2;
        //TCRIT("TranslatedAddress->SecRankAddress is 0x%lx", TranslatedAddress->SecRankAddress);
        //TCRIT("TranslatedAddress->SecChipSelect is 0x%x", TranslatedAddress->SecChipSelect);
        //TCRIT("TranslatedAddress->SecPhysicalRankId is 0x%x", TranslatedAddress->SecPhysicalRankId);
        //TCRIT("TranslatedAddress->SecDimmSlot is 0x%x", TranslatedAddress->SecDimmSlot);
      }
    }
  }
  if (!(TranslatedAddress->MemType == MemType1lmDdr)) {
    PrevRirLimit = 0;
    McTgtId       = TranslatedAddress->MemoryControllerId;
    SktCh = (McTgtId * MAX_MC_CH) + TranslatedAddress->ChannelId;
    reg_mmio.dimmBdf.channel = SktCh;
    ChannelAddress = TranslatedAddress->ChannelAddress;
    Match = FALSE;
    for (RirIndex = 0; RirIndex < MAX_RIR_DDRT; RirIndex++) {
      reg_mmio.offset = FmRirWaynessLimit[RirIndex];
      FmRirWayness.Data = ReadCpuCsrMmio(&reg_mmio);
      RirLimit = (((UINT64)FmRirWayness.Bits.rir_limit + 1) << RIR_UNIT);
      //TDBG("\nRIRWaynessLimit.Data is 0x%x\n", FmRirWayness.Data);
      //TDBG("\nRirLimit is 0x%lx\n", RirLimit);

      if ((PrevRirLimit <= ChannelAddress) && (ChannelAddress < RirLimit)) {
        RirWays = 1 << FmRirWayness.Bits.rir_way;
        Match = TRUE;
        break;
      }
      PrevRirLimit = RirLimit;
    }

    if (Match == FALSE) {
      return EFI_INVALID_PARAMETER;
    }

    dimmBdf.channel     = SktCh;
    GetMcmtr (&dimmBdf, SktId, McTgtId, SktCh, &ClosePg, NULL, NULL, NULL); //Socket, Mc, ChOnSkt, mcmtr.close_page, mcmtr.bank_xor_enable

    PageShift = (UINT8)(ClosePg == 1 ? RANK_INTL_MIN_CLOSE : RANK_INTL_MIN_OPEN);
    //TDBG("\nMcMtr.Bits.close_pg is 0x%x. PageShift = 0x%x", ClosePg, PageShift);

    RankInterleave = (UINT8)(ModU64x32 (RShiftU64(ChannelAddress , PageShift) , RirWays));
    //TDBG("\nRirWays = 0x%x. RankInterleave = 0x%x", RirWays, RankInterleave);

    reg_mmio.offset = FmRirIlvOffset[RirIndex][RankInterleave];
    FmRirIlv0Offset.Data = ReadCpuCsrMmio(&reg_mmio);
    RirOffset = (UINT64)(FmRirIlv0Offset.Bits.rir_offset0) << 26;
    //TDBG("\nRirOffset is 0x%lx\n", RirOffset);

    RankAddress = DivU64x32 (RShiftU64 (ChannelAddress , PageShift) , RirWays);
    RankAddress = LShiftU64 (RankAddress , PageShift) | BitFieldRead64 (ChannelAddress, 0, PageShift-1);
    RankAddress = RankAddress - RirOffset;
    //TDBG("\nRankAddress is 0x%lx\n", RankAddress);

    TranslatedAddress->DPA = RankAddress;
    TranslatedAddress->ChipSelect = (UINT8)FmRirIlv0Offset.Bits.rir_rnk_tgt0;
    TranslatedAddress->PhysicalRankId = TranslatedAddress->ChipSelect % MAX_RANK_DIMM;
    TranslatedAddress->DimmSlot = TranslatedAddress->ChipSelect >> 2;
    //TCRIT("\nTranslatedAddress->ChipSelect is 0x%x", TranslatedAddress->ChipSelect);
    //TCRIT("TranslatedAddress->PhysicalRankId is 0x%x", TranslatedAddress->PhysicalRankId);
    //TCRIT("TranslatedAddress->DimmSlot is 0x%x", TranslatedAddress->DimmSlot);
    //TCRIT("TranslatedAddress->DPA is 0x%x", TranslatedAddress->DPA);
  }

  return EFI_SUCCESS;
}


/**
 Translates RankAddress to get DRAM info

 @param  [in/out] pTranslatedAddress   -- pointer to the structure containing DIMM Address

  @retval EFI_SUCCESS                   Rank Address translation is successful
          EFI_INVALID_PARAMETER         Rank Address translation is not successful


**/
EFI_STATUS
EFIAPI
TranslateRankAddress (
  IN     dimmBDFst *pdimmBdf,
  IN OUT TRANSLATED_ADDRESS * const TranslatedAddress
 )
{

  UINT64    RankAddress;
  UINT8     SktId;
  UINT8     McTgtId;
  UINT8     SktCh;
  UINT8     DimmSlot;
  UINT8     LowPos;
  UINT8     HighPos;
  UINT8     Bank;
  UINT8     BankGroup;
  UINT8     ChipId;
  UINT32    Row;
  UINT32    Col;
  UINT32    ClosePg;
  UINT32    BankXorEn;
  UINT8     Index;

  DIMMMTR_0_MC_MAIN_STRUCT          DimmMtrReg;
  AMAP_MC_MAIN_STRUCT               AmapReg;

  UINT32 DimmMtrOffset[MAX_DIMM] =
    {DIMMMTR_0_MC_MAIN_REG, DIMMMTR_1_MC_MAIN_REG};

  endPointRegInfo reg = {
          .msgType         = MESSAGE_MMIO,
          .addrType        = BAR_32_BITS, /* BAR_64_BITS: KO */
          .optType         = OPTION_DWORD,
          .dimmBdf.cpuType = pdimmBdf->cpuType,
          .dimmBdf.socket  = pdimmBdf->socket,
          .dimmBdf.channel = pdimmBdf->channel,
          .dimmBdf.bus     = pdimmBdf->bus,
          .device          = pdimmBdf->imc + DEVICE_OF_FIRST_IMC,
          .function        = FUNCTION_IMC_MMIO,
          .offset          = DIMMMTR_0_MC_MAIN_REG /* Page-2892 $13.4.2: 4 byte size */
  };
  dimmBDFst dimmBdf;


  if (TranslatedAddress->MemType == MemType1lmAppDirect) {
    return EFI_SUCCESS;
  }

  SktId         = TranslatedAddress->SocketId;

  if (TranslatedAddress->MemType == MemType2lmDdrCacheMemoryMode) {
    McTgtId       = TranslatedAddress->NmMemoryControllerId;
    SktCh = (McTgtId * MAX_MC_CH) + TranslatedAddress->NmChannelId;
    RankAddress = TranslatedAddress->NmRankAddress;
    DimmSlot = TranslatedAddress->NmDimmSlot;
  } else {
    RankAddress   = TranslatedAddress->RankAddress;
    McTgtId       = TranslatedAddress->MemoryControllerId;
    DimmSlot      = TranslatedAddress->DimmSlot;
    SktCh         = (McTgtId * MAX_MC_CH) + TranslatedAddress->ChannelId;
  }
  memcpy(&dimmBdf, pdimmBdf, sizeof(dimmBDFst));
  dimmBdf.channel     = SktCh;

  for (Index = 0; Index < 2; Index++) {
        
    if (Index == 1) {
      if (TranslatedAddress->SecChannelAddress != (UINT64)-1) {
        //TDBG("\nDecoding for mirroring support TranslatedAddress->SecChannelId: 0x%x\n", TranslatedAddress->SecChannelId);
        //TDBG("\nDecoding for mirroring support TranslatedAddress->SecChannelAddress 0x%x\n", TranslatedAddress->SecChannelAddress);
        SktCh = (McTgtId * MAX_MC_CH) + TranslatedAddress->SecChannelId;
        RankAddress = TranslatedAddress->SecRankAddress;
        DimmSlot = TranslatedAddress->SecDimmSlot;
        dimmBdf.channel     = SktCh;
      } else {
        break;
      }
    } 
      
    ChipId  = 0;
    Row = 0;
    Col = 0;
    Bank = 0;
    BankGroup = 0;

    GetMcmtr (&dimmBdf, SktId, McTgtId, SktCh, &ClosePg, &BankXorEn, NULL, NULL); //Socket, Mc, ChOnSkt, mcmtr.close_page, mcmtr.bank_xor_enable
    reg.offset = DimmMtrOffset[DimmSlot];
    DimmMtrReg.Data = ReadCpuCsrMmio(&reg);
    reg.offset = AMAP_MC_MAIN_REG;
    AmapReg.Data = ReadCpuCsrMmio(&reg);

    if (!DimmMtrReg.Bits.dimm_pop) {
      return EFI_INVALID_PARAMETER;
    }

    if ((DimmMtrReg.Bits.rank_disable >> TranslatedAddress->PhysicalRankId) & 0x01) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // for 3DS DIMMs, Row bits are relocated to upper rank address
    //
    if (DimmMtrReg.Bits.ddr4_3dsnumranks_cs != 0) {
      Row = (UINT32)BitFieldRead64(RankAddress, 31, 31);

      if (DimmMtrReg.Bits.ddr4_3dsnumranks_cs > 1){
        Row |= (BitFieldRead64(RankAddress, 32, 32) << 1);
      }

      if (DimmMtrReg.Bits.ddr4_3dsnumranks_cs > 2){
        Row |= (BitFieldRead64(RankAddress, 33, 33) << 2);
      }
    }

    if (ClosePg == 1) {
      //
      // Build BankGroup
      //
      BankGroup = (UINT8) BitFieldRead64 (RankAddress, 6, 6);
	    if (!(DimmMtrReg.Bits.ba_shrink)) {
        BankGroup |= (BitFieldRead64(RankAddress, 7, 7) << 1);
	    }

      //
      // Build Bank
      //
      Bank = (UINT8) BitFieldRead64 (RankAddress, 8, 9);

      if (BankXorEn == 1) {
        BankGroup ^= (UINT8) BitFieldRead64 (RankAddress, 20, 20);
        BankGroup ^= (UINT8)(BitFieldRead64(RankAddress, 21, 21) << 1);
        Bank ^= (UINT8) BitFieldRead64 (RankAddress, 22, 22);
        Bank ^= (UINT8) (BitFieldRead64 (RankAddress, 18, 18) << 1);
      }

      //
      // Build column
      //
      Col = (UINT32)BitFieldRead64(RankAddress, 3, 5);
      Col |= (BitFieldRead64 (RankAddress, 19, 19) << 3);
      Col |= (BitFieldRead64 (RankAddress, 14, 14) << 4);
      Col |= (BitFieldRead64 (RankAddress, 23, 27) << 5);

      //
      // Build Row and ChipId
      //
      if (DimmMtrReg.Bits.ddr4_3dsnumranks_cs == 0) {
        Row = (UINT32)BitFieldRead64(RankAddress, 17, 17);
      } else {
        ChipId = (UINT8)BitFieldRead64(RankAddress, 17, 17);
      }

      if (DimmMtrReg.Bits.ddr4_3dsnumranks_cs > 1) {
        ChipId |= (BitFieldRead64(RankAddress, 15, 15) << 1);
      } else {
        Row |= (BitFieldRead64(RankAddress, 15, 15) << 1);
      }

      if (DimmMtrReg.Bits.ddr4_3dsnumranks_cs > 2) {
        ChipId |= (BitFieldRead64(RankAddress, 16, 16) << 2);
      } else {
        Row |= (BitFieldRead64(RankAddress, 16, 16) << 2);
      }

      Row |= (BitFieldRead64 (RankAddress, 20, 22) << 3);
      Row |= (BitFieldRead64 (RankAddress, 10, 13) << 6);
      Row |= (BitFieldRead64 (RankAddress, 18, 18) << 10);
      Row |= (BitFieldRead64 (RankAddress, 28, 28) << 11);
      if (DimmMtrReg.Bits.ba_shrink) {
        Row |= (BitFieldRead64(RankAddress, 7, 7) << 12);
      } else {
        Row |= (BitFieldRead64(RankAddress, 30, 30) << 12);
      }
      Row |= (BitFieldRead64(RankAddress, 29, 29) << 13);

    } else { // Open page
      //
      // Build BankGroup
      //
      if (AmapReg.Bits.cgbg_interleave == 0) {
        BankGroup = (UINT8)BitFieldRead64(RankAddress, 13, 13);
        if (!(DimmMtrReg.Bits.ba_shrink)) {
          BankGroup |= (BitFieldRead64(RankAddress, 17, 17) << 1);
        }
      } else if (AmapReg.Bits.cgbg_interleave == 1) {
        BankGroup = (UINT8)BitFieldRead64(RankAddress, 6, 6);
        if (!(DimmMtrReg.Bits.ba_shrink)) {
          BankGroup |= (BitFieldRead64(RankAddress, 17, 17) << 1);
        } 
      } else {
          BankGroup = (UINT8)BitFieldRead64(RankAddress, 6, 6);
        if (!(DimmMtrReg.Bits.ba_shrink)) {
          BankGroup |= (BitFieldRead64(RankAddress, 7, 7) << 1);
        }
      }
    

      //
      // Build Bank
      //
      Bank = (UINT8) BitFieldRead64 (RankAddress, 18, 19);

      if (BankXorEn == 1) {
        BankGroup ^= (UINT8)BitFieldRead64(RankAddress, 20, 21);
        Bank ^= (UINT8) BitFieldRead64 (RankAddress, 22, 23);
      }

      //
      // Build Col Address
      //

      Col = (UINT32)BitFieldRead64(RankAddress, 3, 5);
      if (AmapReg.Bits.cgbg_interleave == 0) {
        Col |= (BitFieldRead64(RankAddress, 6, 12) << 3);
      } else if (AmapReg.Bits.cgbg_interleave == 1) {
        Col |= (BitFieldRead64(RankAddress, 13, 13) << 3);
        Col |= (BitFieldRead64(RankAddress, 7, 12) << 4);
      } else {
        Col |= (BitFieldRead64(RankAddress, 13, 14) << 3);
        Col |= (BitFieldRead64(RankAddress, 8, 12) << 5);
      }
   

      //
      // Build Row and ChipId
      //
      if (DimmMtrReg.Bits.ddr4_3dsnumranks_cs == 0) {
        if (AmapReg.Bits.cgbg_interleave == 2) {
          Row = (UINT32)BitFieldRead64(RankAddress, 17, 17);
        } else {
          Row = (UINT32)BitFieldRead64(RankAddress, 14, 14);
        }
      } else {
        if (AmapReg.Bits.cgbg_interleave == 2) {
          ChipId = (UINT8)BitFieldRead64(RankAddress, 17, 17);
        } else {
          ChipId = (UINT8)BitFieldRead64(RankAddress, 14, 14);
        }
      }

      if (DimmMtrReg.Bits.ddr4_3dsnumranks_cs > 1) {
        ChipId |= (BitFieldRead64(RankAddress, 15, 15) << 1);
      } else {
        Row |= (BitFieldRead64(RankAddress, 15, 15) << 1);
      }

      if (DimmMtrReg.Bits.ddr4_3dsnumranks_cs > 2) {
        ChipId |= (BitFieldRead64(RankAddress, 16, 16) << 2);
      } else {
        Row |= (BitFieldRead64(RankAddress, 16, 16) << 2);
      }

      Row |= (BitFieldRead64(RankAddress, 20, 28) << 3);
    
      if (DimmMtrReg.Bits.ba_shrink) {
        if (AmapReg.Bits.cgbg_interleave == 2) {
          Row |= (BitFieldRead64(RankAddress, 7, 7) << 12);
         } else {
          Row |= (BitFieldRead64(RankAddress, 17, 17) << 12);
         }
      } else {
      Row |= (BitFieldRead64(RankAddress, 30, 30) << 12);
      }
    
      Row |= (BitFieldRead64(RankAddress, 29, 29) << 13);
  
    }


    LowPos = (UINT8)(RIR_UNIT + 2 - DimmMtrReg.Bits.ba_shrink + DimmMtrReg.Bits.ddr4_3dsnumranks_cs);
    HighPos = (UINT8)(LowPos + (DimmMtrReg.Bits.ra_width - 2));
    Row |= (BitFieldRead64(RankAddress, LowPos, HighPos) << 14);

    if (RShiftU64 (RankAddress, (HighPos+1))) {
      return EFI_INVALID_PARAMETER;
    }

    if (Index == 0) {
      TranslatedAddress->Col = Col;
      TranslatedAddress->Row = Row;
      TranslatedAddress->Bank = Bank;
      TranslatedAddress->BankGroup = BankGroup;
      TranslatedAddress->ChipId = ChipId;

      //TCRIT("\nTranslatedAddress->Col is 0x%x", TranslatedAddress->Col);
      //TCRIT("TranslatedAddress->Row is 0x%x", TranslatedAddress->Row);
      //TCRIT("TranslatedAddress->Bank is 0x%x", TranslatedAddress->Bank);
      //TCRIT("TranslatedAddress->BankGroup is 0x%x", TranslatedAddress->BankGroup);
      //TCRIT("TranslatedAddress->ChipId is 0x%x", TranslatedAddress->ChipId);
    } else {
      TranslatedAddress->SecCol = Col;
      TranslatedAddress->SecRow = Row;
      TranslatedAddress->SecBank = Bank;
      TranslatedAddress->SecBankGroup = BankGroup;
      TranslatedAddress->SecChipId = ChipId;

      //TCRIT("\nTranslatedAddress->SecCol is 0x%x", TranslatedAddress->SecCol);
      //TCRIT("TranslatedAddress->SecRow is 0x%x", TranslatedAddress->SecRow);
      //TCRIT("TranslatedAddress->SecBank is 0x%x", TranslatedAddress->SecBank);
      //TCRIT("TranslatedAddress->SecBankGroup is 0x%x", TranslatedAddress->SecBankGroup);
      //TCRIT("TranslatedAddress->SecChipId is 0x%x", TranslatedAddress->SecChipId);

    }
 
  }
  return EFI_SUCCESS;
}

/**

   Translate System Address to DIMM Address

    @param  [in]  SystemAddress  - System Address
    @param  [out] pTranslatedAddress   -- pointer to the structure containing DIMM Address

    @retval EFI_SUCCESS / Error code


**/
EFI_STATUS
EFIAPI
TranslateSystemAddress (
  IN  dimmBDFst *pdimmBdf,
  IN  UINTN SystemAddress,
  OUT PTRANSLATED_ADDRESS TranslatedAddress
)
{
  EFI_STATUS                  Status;
  UINT8                       SktId, McId, ChId;

  // Initialize  the fields in TA with -1
  TranslatedAddress->SystemAddress      = (UINT64)SystemAddress;
  TranslatedAddress->SocketId           = (UINT8)-1;
  TranslatedAddress->MemoryControllerId = (UINT8)-1;
  TranslatedAddress->ChannelId          = (UINT8)-1;
  TranslatedAddress->DimmSlot           = (UINT8)-1;
  TranslatedAddress->PhysicalRankId     = (UINT8)-1;
  TranslatedAddress->Row                = (UINT32)-1;
  TranslatedAddress->Col                = (UINT32)-1;
  TranslatedAddress->Bank               = (UINT8)-1;
  TranslatedAddress->BankGroup          = (UINT8)-1;
  TranslatedAddress->ChipSelect         = (UINT8)-1;
  TranslatedAddress->ChipId             = (UINT8)-1;

  //TCRIT("Forward Address Translation start for 0x%lx\n", TranslatedAddress->SystemAddress);

  Status = TranslateSad (pdimmBdf, TranslatedAddress, &SktId, &McId, &ChId);
  if (EFI_ERROR (Status)) {
    //TCRIT("TranslateSad is failing\n");
    return EFI_INVALID_PARAMETER;
  }

  //TDBG("\nTranslateSad is successful\n");
  TranslatedAddress->SocketId = SktId;
  TranslatedAddress->MemoryControllerId = McId;
  TranslatedAddress->ChannelId = ChId;
  //TCRIT("TranslatedAddress->SocketId is 0x%x", TranslatedAddress->SocketId);
  //TCRIT("TranslatedAddress->MemoryControllerId is 0x%x", TranslatedAddress->MemoryControllerId);
  //TCRIT("TranslatedAddress->ChannelId is 0x%x", TranslatedAddress->ChannelId);

  Status = TranslateTad (pdimmBdf, TranslatedAddress);
  if (EFI_ERROR (Status)) {
    //TCRIT("TranslateTad is failing\n");
    return EFI_INVALID_PARAMETER;
  }
  //TDBG("\nTranslateTAD is successful\n");

  Status = TranslateRir (pdimmBdf, TranslatedAddress);
  if (EFI_ERROR (Status)) {
    //TCRIT("TranslateRir is failing\n");
    return EFI_INVALID_PARAMETER;
  }
  //TDBG("\nTranslateRir is successful\n");

  Status = TranslateRankAddress (pdimmBdf, TranslatedAddress);
  if (EFI_ERROR (Status)) {
    //TCRIT("TranslateRankAddress is failing\n");
    return EFI_INVALID_PARAMETER;
  }
  //TDBG("\nTranslateRankAddress is successful\n");

  return Status;

}

/**
  Translate a system address to a DIMM address.

  @param[in]      SystemAddress       System address to be translated
  @param[out]     TranslatedAddress   Pointer to the DIMM Address structure

  @retval EFI_SUCCESS                 Translation was successful
  @retval EFI_INVALID_PARAMETER       System address was invalid
**/
EFI_STATUS
EFIAPI
SystemAddressToDimmAddress (
  IN      dimmBDFst *pdimmBdf,
  IN      UINTN               SystemAddress,
  OUT     PTRANSLATED_ADDRESS TranslatedAddress
  )
{

  EFI_STATUS Status;

  Status = TranslateSystemAddress (pdimmBdf, SystemAddress, TranslatedAddress);

  if (Status == EFI_SUCCESS) {
    //TCRIT("Forward Address Translation Successful!!\n");
  } else {
    //TCRIT("Forward Address Translation is not Successful!!\n");
  }

  return Status;

}
#endif /* ADDRESS_TRANSLATION_FORWARD */


#ifdef CHECK_SGX_MEMORY
/**
Check if the address belongs to SGX memory location

@param[in] SktId  - Socket Id
@param[out] McId  - Memory Controller Id

@retval     TRUE  - If the address belongs to SGX Memory
@retval     FALSE   If the address doesn't belongs to SGX Memory

**/
BOOLEAN
EFIAPI
CheckSgxMemory(
  IN  dimmBDFst *pdimmBdf,
  UINT64      PhyAddr,
  UINT8       SktId,
  UINT8       McId
)
{
  BOOLEAN                                 IsSgxMem = FALSE;
  UINT64                                  PrmrrBaseAddress;
  UINT64                                  PrmrrMask;
  M2M_PRMRR_BASE_N0_M2MEM_MAIN_STRUCT     PrmrrBaseN0Struct;
  M2M_PRMRR_BASE_N1_M2MEM_MAIN_STRUCT     PrmrrBaseN1Struct;
  M2M_PRMRR_MASK_N0_M2MEM_MAIN_STRUCT     PrmrrMaskN0Struct;
  M2M_PRMRR_MASK_N1_M2MEM_MAIN_STRUCT     PrmrrMaskN1Struct;
  endPointRegInfo reg = {
          .msgType         = MESSAGE_LOCAL_PCI,
          .addrType        = PCI_BDF_REG,
          .optType         = OPTION_DWORD/*OPTION_QWORD*/,
          .dimmBdf.cpuType = pdimmBdf->cpuType,
          .dimmBdf.socket  = pdimmBdf->socket,
          .dimmBdf.channel = pdimmBdf->channel,
          .dimmBdf.bus     = BUS_NUMBER_PCI30,
          .device          = pdimmBdf->imc + DEVICE_IMC_PCI, /* 12 @ page-2779 $13.1.96 */
          .function        = 0,
          .offset          = M2M_PRMRR_BASE_N0_M2MEM_MAIN_REG
  };

  PrmrrBaseN0Struct.Data = ReadCpuCsrPciLocal(&reg);
  reg.offset = M2M_PRMRR_MASK_N0_M2MEM_MAIN_REG;
  PrmrrMaskN0Struct.Data = ReadCpuCsrPciLocal(&reg);

  if ((PrmrrBaseN0Struct.Bits.enable) && (PrmrrMaskN0Struct.Bits.lock)) {
    reg.offset = M2M_PRMRR_BASE_N1_M2MEM_MAIN_REG;
    PrmrrBaseN1Struct.Data = ReadCpuCsrPciLocal(&reg);
    reg.offset = M2M_PRMRR_MASK_N1_M2MEM_MAIN_REG;
    PrmrrMaskN1Struct.Data = ReadCpuCsrPciLocal(&reg);

    PrmrrBaseAddress = ((UINT64)PrmrrBaseN0Struct.Bits.base_address << 12) | ((UINT64)PrmrrBaseN1Struct.Bits.base_address << 32);
    PrmrrMask = ((UINT64)PrmrrMaskN0Struct.Bits.range_mask << 12) | ((UINT64)PrmrrMaskN1Struct.Bits.range_mask << 32);

    if ((PhyAddr & PrmrrMask) == (PrmrrBaseAddress & PrmrrMask)) {
      IsSgxMem = TRUE;
    }
  }
  return IsSgxMem;
}
#endif /* CHECK_SGX_MEMORY */
