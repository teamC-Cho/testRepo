/** @file
  Interface of Address decode library.

  @copyright
  INTEL CONFIDENTIAL
  Copyright 2016 - 2020 Intel Corporation. <BR>

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

#ifndef __ADRESS_DECODE_LIB_H__
#define __ADRESS_DECODE_LIB_H__

//#include <Library/MemTypeLib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "bios.h"

#define F_OK	0

//#define   ADDRESS_TRANSLATION_FORWARD /* DIMM-to-SystemAddress */
#define   SOCKET_CHANNEL_CHECK

/* Reference: 574451 3rd Gen Intel Xeon EDS Vol One R2p0.pdf
 * Page-230 $10.6.2 PCI Configuration Space Registers
 * Table 10-24.WrPCIConfigLocal() Device/Function Protection Table
 */


/* Reference: 574451 3rd Gen Intel Xeon EDS Vol One R2p0.pdf
 * Page-232 $10.6.4 BAR IDs for Memory Mapped IO Register
 * Table 10-26.Rd/WrEndPointConfig() MMIO BAR IDs
 */
#define MMIO_BAR_ID_IMC    0
#define MMIO_BAR_ID_PCU    0
#define MMIO_BAR_ID_CHA    0
#define MMIO_BAR_ID_VTD    1
#define MMIO_BAR_ID_DMI    0
#define MMIO_BAR_ID_VMD    2
#define MMIO_BAR_ID_CBDMA  0
#define MMIO_BAR_ID_PCIE   0
#define MMIO_BAR_ID_NTB    0

#define BUS_NO_VTD_0       0x00
#define BUS_NO_VTD_1       0x16
#define BUS_NO_VTD_2       0x30
#define BUS_NO_VTD_3       0x4A
#define BUS_NO_VTD_4       0x64
#define BUS_NO_VTD_5       0x80
#define BUS_NO_VTD_6       0x97
#define BUS_NO_VTD_7       0xB0
#define BUS_NO_VTD_8       0xC9
#define BUS_NO_VTD_9       0xE2

/* PCI bus mapping: 574451 3rd Gen Intel Xeon EDS Vol One R2p0.pdf. $10.1.13
 *
 * For WrEndPointConfig()
 * Local PCI Cfg, the device bus number is as denoted in the processor
 * External Design Specification (EDS), VolumeTwo: Registers.
 * Exception: Bus 30 and 31, which have been translated to Bus 13 and 14 respectively.
 */
#define BUS_NUMBER_PCI30   13
#define BUS_NUMBER_PCI31   14
#define BUS_NUMBER_CHA     BUS_NUMBER_PCI31 /* Read value of  bus 31 and 14 is equal */
#define BUS_NUMBER_VTD     BUS_NO_VTD_0
#define DEVICE_IMC_PCI     12
#define FUNCTION_IMC_MMIO  0 /* 1: wrong */


/* ServerSiliconPkg/Ras/Library10nm/AddressDecodeLib/AddressDecodePrivate.h */
#define   RANK_INTL_MIN_CLOSE     6
#define   RANK_INTL_MIN_OPEN      13

/* ServerSiliconPkg/Include/Library/AddressDecodeLib.h */
#define   FORWARD_ADDRESS_TRANSLATION  1
#define   REVERSE_ADDRESS_TRANSLATION  2

#define   SOCKET_VALID            BIT0
#define   MEMTYPE_VALID           BIT1
#define   DIMMTYPE_VALID          BIT2
#define   MC_VALID                BIT3
#define   CHN_ADDR_VALID          BIT4
#define   CHN_VALID               BIT5
#define   DPA_VALID               BIT6
#define   CHIP_SELECT_VALID       BIT7
#define   PYSICAL_RANK_VALID      BIT8
#define   DIMM_VALID              BIT9
#define   COLUMN_VALID            BIT10
#define   ROW_VALID               BIT11
#define   BANK_VALID              BIT12
#define   BANK_GROUP_VALID        BIT13
#define   SUB_RANK_VALID          BIT14
#define   NM_MC_VALID             BIT15
#define   NM_CHN_VALID            BIT16
#define   NM_CHN_ADDR_VALID       BIT17
#define   NM_CHIP_SELECT_VALID    BIT18
#define   NM_PYSICAL_RANK_VALID   BIT19
#define   NM_DIMM_VALID           BIT20
#define   NM_COLUMN_VALID         BIT21
#define   NM_ROW_VALID            BIT22
#define   NM_BANK_VALID           BIT23
#define   NM_BANK_GROUP_VALID     BIT24
#define   RANK_ADDRESS_VALID      BIT25
#define   NM_RANK_ADDRESS_VALID   BIT26
#define   TARGET_WAY_VALID        BIT27
#define   CHANNEL_WAY_VALID       BIT28

/*-- Miscellaneous Defs --*/
#define IN
#define OUT
#define OPTIONAL

/* ProcessorBind.h */
#define EFIAPI
#define MAX_BIT      0x80000000

/* Base.h + UefiBaseType.h */
#define EFI_ERROR(StatusCode)     (((INT64)(StatusCode)) < 0)

/* IioUniversalData.h */
//#define UINT64  unsigned long long

/* MemDecodeCommonIncludes.h */
#define INTERLEAVE_MODE_256B      2 // Index = [10:8]
#define INTERLEAVE_MODE_256B_XOR  7 // Index = [10:8] ^ [16:14] ^ [24:22]
#define INTERLEAVE_MODE_4KB       6 // Index = [14:12]


/* SiFviLib.h */
typedef struct {
    UINT8   MajorVersion;
    UINT8   MinorVersion;
    UINT8   Revision;
    UINT16  BuildNum;
} RC_VERSION;

/* IioPlatformData.h */
typedef struct {
    UINT8       Device;
    UINT8       Function;
} IIO_PORT_INFO;


/*--ServerSiliconPkg/Include/Library/AddressDecodeLib.h--*/
typedef enum {
  ddr3dimmType = 0,
  ddr4dimmType = 1,
  ddrtdimmType = 2,
  HbmdimmType = 3,
  dimmTypeUnknown
} TRANSLATED_DIMM_TYPE;

/* MemTypeLib.h */
typedef enum {
  MemTypeNone = 0,
  MemType1lmDdr,
  MemType1lmAppDirect,
  MemType1lmAppDirectReserved,
  MemType1lmCtrl,
  MemType1lmHbm,
  MemTypeNxm,
  MemType2lmDdrCacheMemoryMode,
  MemType2lmDdrWbCacheAppDirect,
  MemType2lmHbmCacheDdr,
  MemType2lmHbmCacheMemoryMode,
  MemTypeCxlAccVolatileMem,
  MemTypeCxlAccPersistentMem,
  MemTypeFpga,
  MemTypeCxlExpVolatileMem,
  MemTypeCxlExpPersistentMem,
  MemTypeMax
} MEM_TYPE;





/* IioUniversalData.h */
//--------------------------------------------------------------------------------------//
// Structure definitions for Universal Data Store (UDS)
//--------------------------------------------------------------------------------------//
#pragma pack(1)
typedef struct {
    UINT8                     Valid;         // TRUE, if the link is valid (i.e reached normal operation)
    UINT8                     PeerSocId;     // Socket ID
    UINT8                     PeerSocType;   // Socket Type (0 - CPU; 1 - IIO)
    UINT8                     PeerPort;      // Port of the peer socket
}QPI_PEER_DATA;

typedef struct {
    UINT8                     Valid;
    UINT32                    MmioBar[TYPE_MAX_MMIO_BAR];
    UINT8                     PcieSegment;
    UINT64_STRUCT             SegMmcfgBase;
    UINT16                    stackPresentBitmap;
    UINT16                    M2PciePresentBitmap;
    UINT8                     TotM3Kti;
    UINT8                     TotCha;
    UINT32                    ChaList[MAX_CHA_MAP];
    UINT32                    SocId;
    QPI_PEER_DATA             PeerInfo[MAX_FW_KTI_PORTS];    // QPI LEP info
} QPI_CPU_DATA;

typedef struct {
    UINT8                     Valid;
    UINT8                     SocId;
    QPI_PEER_DATA             PeerInfo[MAX_SOCKET];    // QPI LEP info
} QPI_IIO_DATA;

typedef struct {
    IIO_PORT_INFO           PortInfo[NUMBER_PORTS_PER_SOCKET];
} IIO_DMI_PCIE_INFO;

typedef struct _STACK_RES {
    UINT8                   Personality;
    UINT8                   BusBase;
    UINT8                   BusLimit;
    UINT16                  PciResourceIoBase;
    UINT16                  PciResourceIoLimit;
    UINT32                  IoApicBase;
    UINT32                  IoApicLimit;
    UINT32                  Mmio32Base;            // Base of low MMIO configured for this stack in memory map
    UINT32                  Mmio32Limit;           // Limit of low MMIO configured for this stack in memory map
    UINT64                  Mmio64Base;            // Base of high MMIO configured for this stack in memory map
    UINT64                  Mmio64Limit;           // Limit of high MMIO configured for this stack in memory map
    UINT32                  PciResourceMem32Base;  // Base of low MMIO resource available for PCI devices
    UINT32                  PciResourceMem32Limit; // Limit of low MMIO resource available for PCI devices
    UINT64                  PciResourceMem64Base;  // Base of high MMIO resource available for PCI devices
    UINT64                  PciResourceMem64Limit; // Limit of high MMIO resource available for PCI devices
    UINT32                  VtdBarAddress;
    UINT32                  Mmio32MinSize;         // Minimum required size of MMIO32 resource needed for this stack
} STACK_RES;

typedef struct {
    UINT8                   Valid;
    UINT8                   SocketID;            // Socket ID of the IIO (0..3)
    UINT8                   BusBase;
    UINT8                   BusLimit;
    UINT16                  PciResourceIoBase;
    UINT16                  PciResourceIoLimit;
    UINT32                  IoApicBase;
    UINT32                  IoApicLimit;
    UINT32                  Mmio32Base;          // Base of low MMIO configured for this socket in memory map
    UINT32                  Mmio32Limit;         // Limit of low MMIO configured for this socket in memory map
    UINT64                  Mmio64Base;          // Base of high MMIO configured for this socket in memory map
    UINT64                  Mmio64Limit;         // Limit of high MMIO configured for this socket in memory map
    STACK_RES               StackRes[MAX_LOGIC_IIO_STACK];
    UINT32                  RcBaseAddress;
    IIO_DMI_PCIE_INFO       PcieInfo;
    UINT8                   DmaDeviceCount;
} IIO_RESOURCE_INSTANCE;

typedef struct {
    UINT16                  PlatGlobalIoBase;       // Global IO Base
    UINT16                  PlatGlobalIoLimit;      // Global IO Limit
    UINT32                  PlatGlobalMmio32Base;   // Global Mmiol base
    UINT32                  PlatGlobalMmio32Limit;  // Global Mmiol limit
    UINT64                  PlatGlobalMmio64Base;   // Global Mmioh Base [43:0]
    UINT64                  PlatGlobalMmio64Limit;  // Global Mmioh Limit [43:0]
    QPI_CPU_DATA            CpuQpiInfo[MAX_SOCKET]; // QPI related info per CPU
    QPI_IIO_DATA            IioQpiInfo[MAX_SOCKET]; // QPI related info per IIO
    UINT32                  MemTsegSize;
    UINT32                  MemIedSize;
    UINT64                  PciExpressBase;
    UINT32                  PciExpressSize;
    UINT32                  MemTolm;
    IIO_RESOURCE_INSTANCE   IIO_resource[MAX_SOCKET];
    UINT8                   numofIIO;
    UINT8                   MaxBusNumber;
    UINT32                  packageBspApicID[MAX_SOCKET]; // This data array is valid only for SBSP, not for non-SBSP CPUs. <AS> for CpuSv
    UINT8                   EVMode;
    UINT8                   Pci64BitResourceAllocation;
    UINT8                   SkuPersonality[MAX_SOCKET];
    UINT8                   VMDStackEnable[MaxIIO][MAX_IIO_STACK];
    UINT16                  IoGranularity;
    UINT32                  MmiolGranularity;
    UINT64_STRUCT           MmiohGranularity;
    UINT8                   RemoteRequestThreshold;  //5370389
    UINT32                  UboxMmioSize;
    UINT32                  MaxAddressBits;
} PLATFORM_DATA;

typedef struct {
    UINT8                   CurrentUpiiLinkSpeed;// Current programmed UPI Link speed (Slow/Full speed mode)
    UINT8                   CurrentUpiLinkFrequency; // Current requested UPI Link frequency (in GT)
    UINT8                   OutKtiCpuSktHotPlugEn;            // 0 - Disabled, 1 - Enabled for PM X2APIC
    UINT32                  OutKtiPerLinkL1En[MAX_SOCKET];    // output kti link enabled status for PM
    UINT8                   IsocEnable;
    UINT32                  meRequestedSize; // Size of the memory range requested by ME FW, in MB
    UINT32                  ieRequestedSize; // Size of the memory range requested by IE FW, in MB
    UINT8                   DmiVc1;
    UINT8                   DmiVcm;
    UINT32                  CpuPCPSInfo;
    UINT8                   cpuSubType;
    UINT8                   SystemRasType;
    UINT8                   numCpus;                                        // 1,..4. Total number of CPU packages installed and detected (1..4)by QPI RC
    UINT16                  tolmLimit;
    UINT32                  tohmLimit;
    RC_VERSION              RcVersion;
    BOOLEAN                 MsrTraceEnable;
    UINT8                   DdrXoverMode;           // DDR 2.2 Mode
    // For RAS
    UINT8                   bootMode;
    UINT8                   OutClusterOnDieEn; // Whether RC enabled COD support
    UINT8                   OutSncEn;
    UINT8                   OutNumOfCluster;
    UINT8                   imcEnabled[MAX_SOCKET][MAX_IMC];
    UINT16                  LlcSizeReg;
    UINT8                   chEnabled[MAX_SOCKET][MAX_CH];
    UINT8                   memNode[MC_MAX_NODE];
    UINT8                   IoDcMode;
    UINT8                   DfxRstCplBitsEn;
} SYSTEM_STATUS;

typedef struct {
    PLATFORM_DATA           PlatformData;
    SYSTEM_STATUS           SystemStatus;
    UINT32                  OemValue;
} IIO_UDS;
#pragma pack()

#define IIO_UNIVERSAL_DATA_GUID { 0x7FF396A1, 0xEE7D, 0x431E, { 0xBA, 0x53, 0x8F, 0xCA, 0x12, 0x7C, 0x44, 0xC0 } }

/*** BIOS_LB69_BETA/ServerSiliconPkg/Mem/Library/MemMcIpLib/Ras/MemRasPatrolScrubIcx.c ***/
/* PiHob.h */
///
/// Describes the format and size of the data inside the HOB.
/// All HOBs must contain this generic HOB header.
///
/*
typedef struct {
  ///
  /// Identifies the HOB data structure type.
  ///
  UINT16    HobType;
  ///
  /// The length in bytes of the HOB.
  ///
  UINT16    HobLength;
  ///
  /// This field must always be set to zero.
  ///
  UINT32    Reserved;
} EFI_HOB_GENERIC_HEADER;

///
/// Allows writers of executable content in the HOB producer phase to
/// maintain and manage HOBs with specific GUID.
///
typedef struct _EFI_HOB_GUID_TYPE {
  EFI_HOB_GENERIC_HEADER  Header;
  EFI_GUID                Name;
  //
  // Guid specific data goes here
  //
} EFI_HOB_GUID_TYPE;
*/


/* MemHostChipCommon.h */
#define SAD_UNIT            26                  // SAD limit unit is 64MB aka 2^26
#define MAX_DIMM            2                   // Max DIMM per channel
#define MAX_RIR             4                   // Number of Rank Interleave Register rules for DDR
#define MAX_RIR_DDRT        4                   // Number of Rank Interleave Register rules for NVMDIMM
#define MAX_RIR_WAYS        8                   // Number of interleave ways for RIR for DDR
#define MAX_RIR_DDRT_WAYS   2                   // Number of interleave ways for RIR for NVMDIMM
#define SAD_RULES_10NM     16                   // Number of SAD rule registers in 10NM
#define REMOTE_SAD_RULES_10NM_WAVE1      14     // Number of remote SAD rule registers in 10nm wave1
#define RIR_UNIT            29                  // RIR limit unit is 512MB aka 2^29
#define TAD_RULES           8                   // Number of near memory TAD rule registers
#define FM_TAD_RULES_10NM   4                   // Number of far memory only TAD rule registers in 10nm
#define MAX_TAD_RULES_10NM  (TAD_RULES + FM_TAD_RULES_10NM) // Number of combined near and far TAD rules in 10nm
#define CONVERT_64MB_TO_32GB_GRAN   9

/* CpRcPkg/Library/BaseMemoryCoreLib/Core/Include/MemHostChipCommon.h:161: */
#define MAX_RT              2                   // Number of RTs per route table type

// Enum used to index into array elements of [MaxRtHalves] dimensions
typedef enum {
  RtHalf0,
  RtHalf1,
  MaxRtHalves = MAX_RT
} ROUTE_TABLE_HALVES;

/* SiliconSetting.h */
#define TAD_RULES           8

/* Base.h */
//#define NULL  ((VOID *) 0)
#define SIZE_1MB    0x00100000
#define ENCODE_ERROR(StatusCode)     ((unsigned long long)(MAX_BIT | (StatusCode)))
#define ENCODE_WARNING(StatusCode)   ((unsigned long long)(StatusCode))

/* MemDecodeCommonIncludes.h */
//
// 10nm specific defines
//
#define RT0   0
#define RT1   1
#define RT2   2
#define RT2LM 3

/* UefiBaseType.h */
//typedef UINTN RETURN_STATUS;
//typedef RETURN_STATUS             EFI_STATUS;
//typedef UINTN EFI_STATUS;
typedef UINT64 EFI_STATUS;

///
/// Enumeration of EFI_STATUS.
///@{
#define EFI_SUCCESS               0    
#define EFI_LOAD_ERROR            ENCODE_ERROR (1)
#define EFI_INVALID_PARAMETER     ENCODE_ERROR (2)
#define EFI_UNSUPPORTED           ENCODE_ERROR (3)
#define EFI_BAD_BUFFER_SIZE       ENCODE_ERROR (4)
#define EFI_BUFFER_TOO_SMALL      ENCODE_ERROR (5)
#define EFI_NOT_READY             ENCODE_ERROR (6)
#define EFI_DEVICE_ERROR          ENCODE_ERROR (7)
#define EFI_WRITE_PROTECTED       ENCODE_ERROR (8)
#define EFI_OUT_OF_RESOURCES      ENCODE_ERROR (9)
#define EFI_VOLUME_CORRUPTED      ENCODE_ERROR (10)
#define EFI_VOLUME_FULL           ENCODE_ERROR (11)
#define EFI_NO_MEDIA              ENCODE_ERROR (12)
#define EFI_MEDIA_CHANGED         ENCODE_ERROR (13)
#define EFI_NOT_FOUND             ENCODE_ERROR (14)
#define EFI_ACCESS_DENIED         ENCODE_ERROR (15)
#define EFI_NO_RESPONSE           ENCODE_ERROR (16)
#define EFI_NO_MAPPING            ENCODE_ERROR (17)
#define EFI_TIMEOUT               ENCODE_ERROR (18)
#define EFI_NOT_STARTED           ENCODE_ERROR (19)
#define EFI_ALREADY_STARTED       ENCODE_ERROR (20)
#define EFI_ABORTED               ENCODE_ERROR (21)
#define EFI_ICMP_ERROR            ENCODE_ERROR (22)
#define EFI_TFTP_ERROR            ENCODE_ERROR (23)
#define EFI_PROTOCOL_ERROR        ENCODE_ERROR (24)
#define EFI_INCOMPATIBLE_VERSION  ENCODE_ERROR (25)
#define EFI_SECURITY_VIOLATION    ENCODE_ERROR (26)
#define EFI_CRC_ERROR             ENCODE_ERROR (27)
#define EFI_END_OF_MEDIA          ENCODE_ERROR (28)
/* 29, 30: absent */
#define EFI_END_OF_FILE           ENCODE_ERROR (31)
#define EFI_INVALID_LANGUAGE      ENCODE_ERROR (32)
#define EFI_COMPROMISED_DATA      ENCODE_ERROR (33)
#define EFI_HTTP_ERROR            

#define EFI_WARN_UNKNOWN_GLYPH    ENCODE_WARNING (1)
#define EFI_WARN_DELETE_FAILURE   ENCODE_WARNING (2)
#define EFI_WARN_WRITE_FAILURE    ENCODE_WARNING (3)
#define EFI_WARN_BUFFER_TOO_SMALL ENCODE_WARNING (4)
#define EFI_WARN_STALE_DATA       ENCODE_WARNING (5)
#define EFI_WARN_FILE_SYSTEM      ENCODE_WARNING (6)

///@}

/* MMIO type registers have channel 0 and 1.
 * The address difference between them is 0x4000.
 */
#define MMIO_CHANNEL_1_INCREASE         0x4000

/******************************************************************************
 * Following definitions on offset are based on register reference manual:
 *   ICX EDSVol2 574942 R1-5.pdf
 *
 * "Icx" is chosen instead of "Cpx" or "Skx".
 * $ grep -RHn "define DIMMMTR_[0-9]_MC_MAIN_REG" --include=*.h
 * Use a member variable of each register's struct to search
 * the corresponding register from register manual.
 *
 * Comment format of each definition:
 *   BIOS header file name with the register
 *   Pages and section number at the reference manual
 ******************************************************************************/
/* ServerSiliconPkg/Include/Registers/Icx/M2MEM_MAIN.h:6986
 * "page-2718. $13.1.43"	PCI	30 12 0 */
#define TAD_RD_N0_M2MEM_MAIN_REG	0x238
#define TAD_RD_N1_M2MEM_MAIN_REG	0x23c
/* ServerSiliconPkg/Mem/Library/MemMcIpLib/Include/Registers/Icx/MC_MAIN.h:8321
 * "page-2950. $13.5.16"	MMIO	30 0 1 */
#define DRAM_RULE_CFG0_N0_MC_MAIN_REG 	0x20CE8
#define DRAM_RULE_CFG1_N0_MC_MAIN_REG 	0x20CF0
#define DRAM_RULE_CFG2_N0_MC_MAIN_REG 	0x20CF8
#define DRAM_RULE_CFG3_N0_MC_MAIN_REG 	0x20D00
#define DRAM_RULE_CFG4_N0_MC_MAIN_REG 	0x20D08
#define DRAM_RULE_CFG5_N0_MC_MAIN_REG 	0x20D10
#define DRAM_RULE_CFG6_N0_MC_MAIN_REG 	0x20D18
#define DRAM_RULE_CFG7_N0_MC_MAIN_REG 	0x20D20
#define DRAM_RULE_CFG0_N1_MC_MAIN_REG 	0x20CEC
#define DRAM_RULE_CFG1_N1_MC_MAIN_REG 	0x20CF4
#define DRAM_RULE_CFG2_N1_MC_MAIN_REG 	0x20CFC
#define DRAM_RULE_CFG3_N1_MC_MAIN_REG 	0x20D08
#define DRAM_RULE_CFG4_N1_MC_MAIN_REG 	0x20D0C
#define DRAM_RULE_CFG5_N1_MC_MAIN_REG 	0x20D18
#define DRAM_RULE_CFG6_N1_MC_MAIN_REG 	0x20D1C
#define DRAM_RULE_CFG7_N1_MC_MAIN_REG 	0x20D28
/* ServerSiliconPkg/Include/Registers/Icx/CHABC_SAD1.h:5615
 * "page-3501. $15.4.72"	PCI	31 29 1 */
#define BLOCK_DECODER_ADDR_N0_CHABC_SAD1_REG	0x298
#define BLOCK_DECODER_ADDR_N1_CHABC_SAD1_REG	0x29c
/* ServerSiliconPkg/Include/Registers/Icx/CHA_MISC.h
 * "page-3413. $15.2.36"	PCI	31 10 0 */
#define H0_TGT_ROUTE_TABLE_0_CHA_MISC_REG	0x184
#define H1_TGT_ROUTE_TABLE_0_CHA_MISC_REG	0x19C
#define H0_TGT_ROUTE_TABLE_1_CHA_MISC_REG	0x1b4
#define H1_TGT_ROUTE_TABLE_1_CHA_MISC_REG	0x1cc
#define H0_TGT_ROUTE_TABLE_2_CHA_MISC_REG	0x1e0
#define H1_TGT_ROUTE_TABLE_2_CHA_MISC_REG	0x1f8
#define H0_CH_ROUTE_TABLE_0_CHA_MISC_REG	0x180
#define H1_CH_ROUTE_TABLE_0_CHA_MISC_REG	0x198
#define H0_CH_ROUTE_TABLE_1_CHA_MISC_REG	0x1b0
#define H1_CH_ROUTE_TABLE_1_CHA_MISC_REG	0x1c8
#define H0_CH_ROUTE_TABLE_2_CHA_MISC_REG	0x1e4
#define H1_CH_ROUTE_TABLE_2_CHA_MISC_REG	0x1fc
/* ServerSiliconPkg/Include/Registers/Icx/CHA_MISC.h:3146
 * "page-3416. $15.2.47"	PCI	31 10 0 */
#define H0_TGT_ROUTE_TABLE_2LM_CHA_MISC_REG	0x210
#define H1_TGT_ROUTE_TABLE_2LM_CHA_MISC_REG	0x228
/* ServerSiliconPkg/Include/Registers/Icx/CHABC_SAD.h:7885
 * "page-3454. $15.3.82"	PCI	31 29 0 */
#define DRAM_GLOBAL_INTERLEAVE_CHABC_SAD_REG	0x210
/* ServerSiliconPkg/Include/Registers/Icx/CHABC_SAD.h:8100
 * "page-3456. $15.3.84"	PCI	31 29 0 */
#define DRAM_H0_RT0_MODE0_CHABC_SAD_REG	0x220
#define DRAM_H0_RT0_MODE1_CHABC_SAD_REG	0x224
#define DRAM_H1_RT0_MODE0_CHABC_SAD_REG	0x228
#define DRAM_H1_RT0_MODE1_CHABC_SAD_REG	0x22c
#define DRAM_H0_RT1_MODE0_CHABC_SAD_REG	0x230
#define DRAM_H0_RT1_MODE1_CHABC_SAD_REG	0x234
#define DRAM_H1_RT1_MODE0_CHABC_SAD_REG	0x238
#define DRAM_H1_RT1_MODE1_CHABC_SAD_REG	0x23c
#define DRAM_H0_RT2_MODE0_CHABC_SAD_REG	0x240
#define DRAM_H1_RT2_MODE0_CHABC_SAD_REG	0x248
/* ServerSiliconPkg/Include/Registers/Icx/CHABC_SAD.h:7187
 * "page-3449. $15.3.80"	PCI	31 29 0 */
#define DRAM_MC_TARGET_CHABC_SAD_REG	0x200
/* ServerSiliconPkg/Include/Registers/Icx/CHABC_SAD.h:7586
 * "page-3451. $15.3.81"	PCI	31 29 0 */
#define DRAM_MC_CHANNEL_CHABC_SAD_REG	0x208
/* ServerSiliconPkg/Include/Registers/Icx/CHABC_SAD.h:2323
 * "page-3437. $15.3.35"	PCI	31 29 0 */
#define INTERLEAVE_LIST_CFG_0_CHABC_SAD_REG		0x10C
#define INTERLEAVE_LIST_CFG_1_CHABC_SAD_REG		0x114
#define INTERLEAVE_LIST_CFG_2_CHABC_SAD_REG		0x11C
#define INTERLEAVE_LIST_CFG_3_CHABC_SAD_REG		0x124
#define INTERLEAVE_LIST_CFG_4_CHABC_SAD_REG		0x12C
#define INTERLEAVE_LIST_CFG_5_CHABC_SAD_REG		0x134
#define INTERLEAVE_LIST_CFG_6_CHABC_SAD_REG		0x13C
#define INTERLEAVE_LIST_CFG_7_CHABC_SAD_REG		0x144
#define INTERLEAVE_LIST_CFG_8_CHABC_SAD_REG		0x14C
#define INTERLEAVE_LIST_CFG_9_CHABC_SAD_REG		0x154
#define INTERLEAVE_LIST_CFG_10_CHABC_SAD_REG	0x15C
#define INTERLEAVE_LIST_CFG_11_CHABC_SAD_REG	0x164
#define INTERLEAVE_LIST_CFG_12_CHABC_SAD_REG	0x16C
#define INTERLEAVE_LIST_CFG_13_CHABC_SAD_REG	0x174
#define INTERLEAVE_LIST_CFG_14_CHABC_SAD_REG	0x17C
#define INTERLEAVE_LIST_CFG_15_CHABC_SAD_REG	0x184
/* ServerSiliconPkg/Include/Registers/Icx/CHABC_SAD.h:5731
 * "page-3446. $15.3.66"	PCI	31 29 0 */
#define REMOTE_DRAM_RULE_CFG_0_N0_CHABC_SAD_REG		0x190
#define REMOTE_DRAM_RULE_CFG_1_N0_CHABC_SAD_REG		0x198
#define REMOTE_DRAM_RULE_CFG_2_N0_CHABC_SAD_REG		0x1A0
#define REMOTE_DRAM_RULE_CFG_3_N0_CHABC_SAD_REG		0x1A8
#define REMOTE_DRAM_RULE_CFG_4_N0_CHABC_SAD_REG		0x1B0
#define REMOTE_DRAM_RULE_CFG_5_N0_CHABC_SAD_REG		0x1B8
#define REMOTE_DRAM_RULE_CFG_6_N0_CHABC_SAD_REG		0x1C0
#define REMOTE_DRAM_RULE_CFG_7_N0_CHABC_SAD_REG		0x1C8
#define REMOTE_DRAM_RULE_CFG_8_N0_CHABC_SAD_REG		0x1D0
#define REMOTE_DRAM_RULE_CFG_9_N0_CHABC_SAD_REG		0x1D8
#define REMOTE_DRAM_RULE_CFG_10_N0_CHABC_SAD_REG	0x1E0
#define REMOTE_DRAM_RULE_CFG_11_N0_CHABC_SAD_REG	0x1E8
#define REMOTE_DRAM_RULE_CFG_12_N0_CHABC_SAD_REG	0x1F0
#define REMOTE_DRAM_RULE_CFG_13_N0_CHABC_SAD_REG	0x1F8
#define REMOTE_DRAM_RULE_CFG_0_N1_CHABC_SAD_REG 	0x194
#define REMOTE_DRAM_RULE_CFG_1_N1_CHABC_SAD_REG		0x19C
#define REMOTE_DRAM_RULE_CFG_2_N1_CHABC_SAD_REG		0x1A4
#define REMOTE_DRAM_RULE_CFG_3_N1_CHABC_SAD_REG		0x1AC
#define REMOTE_DRAM_RULE_CFG_4_N1_CHABC_SAD_REG		0x1B4
#define REMOTE_DRAM_RULE_CFG_5_N1_CHABC_SAD_REG		0x1BC
#define REMOTE_DRAM_RULE_CFG_6_N1_CHABC_SAD_REG		0x1C4
#define REMOTE_DRAM_RULE_CFG_7_N1_CHABC_SAD_REG		0x1CC
#define REMOTE_DRAM_RULE_CFG_8_N1_CHABC_SAD_REG		0x1D4
#define REMOTE_DRAM_RULE_CFG_9_N1_CHABC_SAD_REG		0x1DC
#define REMOTE_DRAM_RULE_CFG_10_N1_CHABC_SAD_REG	0x1E4
#define REMOTE_DRAM_RULE_CFG_11_N1_CHABC_SAD_REG	0x1EC
#define REMOTE_DRAM_RULE_CFG_12_N1_CHABC_SAD_REG	0x1F4
#define REMOTE_DRAM_RULE_CFG_13_N1_CHABC_SAD_REG	0x1FC
/* ServerSiliconPkg/Include/Registers/Icx/CHA_PMA.h:1881
 * "page-3388. $15.1.33"	PCI	31 0 0 */
#define CHA_SNC_CONFIG_CHA_PMA_REG	0x220
/* ServerSiliconPkg/Include/Registers/Icx/IIO_VTD.h:8671
 * "page-773. $6.1.108"	PCI	0 0 0 */
#define UMA_CLUSTER_CONFIG_IIO_VTD_REG	0x44c
/* ServerSiliconPkg/Include/Registers/Icx/IIO_VTD.h:8544
 * "page-772. $6.1.106"	PCI	0 0 0 */
#define SNC_UPPER_BASE_IIO_VTD_REG	0x444
/* ServerSiliconPkg/Include/Registers/Icx/IIO_VTD.h:8325
 * "page-769. $6.1.101"	PCI	0 0 0 */
#define SNC_BASE_1_IIO_VTD_REG	0x430
#define SNC_BASE_2_IIO_VTD_REG	0x434
#define SNC_BASE_3_IIO_VTD_REG	0x438
#define SNC_BASE_4_IIO_VTD_REG	0x43C
#define SNC_BASE_5_IIO_VTD_REG	0x440
/* ServerSiliconPkg/Include/Registers/Icx/CHABC_SAD1.h:5703
 * "page-3502. $15.4.73"	PCI	31 29 1 */
#define BLOCK_DECODER_EN_CFG_0_CHABC_SAD1_REG	0x2A0
#define BLOCK_DECODER_EN_CFG_1_CHABC_SAD1_REG	0x2A2
#define BLOCK_DECODER_EN_CFG_2_CHABC_SAD1_REG	0x2A4
#define BLOCK_DECODER_EN_CFG_3_CHABC_SAD1_REG	0x2A6
#define BLOCK_DECODER_EN_CFG_4_CHABC_SAD1_REG	0x2A8
#define BLOCK_DECODER_EN_CFG_5_CHABC_SAD1_REG	0x2AA
#define BLOCK_DECODER_EN_CFG_6_CHABC_SAD1_REG	0x2AC
#define BLOCK_DECODER_EN_CFG_7_CHABC_SAD1_REG	0x2AE
/* ServerSiliconPkg/Include/Registers/Icx/CHABC_SAD.h:2244
 * "page-3436. $15.3.34"	PCI	31 29 0 */
#define DRAM_RULE_CFG_0_CHABC_SAD_REG	0x108
#define DRAM_RULE_CFG_1_CHABC_SAD_REG	0x110
#define DRAM_RULE_CFG_2_CHABC_SAD_REG	0x118
#define DRAM_RULE_CFG_3_CHABC_SAD_REG	0x120
#define DRAM_RULE_CFG_4_CHABC_SAD_REG	0x128
#define DRAM_RULE_CFG_5_CHABC_SAD_REG	0x130
#define DRAM_RULE_CFG_6_CHABC_SAD_REG	0x138
#define DRAM_RULE_CFG_7_CHABC_SAD_REG	0x140
#define DRAM_RULE_CFG_8_CHABC_SAD_REG	0x148
#define DRAM_RULE_CFG_9_CHABC_SAD_REG	0x150
#define DRAM_RULE_CFG_10_CHABC_SAD_REG	0x158
#define DRAM_RULE_CFG_11_CHABC_SAD_REG	0x160
#define DRAM_RULE_CFG_12_CHABC_SAD_REG	0x168
#define DRAM_RULE_CFG_13_CHABC_SAD_REG	0x170
#define DRAM_RULE_CFG_14_CHABC_SAD_REG	0x178
#define DRAM_RULE_CFG_15_CHABC_SAD_REG	0x180
/* ServerSiliconPkg/Mem/Library/MemMcIpLib/Include/Registers/Icx/MC_2LM.h:265
 * "page-2863. $13.3.8"	MMIO	30 0 1 */
#define MCNMCACHINGINTLV_MC_2LM_REG	0x20010
#define MCNMCACHINGOFFSET_MC_2LM_REG	0x20018
#define MCNMCACHINGCFG_MC_2LM_REG	0x2000C
/* ServerSiliconPkg/Include/Registers/Icx/M2MEM_MAIN.h:10873
 * "page-2747. $13.1.71"	PCI	30 12 0 */
#define MIRRORFAILOVER_M2MEM_MAIN_REG	0x300
#define MIRRORCHNLS_M2MEM_MAIN_REG	0x2FC
/* ServerSiliconPkg/Mem/Library/MemMcIpLib/Include/Registers/Icx/MC_MAIN.h:688
 * "page-2898. $13.4.7"	MMIO	30 0 1 */
#define TADCHNILVOFFSET_0_N0_MC_MAIN_REG	0x20820
#define TADCHNILVOFFSET_1_N0_MC_MAIN_REG	0x20828
#define TADCHNILVOFFSET_2_N0_MC_MAIN_REG	0x20830
#define TADCHNILVOFFSET_3_N0_MC_MAIN_REG	0x20838
#define TADCHNILVOFFSET_4_N0_MC_MAIN_REG	0x20840
#define TADCHNILVOFFSET_5_N0_MC_MAIN_REG	0x20848
#define TADCHNILVOFFSET_6_N0_MC_MAIN_REG	0x20850
#define TADCHNILVOFFSET_7_N0_MC_MAIN_REG	0x20858
#define TADCHNILVOFFSET_8_N0_MC_MAIN_REG	0x20860
#define TADCHNILVOFFSET_9_N0_MC_MAIN_REG	0x20868
#define TADCHNILVOFFSET_10_N0_MC_MAIN_REG	0x20870
#define TADCHNILVOFFSET_11_N0_MC_MAIN_REG	0x20878
#define TADCHNILVOFFSET_0_N1_MC_MAIN_REG	0x20824
#define TADCHNILVOFFSET_1_N1_MC_MAIN_REG	0x2082C
#define TADCHNILVOFFSET_2_N1_MC_MAIN_REG	0x20834
#define TADCHNILVOFFSET_3_N1_MC_MAIN_REG	0x2083C
#define TADCHNILVOFFSET_4_N1_MC_MAIN_REG	0x20844
#define TADCHNILVOFFSET_5_N1_MC_MAIN_REG	0x2084C
#define TADCHNILVOFFSET_6_N1_MC_MAIN_REG	0x20854
#define TADCHNILVOFFSET_7_N1_MC_MAIN_REG	0x2085C
#define TADCHNILVOFFSET_8_N1_MC_MAIN_REG	0x20864
#define TADCHNILVOFFSET_9_N1_MC_MAIN_REG	0x2086C
#define TADCHNILVOFFSET_10_N1_MC_MAIN_REG	0x20874
#define TADCHNILVOFFSET_11_N1_MC_MAIN_REG	0x2087C
/* ServerSiliconPkg/Mem/Library/MemMcIpLib/Include/Registers/Icx/MC_MAIN.h:2399
 * "page-2912. $13.4.19"	MMIO	30 0 1 */
#define RIRWAYNESSLIMIT_0_MC_MAIN_REG	0x20884
#define RIRWAYNESSLIMIT_1_MC_MAIN_REG	0x20888
#define RIRWAYNESSLIMIT_2_MC_MAIN_REG	0x2088C
#define RIRWAYNESSLIMIT_3_MC_MAIN_REG	0x20890
/* ServerSiliconPkg/Mem/Library/MemMcIpLib/Include/Registers/Icx/MC_2LM.h:6782
 * "page-2802. $13.2.17"	MMIO	30 0 1 */
#define FMRIRILV0OFFSET_0_MC_2LM_REG	0x2045C
#define FMRIRILV1OFFSET_0_MC_2LM_REG	0x20460
#define FMRIRILV0OFFSET_1_MC_2LM_REG	0x20464
#define FMRIRILV1OFFSET_1_MC_2LM_REG	0x20468
#define FMRIRILV0OFFSET_2_MC_2LM_REG	0x2046C
#define FMRIRILV1OFFSET_2_MC_2LM_REG	0x20470
#define FMRIRILV0OFFSET_3_MC_2LM_REG	0x20474
#define FMRIRILV1OFFSET_3_MC_2LM_REG	0x20478
/* ServerSiliconPkg/Mem/Library/MemMcIpLib/Include/Registers/Icx/MC_MAIN.h:108
 * "page-2892. $13.4.2"	MMIO	30 0 1 */
#define DIMMMTR_0_MC_MAIN_REG	0x2080C
#define DIMMMTR_1_MC_MAIN_REG	0x20810
/* ServerSiliconPkg/Mem/Library/MemMcIpLib/Include/Registers/Icx/MC_MAIN.h:410
 * "page-2895. $13.4.4"	MMIO	30 0 1 */
#define AMAP_MC_MAIN_REG		0x20814
/* ServerSiliconPkg/Mem/Library/MemMcIpLib/Include/Registers/Icx/MC_MAIN.h:5680
 * "page-2925. $13.4.69"	MMIO	30 0 1 */
#define MCDDRTCFG_MC_MAIN_REG	0x20970
/* ServerSiliconPkg/Include/Registers/Icx/M2MEM_MAIN.h:13670
 * "page-2779. $13.1.96"	PCI	30 12 0 */
#define M2M_PRMRR_BASE_N0_M2MEM_MAIN_REG	0x410
#define M2M_PRMRR_BASE_N1_M2MEM_MAIN_REG	0x414
/* ServerSiliconPkg/Include/Registers/Icx/M2MEM_MAIN.h:13776
 * "page-2780. $13.1.97"	PCI	30 12 0 */
#define M2M_PRMRR_MASK_N0_M2MEM_MAIN_REG	0x418
#define M2M_PRMRR_MASK_N1_M2MEM_MAIN_REG	0x41C
/* ServerSiliconPkg/Mem/Library/MemMcIpLib/Include/Registers/Icx/MC_MAIN.h:2659
 * "page-2913. $13.4.23"	MMIO	30 0 1 */
#define RIRILV0OFFSET_0_MC_MAIN_REG	0x20894
#define RIRILV0OFFSET_1_MC_MAIN_REG	0x20898
#define RIRILV0OFFSET_2_MC_MAIN_REG	0x2089C
#define RIRILV0OFFSET_3_MC_MAIN_REG	0x208A0
#define RIRILV1OFFSET_0_MC_MAIN_REG	0x208A4
#define RIRILV1OFFSET_1_MC_MAIN_REG	0x208A8
#define RIRILV1OFFSET_2_MC_MAIN_REG	0x208AC
#define RIRILV1OFFSET_3_MC_MAIN_REG	0x208B0
#define RIRILV2OFFSET_0_MC_MAIN_REG	0x208B4
#define RIRILV2OFFSET_1_MC_MAIN_REG	0x208B8
#define RIRILV2OFFSET_2_MC_MAIN_REG	0x208BC
#define RIRILV2OFFSET_3_MC_MAIN_REG	0x208C0
#define RIRILV3OFFSET_0_MC_MAIN_REG	0x208C4
#define RIRILV3OFFSET_1_MC_MAIN_REG	0x208C8
#define RIRILV3OFFSET_2_MC_MAIN_REG	0x208CC
#define RIRILV3OFFSET_3_MC_MAIN_REG	0x208D0
#define RIRILV4OFFSET_0_MC_MAIN_REG	0x208D4
#define RIRILV4OFFSET_1_MC_MAIN_REG	0x208D8
#define RIRILV4OFFSET_2_MC_MAIN_REG	0x208DC
#define RIRILV4OFFSET_3_MC_MAIN_REG	0x208E0
#define RIRILV5OFFSET_0_MC_MAIN_REG	0x208E4
#define RIRILV5OFFSET_1_MC_MAIN_REG	0x208E8
#define RIRILV5OFFSET_2_MC_MAIN_REG	0x208EC
#define RIRILV5OFFSET_3_MC_MAIN_REG	0x208F0
#define RIRILV6OFFSET_0_MC_MAIN_REG	0x208F4
#define RIRILV6OFFSET_1_MC_MAIN_REG	0x208F8
#define RIRILV6OFFSET_2_MC_MAIN_REG	0x208FC
#define RIRILV6OFFSET_3_MC_MAIN_REG	0x20900
#define RIRILV7OFFSET_0_MC_MAIN_REG	0x20904
#define RIRILV7OFFSET_1_MC_MAIN_REG	0x20908
#define RIRILV7OFFSET_2_MC_MAIN_REG	0x2090C
#define RIRILV7OFFSET_3_MC_MAIN_REG	0x20910
/* ServerSiliconPkg/Mem/Library/MemMcIpLib/Include/Registers/Icx/MC_2LM.h:6526
 * "page-2801. $13.2.13"	MMIO	30 0 1 */
#define FMRIRWAYNESSLIMIT_0_MC_2LM_REG	0x2044C
#define FMRIRWAYNESSLIMIT_1_MC_2LM_REG	0x20450
#define FMRIRWAYNESSLIMIT_2_MC_2LM_REG	0x20454
#define FMRIRWAYNESSLIMIT_3_MC_2LM_REG	0x20458
/* ServerSiliconPkg/Mem/Library/MemMcIpLib/Include/Registers/Icx/MC_MAIN.h:13641
 * "page-2986. $13.5.84"	MMIO	30 0 1 */
#define MCMTR_MC_MAIN_REG	0x20EF8
/******************************************************************************/




/* MC_MAIN.h */
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32 rule_enable : 1;
                            /* Bits[0:0], Access Type=RW, default=0x00000000*/
                            /* Enable for this DRAM rule. */
    UINT32 limit : 26;
                            /* Bits[26:1], Access Type=RW, default=0x00000000*/
                            /*
                               This correspond to Addr[51:26] of the DRAM rule
                               top limit address. Must be strickly greater then
                               previous rule, even if this rule is disabled,
                               unless this rule and all following rules are
                               disabled. Lower limit is the previous rule (or 0
                               if this is the first rule)
                            */
    UINT32 nm_chn_ways : 3;
                            /* Bits[29:27], Access Type=RW, default=0x00000000*/
                            /*
                               NmChnWays - Near Memory Channel Ways: used to
                               calculate ChnLID 000 = 1 way (ChnLID = 0) 001 =
                               2 ways (ChnLID = modulo 2 of System Address) 010
                               = 3 ways (ChnLID = modulo 3 of System Address)
                               011 = 8 ways (ChnLID = module 8 of System
                               Address) 100 = 4 ways (ChnLID = module 4 of
                               System Address)
                            */
    UINT32 nm_chn_gran : 2;
                            /* Bits[31:30], Access Type=RW, default=0x00000000*/
                            /*
                               NmChnGran - Near Memory Channel Granularity:
                               000=64B, 001=256B, 010=4KB,
                               100-111=128B,512B,1KB,2KB; other reserved
                            */
  } Bits;
  UINT32 Data;
} NM_DRAM_RULE_CFG0_N0_MC_MAIN_STRUCT;

typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32 nm_chn_gran : 1;
                            /* Bits[0:0], Access Type=RW, default=0x00000000*/
                            /*
                               NmChnGran - Near Memory Channel Granularity:
                               000=64B, 001=256B, 010=4KB,
                               100-111=128B,512B,1KB,2KB; other reserved
                            */
    UINT32 nm_target_ways : 2;
                            /* Bits[2:1], Access Type=RW, default=0x00000000*/
                            /*
                               NmTargetWays - Near Memory Target Ways: used to
                               calculate ChnLID 00 = 1 way 01 = 2 ways 10 = 4
                               ways 11 = 8 ways
                            */
    UINT32 nm_gran_eq : 1;
                            /* Bits[3:3], Access Type=RW, default=0x00000000*/
                            /*
                               When set to 0, indicate near memory target and
                               channel granularity are different, or target way
                               is 1.
                            */
    UINT32 nm_chn_l2pid : 24;
                            /* Bits[27:4], Access Type=RW, default=0x00FAC688*/
                            /*
                               NmChnL2PID - translation table for Logical
                               (ChnLID) to Physical ID (ChnPID) NmChnL2PID[ 2:
                               0] = mapping from ChnLID=0 to ChnPID NmChnL2PID[
                               5: 3] = mapping from ChnLID=1 to ChnPID
                               NmChnL2PID[ 8: 6] = mapping from ChnLID=2 to
                               ChnPID NmChnL2PID[11: 9] = mapping from ChnLID=3
                               to ChnPID; only used in HBM mode
                               NmChnL2PID[14:12] = mapping from ChnLID=4 to
                               ChnPID; only used in HBM mode NmChnL2PID[17:15]
                               = mapping from ChnLID=5 to ChnPID; only used in
                               HBM mode NmChnL2PID[20:18] = mapping from
                               ChnLID=6 to ChnPID; only used in HBM mode
                               NmChnL2PID[23:21] = mapping from ChnLID=7 to
                               ChnPID; only used in HBM mode Default value maps
                               ChnLID to the same ChnPID
                            */
    UINT32 rsvd : 4;
                            /* Bits[31:28], Access Type=RO, default=None*/

                            /* Reserved */
  } Bits;
  UINT32 Data;
} NM_DRAM_RULE_CFG0_N1_MC_MAIN_STRUCT;

/* CHABC_SAD.h */
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32 rt0_interleave_mode : 3;
                            /* Bits[2:0], Access Type=RW, default=0x00000000*/
                            /*
                               (p)Determines how the route table index for
                               route table 0 is determined.(/p) (p)0 - use the
                               following bit-wise XOR as the index [10:8] ^
                               [16:14] ^ [24:22](/p) (p)1 - defeature mode that
                               disable the XOR. uses [10:8] as the index.(/p)
                               (p)2-7 - reserved(/p)
                            */
    UINT32 rsvd : 1;
                            /* Bits[3:3], Access Type=RO, default=None*/
                            /* Reserved */
    UINT32 rt1_interleave_shift : 3;
                            /* Bits[6:4], Access Type=RW, default=0x00000000*/
                            /*
                               <p>Number of bits to shift the physical address
                               before feeding into the RT1 route table indexing
                               logic.</p> <p>0 use [8:6] when in a power of two
                               mode. Use [51:6] as input to the mod3 for 3-way
                               interleave modes.</p> <p>1 reserved</p> <p>2 use
                               [10:8] when in a power of two mode. Use [51:8]
                               as input to the mod3 for 3-way interleave
                               modes.</p> <p>3 reserved</p> <p>4 reserved</p>
                               <p>5 reserved</p> <p>6 - use [14:12] when in a
                               power of two mode. Use [51:12] as input to the
                               mod3 for 3-way interleave modes.</p> <p>7 - use
                               the following bit-wise XOR as the index [10:8] ^
                               [16:14] ^ [24:22] in power of two mode. 3-way
                               interleave mode not supported. </p>
                            */
    UINT32 rsvd_7 : 1;
                            /* Bits[7:7], Access Type=RO, default=None*/
                            /* Reserved */
    UINT32 rt2_interleave_shift : 4;

                            /* Bits[11:8], Access Type=RW, default=0x00000000*/
                            /*
                               (p)Number of bits to shift the physical address
                               before feeding into the RT2 route table indexing
                               logic.(/p) (p)0 reserved(/p) (p)1 reserved(/p)
                               (p)2 use [9:8](/p) (p)3 reserved(/p) (p)4
                               reserved(/p) (p)5 reserved(/p) (p)6 use
                               [13:12](/p)
                            */
    UINT32 rsvd_12 : 20;
                            /* Bits[31:12], Access Type=RO, default=None*/
                            /* Reserved */
  } Bits;
  UINT32 Data;
} DRAM_GLOBAL_INTERLEAVE_CHABC_SAD_STRUCT;

typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32 mode_0 : 3;
                            /* Bits[2:0], Access Type=RW, default=0x00000000*/
                            /*
                               Specifies the decode mode for the memory
                               targetted by route table 0 and route table 1 on
                               one side of the die for DRAM rule 0
                            */
    UINT32 mode_1 : 3;
                            /* Bits[5:3], Access Type=RW, default=0x00000000*/
                            /*
                               Specifies the decode mode for the memory
                               targetted by route table 0 and route table 1 on
                               one side of the die for DRAM rule 1
                            */
    UINT32 mode_2 : 3;
                            /* Bits[8:6], Access Type=RW, default=0x00000000*/
                            /*
                               Specifies the decode mode for the memory
                               targetted by route table 0 and route table 1 on
                               one side of the die for DRAM rule 2
                            */
    UINT32 mode_3 : 3;
                            /* Bits[11:9], Access Type=RW, default=0x00000000*/
                            /*
                               Specifies the decode mode for the memory
                               targetted by route table 0 and route table 1 on
                               one side of the die for DRAM rule 3
                            */
    UINT32 mode_4 : 3;
                            /* Bits[14:12], Access Type=RW, default=0x00000000*/
                            /*
                               Specifies the decode mode for the memory
                               targetted by route table 0 and route table 1 on
                               one side of the die for DRAM rule 4
                            */
    UINT32 mode_5 : 3;
                            /* Bits[17:15], Access Type=RW, default=0x00000000*/
                            /*
                               Specifies the decode mode for the memory
                               targetted by route table 0 and route table 1 on
                               one side of the die for DRAM rule 5
                            */
    UINT32 mode_6 : 3;
                            /* Bits[20:18], Access Type=RW, default=0x00000000*/
                            /*
                               Specifies the decode mode for the memory
                               targetted by route table 0 and route table 1 on
                               one side of the die for DRAM rule 6
                            */
    UINT32 mode_7 : 3;
                            /* Bits[23:21], Access Type=RW, default=0x00000000*/
                            /*
                               Specifies the decode mode for the memory
                               targetted by route table 0 and route table 1 on
                               one side of the die for DRAM rule 7
                            */
    UINT32 mode_8 : 3;
                            /* Bits[26:24], Access Type=RW, default=0x00000000*/
                            /*
                               Specifies the decode mode for the memory
                               targetted by route table 0 and route table 1 on
                               one side of the die for DRAM rule 8
                            */
    UINT32 mode_9 : 3;
                            /* Bits[29:27], Access Type=RW, default=0x00000000*/
                            /*
                               Specifies the decode mode for the memory
                               targetted by route table 0 and route table 1 on
                               one side of the die for DRAM rule 9
                            */
    UINT32 rsvd : 2;
                            /* Bits[31:30], Access Type=RO, default=None*/
                            /* Reserved */
  } Bits;
  UINT32 Data;
} DRAM_H0_RT0_MODE0_CHABC_SAD_STRUCT;

typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32 mode_0 : 3;
                            /* Bits[2:0], Access Type=RW, default=0x00000000*/
                            /*
                               Specifies the decode mode for the memory
                               targetted by route table 0 and route table 1 on
                               one side of the die for DRAM rule 0
                            */
    UINT32 mode_1 : 3;
                            /* Bits[5:3], Access Type=RW, default=0x00000000*/
                            /*
                               Specifies the decode mode for the memory
                               targetted by route table 0 and route table 1 on
                               one side of the die for DRAM rule 1
                            */
    UINT32 mode_2 : 3;
                            /* Bits[8:6], Access Type=RW, default=0x00000000*/
                            /*
                               Specifies the decode mode for the memory
                               targetted by route table 0 and route table 1 on
                               one side of the die for DRAM rule 2
                            */
    UINT32 mode_3 : 3;
                            /* Bits[11:9], Access Type=RW, default=0x00000000*/
                            /*
                               Specifies the decode mode for the memory
                               targetted by route table 0 and route table 1 on
                               one side of the die for DRAM rule 3
                            */
    UINT32 mode_4 : 3;
                            /* Bits[14:12], Access Type=RW, default=0x00000000*/
                            /*
                               Specifies the decode mode for the memory
                               targetted by route table 0 and route table 1 on
                               one side of the die for DRAM rule 4
                            */
    UINT32 mode_5 : 3;
                            /* Bits[17:15], Access Type=RW, default=0x00000000*/
                            /*
                               Specifies the decode mode for the memory
                               targetted by route table 0 and route table 1 on
                               one side of the die for DRAM rule 5
                            */
    UINT32 mode_6 : 3;
                            /* Bits[20:18], Access Type=RW, default=0x00000000*/
                            /*
                               Specifies the decode mode for the memory
                               targetted by route table 0 and route table 1 on
                               one side of the die for DRAM rule 6
                            */
    UINT32 mode_7 : 3;
                            /* Bits[23:21], Access Type=RW, default=0x00000000*/
                            /*
                               Specifies the decode mode for the memory
                               targetted by route table 0 and route table 1 on
                               one side of the die for DRAM rule 7
                            */
    UINT32 mode_8 : 3;
                            /* Bits[26:24], Access Type=RW, default=0x00000000*/
                            /*
                               Specifies the decode mode for the memory
                               targetted by route table 0 and route table 1 on
                               one side of the die for DRAM rule 8
                            */
    UINT32 mode_9 : 3;
                            /* Bits[29:27], Access Type=RW, default=0x00000000*/
                            /*
                               Specifies the decode mode for the memory
                               targetted by route table 0 and route table 1 on
                               one side of the die for DRAM rule 9
                            */
    UINT32 rsvd : 2;
                            /* Bits[31:30], Access Type=RO, default=None*/
                            /* Reserved */

  } Bits;
  UINT32 Data;
} DRAM_H0_RT1_MODE0_CHABC_SAD_STRUCT;

typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32 fm_mod_0 : 2;
                            /* Bits[1:0], Access Type=RW, default=0x00000000*/
                            /*
                               Specifies the decode mode for the far memory
                               targets on one side of the die for DRAM rule 0
                            */
    UINT32 fm_mod_1 : 2;
                            /* Bits[3:2], Access Type=RW, default=0x00000000*/
                            /*
                               Specifies the decode mode for the far memory
                               targets on one side of the die for DRAM rule 1
                            */
    UINT32 fm_mod_2 : 2;
                            /* Bits[5:4], Access Type=RW, default=0x00000000*/
                            /*
                               Specifies the decode mode for the far memory
                               targets on one side of the die for DRAM rule 2
                            */
    UINT32 fm_mod_3 : 2;
                            /* Bits[7:6], Access Type=RW, default=0x00000000*/
                            /*
                               Specifies the decode mode for the far memory
                               targets on one side of the die for DRAM rule 3
                            */
    UINT32 fm_mod_4 : 2;
                            /* Bits[9:8], Access Type=RW, default=0x00000000*/
                            /*
                               Specifies the decode mode for the far memory
                               targets on one side of the die for DRAM rule 4
                            */
    UINT32 fm_mod_5 : 2;
                            /* Bits[11:10], Access Type=RW, default=0x00000000*/
                            /*
                               Specifies the decode mode for the far memory
                               targets on one side of the die for DRAM rule 5
                            */
    UINT32 fm_mod_6 : 2;
                            /* Bits[13:12], Access Type=RW, default=0x00000000*/
                            /*
                               Specifies the decode mode for the far memory
                               targets on one side of the die for DRAM rule 6
                            */
    UINT32 fm_mod_7 : 2;
                            /* Bits[15:14], Access Type=RW, default=0x00000000*/
                            /*
                               Specifies the decode mode for the far memory
                               targets on one side of the die for DRAM rule 7
                            */
    UINT32 fm_mod_8 : 2;
                            /* Bits[17:16], Access Type=RW, default=0x00000000*/
                            /*
                               Specifies the decode mode for the far memory
                               targets on one side of the die for DRAM rule 8
                            */
    UINT32 fm_mod_9 : 2;
                            /* Bits[19:18], Access Type=RW, default=0x00000000*/
                            /*
                               Specifies the decode mode for the far memory
                               targets on one side of the die for DRAM rule 9
                            */
    UINT32 fm_mod_10 : 2;
                            /* Bits[21:20], Access Type=RW, default=0x00000000*/
                            /*
                               Specifies the decode mode for the far memory
                               targets on one side of the die for DRAM rule 10
                            */
    UINT32 fm_mod_11 : 2;
                            /* Bits[23:22], Access Type=RW, default=0x00000000*/
                            /*
                               Specifies the decode mode for the far memory
                               targets on one side of the die for DRAM rule 11
                            */
    UINT32 fm_mod_12 : 2;
                            /* Bits[25:24], Access Type=RW, default=0x00000000*/
                            /*
                               Specifies the decode mode for the far memory
                               targets on one side of the die for DRAM rule 12
                            */
    UINT32 fm_mod_13 : 2;
                            /* Bits[27:26], Access Type=RW, default=0x00000000*/
                            /*
                               Specifies the decode mode for the far memory
                               targets on one side of the die for DRAM rule 13
                            */
    UINT32 fm_mod_14 : 2;
                            /* Bits[29:28], Access Type=RW, default=0x00000000*/
                            /*
                               Specifies the decode mode for the far memory
                               targets on one side of the die for DRAM rule 14
                            */
    UINT32 fm_mod_15 : 2;
                            /* Bits[31:30], Access Type=RW, default=0x00000000*/
                            /*
                               Specifies the decode mode for the far memory
                               targets on one side of the die for DRAM rule 15
                            */

  } Bits;
  UINT32 Data;
} DRAM_H0_RT2_MODE0_CHABC_SAD_STRUCT;

/* MC_MAIN.h */
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32 rule_enable : 1;
                            /* Bits[0:0], Access Type=RW, default=0x00000000*/
                            /* Enable for this DRAM rule. */
    UINT32 limit : 26;
                            /* Bits[26:1], Access Type=RW, default=0x00000000*/
                            /*
                               This correspond to Addr[51:26] of the DRAM rule
                               top limit address. Must be strickly greater then
                               previous rule, even if this rule is disabled,
                               unless this rule and all following rules are
                               disabled. Lower limit is the previous rule (or 0
                               if this is the first rule)
                            */
    UINT32 fm_target : 2;
                            /* Bits[28:27], Access Type=RW, default=0x00000000*/
                            /*
                               FmTarget - Target Far Memory MC; Bit 0 defines
                               the half target (KNH) that all traffic go to.
                            */
    UINT32 fm_chn_ways : 2;
                            /* Bits[30:29], Access Type=RW, default=0x00000000*/
                            /*
                               FmChnWays - Far Memory Channel Ways: used to
                               calculate ChnLID 00 = 1 way (ChnLID = 0) 01 = 2
                               ways (ChnLID = modulo 2 of System Address) 10 =
                               3 ways (ChnLID = modulo 3 of System Address) 11
                               = Reserved
                            */
    UINT32 rsvd : 1;
                            /* Bits[31:31], Access Type=RO, default=None*/
                            /* Reserved */
  } Bits;
  UINT32 Data;
} DRAM_RULE_CFG0_N0_MC_MAIN_STRUCT;

typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32 fm_chn_gran : 3;
                            /* Bits[2:0], Access Type=RW, default=0x00000000*/
                            /*
                               FmChnGran - Far Memory Channel Granularity:
                               001=256B, 010=4KB, 111=2KB, other reserved
                            */
    UINT32 fm_target_ways : 3;
                            /* Bits[5:3], Access Type=RW, default=0x00000000*/
                            /*
                               FmTargetWays - Far Memory Target Ways: used to
                               calculate ChnLID when fm_gran_eq = 1 000 = 1 way
                               001 = 2 ways 010 = 4 ways 011 = 8 ways 100 = 16
                               ways
                            */
    UINT32 fm_gran_eq : 1;
                            /* Bits[6:6], Access Type=RW, default=0x00000000*/
                            /*
                               When set to 0, indicate far memory target and
                               channel granularity are different, or target way
                               is 1.
                            */
    UINT32 fm_chn_l2pid : 6;
                            /* Bits[12:7], Access Type=RW, default=0x00000024*/
                            /*
                               FmChnL2PID - translation table for Logical
                               (ChnLID) to Physical ID (ChnPID) FmChnL2PID[1:0]
                               = mapping from ChnLID=0 to ChnPID
                               FmChnL2PID[3:2] = mapping from ChnLID=1 to
                               ChnPID FmChnL2PID[5:4] = mapping from ChnLID=2
                               to ChnPID ChnPID values are 00-01-10, value 11
                               is not valid
                            */
    UINT32 rsvd : 19;
                            /* Bits[31:13], Access Type=RO, default=None*/
                            /* Reserved */
  } Bits;
  UINT32 Data;
} DRAM_RULE_CFG0_N1_MC_MAIN_STRUCT;

/* CHABC_SAD1.h */
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32 block_base : 26;
                            /* Bits[25:0], Access Type=RW, default=0x00000000*/
                            /*
                               Base address[51:26] of block/mailbox decoder
                               region.
                            */
    UINT32 rsvd : 6;
                            /* Bits[31:26], Access Type=RO, default=None*/
                            /* Reserved */
  } Bits;
  UINT32 Data;
} BLOCK_DECODER_ADDR_N0_CHABC_SAD1_STRUCT;

typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32 block_limit : 26;
                            /* Bits[25:0], Access Type=RW, default=0x00000000*/
                            /*
                               Limit Address [51:26] of block/mailbox decoder
                               region.
                            */
    UINT32 block_rule_en : 1;
                            /* Bits[26:26], Access Type=RW, default=0x00000000*/
                            /* Enable for the Block Decoder Rule - turns it on. */
    UINT32 rsvd : 5;
                            /* Bits[31:27], Access Type=RO, default=None*/
                            /* Reserved */
  } Bits;
  UINT32 Data;
} BLOCK_DECODER_ADDR_N1_CHABC_SAD1_STRUCT;

/* CHA_MISC.h */
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32 target_id_0 : 4;
                            /* Bits[3:0], Access Type=RW, default=0x00000000*/
                            /*
                               This is the route table struct for tracking the
                               mapping of the SAD registers into memory
                               controllers on the die for the 0th target memory
                               controller
                            */
    UINT32 target_id_1 : 4;
                            /* Bits[7:4], Access Type=RW, default=0x00000000*/
                            /*
                               This is the route table struct for tracking the
                               mapping of the SAD registers into memory
                               controllers on the die for the 1st target memory
                               controller
                            */
    UINT32 target_id_2 : 4;
                            /* Bits[11:8], Access Type=RW, default=0x00000000*/
                            /*
                               This is the route table struct for tracking the
                               mapping of the SAD registers into memory
                               controllers on the die for the 2nd target memory
                               controller
                            */
    UINT32 target_id_3 : 4;
                            /* Bits[15:12], Access Type=RW, default=0x00000000*/
                            /*
                               This is the route table struct for tracking the
                               mapping of the SAD registers into memory
                               controllers on the die for the 3rd target memory
                               controller
                            */
    UINT32 target_id_4 : 4;
                            /* Bits[19:16], Access Type=RW, default=0x00000000*/
                            /*
                               This is the route table struct for tracking the
                               mapping of the SAD registers into memory
                               controllers on the die for the 4th target memory
                               controller
                            */
    UINT32 target_id_5 : 4;
                            /* Bits[23:20], Access Type=RW, default=0x00000000*/
                            /*
                               This is the route table struct for tracking the
                               mapping of the SAD registers into memory
                               controllers on the die for the 5th target memory
                               controller
                            */
    UINT32 target_id_6 : 4;
                            /* Bits[27:24], Access Type=RW, default=0x00000000*/
                            /*
                               This is the route table struct for tracking the
                               mapping of the SAD registers into memory
                               controllers on the die for the 6th target memory
                               controller
                            */
    UINT32 target_id_7 : 4;
                            /* Bits[31:28], Access Type=RW, default=0x00000000*/
                            /*
                               This is the route table struct for tracking the
                               mapping of the SAD registers into memory
                               controllers on the die for the 7th target memory
                               controller
                            */
  } Bits;
  UINT32 Data;
} H0_TGT_ROUTE_TABLE_0_CHA_MISC_STRUCT;

typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32 channel_id_0 : 2;
                            /* Bits[1:0], Access Type=RW, default=0x00000000*/
                            /*
                               This is the route table struct for tracking the
                               mapping of the SAD registers into the memory
                               channels on the die This is for channel 0
                            */
    UINT32 channel_id_1 : 2;
                            /* Bits[3:2], Access Type=RW, default=0x00000000*/
                            /*
                               This is the route table struct for tracking the
                               mapping of the SAD registers into the memory
                               channels on the die This is for channel 1
                            */
    UINT32 channel_id_2 : 2;
                            /* Bits[5:4], Access Type=RW, default=0x00000000*/
                            /*
                               This is the route table struct for tracking the
                               mapping of the SAD registers into the memory
                               channels on the die This is for channel 2
                            */
    UINT32 channel_id_3 : 2;
                            /* Bits[7:6], Access Type=RW, default=0x00000000*/
                            /*
                               This is the route table struct for tracking the
                               mapping of the SAD registers into the memory
                               channels on the die This is for channel 3
                            */
    UINT32 channel_id_4 : 2;
                            /* Bits[9:8], Access Type=RW, default=0x00000000*/
                            /*
                               This is the route table struct for tracking the
                               mapping of the SAD registers into the memory
                               channels on the die This is for channel 4
                            */
    UINT32 channel_id_5 : 2;
                            /* Bits[11:10], Access Type=RW, default=0x00000000*/
                            /*
                               This is the route table struct for tracking the
                               mapping of the SAD registers into the memory
                               channels on the die This is for channel 5
                            */
    UINT32 channel_id_6 : 2;
                            /* Bits[13:12], Access Type=RW, default=0x00000000*/
                            /*
                               This is the route table struct for tracking the
                               mapping of the SAD registers into the memory
                               channels on the die This is for channel 6
                            */
    UINT32 channel_id_7 : 2;
                            /* Bits[15:14], Access Type=RW, default=0x00000000*/
                            /*
                               This is the route table struct for tracking the
                               mapping of the SAD registers into the memory
                               channels on the die This is for channel 7
                            */
    UINT32 rsvd : 16;
                            /* Bits[31:16], Access Type=RO, default=None*/
                            /* Reserved */
  } Bits;
  UINT32 Data;
} H0_CH_ROUTE_TABLE_0_CHA_MISC_STRUCT;

typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32 target_id_0 : 4;

                            /* Bits[3:0], Access Type=RW, default=0x00000000*/

                            /*
                               This is the route table struct for tracking the
                               mapping of the SAD registers into memory
                               controllers on the die for the 0th target memory
                               controller
                            */
    UINT32 target_id_1 : 4;

                            /* Bits[7:4], Access Type=RW, default=0x00000000*/

                            /*
                               This is the route table struct for tracking the
                               mapping of the SAD registers into memory
                               controllers on the die for the 1st target memory
                               controller
                            */
    UINT32 target_id_2 : 4;

                            /* Bits[11:8], Access Type=RW, default=0x00000000*/

                            /*
                               This is the route table struct for tracking the
                               mapping of the SAD registers into memory
                               controllers on the die for the 2nd target memory
                               controller
                            */
    UINT32 target_id_3 : 4;

                            /* Bits[15:12], Access Type=RW, default=0x00000000*/

                            /*
                               This is the route table struct for tracking the
                               mapping of the SAD registers into memory
                               controllers on the die for the 3rd target memory
                               controller
                            */
    UINT32 target_id_4 : 4;

                            /* Bits[19:16], Access Type=RW, default=0x00000000*/

                            /*
                               This is the route table struct for tracking the
                               mapping of the SAD registers into memory
                               controllers on the die for the 4th target memory
                               controller
                            */
    UINT32 target_id_5 : 4;

                            /* Bits[23:20], Access Type=RW, default=0x00000000*/

                            /*
                               This is the route table struct for tracking the
                               mapping of the SAD registers into memory
                               controllers on the die for the 5th target memory
                               controller
                            */
    UINT32 target_id_6 : 4;

                            /* Bits[27:24], Access Type=RW, default=0x00000000*/

                            /*
                               This is the route table struct for tracking the
                               mapping of the SAD registers into memory
                               controllers on the die for the 6th target memory
                               controller
                            */
    UINT32 target_id_7 : 4;

                            /* Bits[31:28], Access Type=RW, default=0x00000000*/

                            /*
                               This is the route table struct for tracking the
                               mapping of the SAD registers into memory
                               controllers on the die for the 7th target memory
                               controller
                            */

  } Bits;
  UINT32 Data;

} H0_TGT_ROUTE_TABLE_2LM_CHA_MISC_STRUCT;

typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32 target_id_0 : 4;

                            /* Bits[3:0], Access Type=RW, default=0x00000000*/

                            /*
                               This is the route table struct for tracking the
                               mapping of the SAD registers into memory
                               controllers on the die for the 0th target memory
                               controller
                            */
    UINT32 target_id_1 : 4;

                            /* Bits[7:4], Access Type=RW, default=0x00000000*/

                            /*
                               This is the route table struct for tracking the
                               mapping of the SAD registers into memory
                               controllers on the die for the 1st target memory
                               controller
                            */
    UINT32 target_id_2 : 4;

                            /* Bits[11:8], Access Type=RW, default=0x00000000*/

                            /*
                               This is the route table struct for tracking the
                               mapping of the SAD registers into memory
                               controllers on the die for the 2nd target memory
                               controller
                            */
    UINT32 target_id_3 : 4;

                            /* Bits[15:12], Access Type=RW, default=0x00000000*/

                            /*
                               This is the route table struct for tracking the
                               mapping of the SAD registers into memory
                               controllers on the die for the 3rd target memory
                               controller
                            */
    UINT32 target_id_4 : 4;

                            /* Bits[19:16], Access Type=RW, default=0x00000000*/

                            /*
                               This is the route table struct for tracking the
                               mapping of the SAD registers into memory
                               controllers on the die for the 4th target memory
                               controller
                            */
    UINT32 target_id_5 : 4;

                            /* Bits[23:20], Access Type=RW, default=0x00000000*/

                            /*
                               This is the route table struct for tracking the
                               mapping of the SAD registers into memory
                               controllers on the die for the 5th target memory
                               controller
                            */
    UINT32 target_id_6 : 4;

                            /* Bits[27:24], Access Type=RW, default=0x00000000*/

                            /*
                               This is the route table struct for tracking the
                               mapping of the SAD registers into memory
                               controllers on the die for the 6th target memory
                               controller
                            */
    UINT32 target_id_7 : 4;

                            /* Bits[31:28], Access Type=RW, default=0x00000000*/

                            /*
                               This is the route table struct for tracking the
                               mapping of the SAD registers into memory
                               controllers on the die for the 7th target memory
                               controller
                            */

  } Bits;
  UINT32 Data;

} H1_TGT_ROUTE_TABLE_2LM_CHA_MISC_STRUCT;

/* ServerSiliconPkg/Mem/Library/MemMcIpLib/Include/Registers/Icx/MC_MAIN.h:753 */
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32 target_ways : 3;

                            /* Bits[2:0], Access Type=RW, default=0x00000000*/

                            /*
                               target interleave wayness 000 = 1 way, 001 = 2
                               way, 010 = 4 way, 011 = 8 way. 100 = 16 way.
                            */
    UINT32 chn_ways : 3;

                            /* Bits[5:3], Access Type=RW, default=0x00000000*/

                            /*
                               Channel interleave wayness 000 = 1 way 001 = 2
                               way 010 = 3 way 011 = 8 way 100 = 4 way 101 -
                               111 = Reserved
                            */
    UINT32 target_gran : 4;

                            /* Bits[9:6], Access Type=RW, default=0x00000000*/

                            /*
                               Specifies the granularity of the skt_way
                               interleave b0001 256B (based off PA[8] and up)
                               b0010 4KB (based off PA[12] and up) b0111 2KB
                               (based off PA[11] and up) Others Reserved Note:
                               Only certain combinations of
                               chn_gran/target_gran are valid.
                            */
    UINT32 chn_gran : 4;

                            /* Bits[13:10], Access Type=RW, default=0x00000000*/

                            /*
                               Specifies the granularity of the ch_way
                               interleave b0001 256B (based off PA[8] and up)
                               b0010 4KB (based off PA[12] and up) b0111 2KB
                               (based off PA[11] and up) Others Reserved Note:
                               Only certain combinations of
                               chn_gran/target_gran are valid.
                            */
    UINT32 tad_offset : 18;

                            /* Bits[31:14], Access Type=RW, default=0x00000000*/

                            /*
                               channel interleave 0 offset, i.e.
                               CHANNELOFFSET[51:26] == channel interleave i
                               offset, 64MB granularity. tad_offset[25:22] are
                               reserved and should be kept at 0. When negative
                               offset is programmed (2's complement), this
                               field only support +/-32T and the upper bit is
                               used to indicate the polarity of the offset.
                            */

  } Bits;
  UINT32 Data;

} TADCHNILVOFFSET_0_N0_MC_MAIN_STRUCT;

typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32 tad_offset : 8;

                            /* Bits[7:0], Access Type=RW, default=0x00000000*/

                            /*
                               channel interleave 0 offset, i.e.
                               CHANNELOFFSET[51:26] == channel interleave i
                               offset, 64MB granularity. tad_offset[25:22] are
                               reserved and should be kept at 0. When negative
                               offset is programmed (2's complement), this
                               field only support +/-32T and the upper bit is
                               used to indicate the polarity of the offset.
                            */
    UINT32 tad_offset_sign : 1;

                            /* Bits[8:8], Access Type=RW, default=0x00000000*/

                            /*
                               When 0, the tad_offset is subtracted from the
                               system address. When 1, the tad_offset is added
                               to the system address.
                            */
    UINT32 rsvd : 23;

                            /* Bits[31:9], Access Type=RO, default=None*/

                            /* Reserved */

  } Bits;
  UINT32 Data;

} TADCHNILVOFFSET_0_N1_MC_MAIN_STRUCT;

/* ServerSiliconPkg/Include/Registers/Icx/M2MEM_MAIN.h:7161 */
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32 tadid : 5;

                            /* Bits[4:0], Access Type=RW/P, default=None*/

                            /*
                               TAD Table SAD Entry/Index (TadId): Specifies
                               what TAD entry in the TAD table to read
                               indirectly through this register. There are 20
                               TAD entries so the legal range is 0-19.
                            */
    UINT32 rsvd : 1;

                            /* Bits[5:5], Access Type=RO, default=None*/

                            /* Reserved */
    UINT32 tadrden : 1;

                            /* Bits[6:6], Access Type=RW/V, default=None*/

                            /*
                               TAD Read (TadRdEn): Read the TAD table entry.
                               The read information will get stored in this
                               register. Hardware will read the TAD table entry
                               when seeing a zero-to-one transition on this
                               bitfield. Hardware will reset this bit to 0 the
                               cycle after it is written to 1.
                            */
    UINT32 tadvld : 1;

                            /* Bits[7:7], Access Type=RW/V/P, default=None*/

                            /*
                               TAD Table Entry Valid (TadVld): Specifies
                               whether this TAD entry in the TAD table is
                               valid. It is illegal to program TadVld[N]==1 if
                               TadVld[N-1]==0 (for N>0).
                            */
    UINT32 ddrtadid : 4;

                            /* Bits[11:8], Access Type=RW/V/P, default=None*/

                            /*
                               DDR TAD entry (DDRtadId): DDR TAD entry
                               associated with this TAD table entry. M2mem puts
                               no restrictions on the values here, but this
                               field should be programmed consistent with the
                               channel's capabilities. There are 12 TAD entries
                               in the channel, hence the current legal range is
                               0-11. DDR4 can only use the first 8 TAD entries,
                               so the legal range is 0-7 for regions with the
                               DDR4 bit set. HBM should only use TAD entry 0.
                            */
    UINT32 rsvd_12 : 4;

                            /* Bits[15:12], Access Type=RO, default=None*/

                            /* Reserved */
    UINT32 pmemvld : 1;

                            /* Bits[16:16], Access Type=RW/V/P, default=None*/

                            /*
                               PMem Region Valid (PmemVld): This TAD entry
                               represents a persistent memory region.
                            */
    UINT32 blkvld : 1;

                            /* Bits[17:17], Access Type=RW/V/P, default=None*/

                            /*
                               Block Region Valid (BlkVld): This TAD entry
                               represents a block region.
                            */
    UINT32 ddr4 : 1;

                            /* Bits[18:18], Access Type=RW/V/P, default=None*/

                            /*
                               DDR4: This TAD entry represents near-memory at
                               NMC or 1LM DDR4. In non-Xtile config: this just
                               represents the 1LM DDR4 region. In Xtile config:
                               this represents NM access at the near-memory
                               controller (or DDR4 access at the FMC). BIOS to
                               calculate as follows: '(!PmemVld & !BlockVld &
                               (!NmCacheableVld | Mode[FmcXtile])) | (PmemVld &
                               DDR4_NVDIMM)'
                            */
    UINT32 nmcacheablevld : 1;

                            /* Bits[19:19], Access Type=RW/V/P, default=None*/

                            /*
                               Near-Memory Cacheable Valid (NmCacheableVld):
                               This TAD entry represents a near-memory cached
                               region, i.e. not a 1LM region.
                            */
    UINT32 mirror : 1;

                            /* Bits[20:20], Access Type=RW/V/P, default=None*/

                            /*
                               Mirror region: This TAD entry represents a
                               mirroring region.
                            */
    UINT32 secondary1st : 1;

                            /* Bits[21:21], Access Type=RW/V/P, default=None*/

                            /*
                               Read secondary mirror channel first
                               (Secondary1st): For this memory region, read
                               secondary mirror channel first. Used in
                               mirroring to prevent (non error flow) accesses
                               to a primary mirror channel for a bad primary
                               region/address. Note, if an error flow does get
                               invoked because of an error on secondary for an
                               access to this region then the primary storage
                               will still be accessed (when channel not failed
                               over).
                            */
    UINT32 frcnpwr : 1;

                            /* Bits[22:22], Access Type=RW/V/P, default=None*/

                            /*
                               Force Non-Posted Writes (FrcNPWr): This TAD
                               entry represents region that requires NP write
                               semantics on CMI. Only applies to the far-memory
                               region if this is a 2LM region.
                            */
    UINT32 lowbw : 1;

                            /* Bits[23:23], Access Type=RW/V/P, default=None*/

                            /*
                               Low Bandwidth Memory (LowBW): This TAD entry
                               represents a region for which CHA uses a low
                               bandwidth credit. Setting this bit causes m2mem
                               to set the ddrt_cdt bit on BL credit returns for
                               writes targeting this region.
                            */
    UINT32 spare : 2;

                            /* Bits[25:24], Access Type=RW/V/P, default=None*/

                            /* Spare: Spare bits to describe this TAD entry. */
    UINT32 addresslimit : 6;

                            /* Bits[31:26], Access Type=RW/V/P, default=None*/

                            /*
                               Limit address Specifies the upper limit
                               (inclusive) for address[51:26] (64MB aligned) to
                               be part of this memory region. An address hits a
                               particular TAD entry if all of the following
                               conditions are true (where 'N' is the TAD entry
                               number): [list] [*]TadVld[N] == 1
                               [*]address[51:26] <= AddressLimit[N]
                               [*]address[51:26] > AddressLimit[N-1] (only for
                               N>0) [/list] System firmware must program the
                               TAD AddressLimit fields such that
                               AddressLimit[N] > AddressLimit[N-1] for all N>0
                               with TadVld[N]==1.
                            */

  } Bits;
  UINT32 Data;

} TAD_RD_N0_M2MEM_MAIN_STRUCT;

typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32 addresslimit : 20;

                            /* Bits[19:0], Access Type=RW/V/P, default=None*/

                            /*
                               Limit address Specifies the upper limit
                               (inclusive) for address[51:26] (64MB aligned) to
                               be part of this memory region. An address hits a
                               particular TAD entry if all of the following
                               conditions are true (where 'N' is the TAD entry
                               number): [list] [*]TadVld[N] == 1
                               [*]address[51:26] <= AddressLimit[N]
                               [*]address[51:26] > AddressLimit[N-1] (only for
                               N>0) [/list] System firmware must program the
                               TAD AddressLimit fields such that
                               AddressLimit[N] > AddressLimit[N-1] for all N>0
                               with TadVld[N]==1.
                            */
    UINT32 rsvd : 12;

                            /* Bits[31:20], Access Type=RO, default=None*/

                            /* Reserved */

  } Bits;
  UINT32 Data;

} TAD_RD_N1_M2MEM_MAIN_STRUCT;

/* ServerSiliconPkg/Include/Registers/Icx/M2MEM_MAIN.h:10962 */
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32 ddr4chnlfailed : 3;

                            /* Bits[2:0], Access Type=RW/V, default=0x00000000*/

                            /*
                               DDR4 Channel Failed (Ddr4ChnlFailed): Each bit
                               is an status and control bit for a channel
                               indicating whether the DDR4 memory associated
                               with that channel failed over. Bits 2...0 map to
                               channels 2...0. Channel failed over semantics:
                               (i) reads will no longer access this channel,
                               (ii) writes will no longer go to this channel
                               (unless ForceWrsToFailed is set). Writing a bit
                               to 1 will force that DDR4 channel to go to
                               failed over state.
                            */
    UINT32 rsvd : 1;

                            /* Bits[3:3], Access Type=RO, default=None*/

                            /* Reserved */
    UINT32 ddrtchnlfailed : 3;

                            /* Bits[6:4], Access Type=RW/V, default=0x00000000*/

                            /*
                               DDRT Channel Failed (DdrtChnlFailed): Each bit
                               is a status and control bit for a channel
                               indicating whether DDRT memory associated with
                               that channel failed over. Bits 6...4 map to
                               channels 2...0. Channel failed over semantics:
                               (i) reads will no longer access this channel,
                               (ii) writes will no longer go to this channel
                               (unless ForceWrsToFailed is set). Writing a bit
                               to 1 will force that DDR4 channel to go to
                               failed over state.
                            */
    UINT32 rsvd_7 : 1;

                            /* Bits[7:7], Access Type=RO, default=None*/

                            /* Reserved */
    UINT32 ddr4frcmirrwrstofailed : 1;

                            /* Bits[8:8], Access Type=RW, default=0x00000000*/

                            /*
                               DDR4 Force Mirror Writes to Failed Channel
                               (Ddr4FrcMirrWrsToFailed): Description is
                               identical to above field, but in this case
                               applicable to DDR4 mirror channels.
                            */
    UINT32 ddrt4frcmirrwrstofailed : 1;

                            /* Bits[9:9], Access Type=RW, default=0x00000000*/

                            /*
                               DDRT Force Mirror Writes to Failed Channel
                               (DDRT4FrcMirrWrsToFailed): When set, it forces
                               channel mirror writes to both mirroring channels
                               even after failover. This should only be set
                               during system quiesce, i.e. when no mesh2mem
                               traffic. 0: Dont send writes to the failover
                               channel. This is the default production setting.
                               1: Force writes to also go to the failover
                               channel. This is a debug mode. The usage model
                               is to facilitate post silicon exercising of
                               channel mirroring while injection uncorrectables
                               on one channel. This will keep writes going to
                               the failed channel so that later on one can undo
                               the failover and continue testing channel
                               mirroring with failover events.
                            */
    UINT32 rsvd_10 : 22;

                            /* Bits[31:10], Access Type=RO, default=None*/

                            /* Reserved */

  } Bits;
  UINT32 Data;

} MIRRORFAILOVER_M2MEM_MAIN_STRUCT;

typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32 ddr4chnl0secondary : 2;

                            /* Bits[1:0], Access Type=RW, default=0x00000001*/

                            /*
                               DDR4 Channel 0 secondary (Ddr4Chnl0Secondary):
                               Secondary channel associated with primary
                               channel 0.
                            */
    UINT32 ddr4chnl1secondary : 2;

                            /* Bits[3:2], Access Type=RW, default=0x00000002*/

                            /*
                               DDR4 Channel 1 secondary (Ddr4Chnl1Secondary):
                               Secondary channel associated with primary
                               channel 1.
                            */
    UINT32 ddr4chnl2secondary : 2;

                            /* Bits[5:4], Access Type=RW, default=0x00000000*/

                            /*
                               DDR4 Channel 2 secondary (Ddr4Chnl2Secondary):
                               Secondary channel associated with primary
                               channel 2.
                            */
    UINT32 ddrtchnl0secondary : 2;

                            /* Bits[7:6], Access Type=RW, default=0x00000001*/

                            /*
                               DDR4 Channel 0 secondary (DdrtChnl0Secondary):
                               Secondary channel associated with primary
                               channel 0.
                            */
    UINT32 ddrtchnl1secondary : 2;

                            /* Bits[9:8], Access Type=RW, default=0x00000002*/

                            /*
                               DDR4 Channel 1 secondary (DdrtChnl1Secondary):
                               Secondary channel associated with primary
                               channel 1.
                            */
    UINT32 ddrtchnl2secondary : 2;

                            /* Bits[11:10], Access Type=RW, default=0x00000000*/

                            /*
                               DDRT Channel 2 secondary (DDRTChnl2Secondary):
                               Secondary channel associated with primary
                               channel 2.
                            */
    UINT32 rsvd : 20;

                            /* Bits[31:12], Access Type=RO, default=None*/

                            /* Reserved */

  } Bits;
  UINT32 Data;

} MIRRORCHNLS_M2MEM_MAIN_STRUCT;

/* ServerSiliconPkg/Include/Registers/Icx/CHABC_SAD.h:2438 */
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32 package0 : 4;

                            /* Bits[3:0], Access Type=RW, default=0x00000008*/

                            /*
                               Interleave list target. Encoding is as follows:
                               4b0xyz - target is a remote socket with
                               NodeId=3bxyz 4b1xyz - target is a local memory
                               controller which maps to MC_ROUTE_TABLE index
                               3bxyz Note: in the 4b1xyz case, if mod3==1, the
                               MC_ROUTE_TABLE index is actually {mod3[1:0],z}
                               (i.e., xy is ignored)
                            */
    UINT32 package1 : 4;

                            /* Bits[7:4], Access Type=RW, default=0x00000008*/

                            /*
                               Interleave list target. Encoding is as follows:
                               4b0xyz - target is a remote socket with
                               NodeId=3bxyz 4b1xyz - target is a local memory
                               controller which maps to MC_ROUTE_TABLE index
                               3bxyz Note: in the 4b1xyz case, if mod3==1, the
                               MC_ROUTE_TABLE index is actually {mod3[1:0],z}
                               (i.e., xy is ignored)
                            */
    UINT32 package2 : 4;

                            /* Bits[11:8], Access Type=RW, default=0x00000008*/

                            /*
                               Interleave list target. Encoding is as follows:
                               4b0xyz - target is a remote socket with
                               NodeId=3bxyz 4b1xyz - target is a local memory
                               controller which maps to MC_ROUTE_TABLE index
                               3bxyz Note: in the 4b1xyz case, if mod3==1, the
                               MC_ROUTE_TABLE index is actually {mod3[1:0],z}
                               (i.e., xy is ignored)
                            */
    UINT32 package3 : 4;

                            /* Bits[15:12], Access Type=RW, default=0x00000008*/

                            /*
                               Interleave list target. Encoding is as follows:
                               4b0xyz - target is a remote socket with
                               NodeId=3bxyz 4b1xyz - target is a local memory
                               controller which maps to MC_ROUTE_TABLE index
                               3bxyz Note: in the 4b1xyz case, if mod3==1, the
                               MC_ROUTE_TABLE index is actually {mod3[1:0],z}
                               (i.e., xy is ignored)
                            */
    UINT32 package4 : 4;

                            /* Bits[19:16], Access Type=RW, default=0x00000008*/

                            /*
                               Interleave list target. Encoding is as follows:
                               4b0xyz - target is a remote socket with
                               NodeId=3bxyz 4b1xyz - target is a local memory
                               controller which maps to MC_ROUTE_TABLE index
                               3bxyz Note: in the 4b1xyz case, if mod3==1, the
                               MC_ROUTE_TABLE index is actually {mod3[1:0],z}
                               (i.e., xy is ignored)
                            */
    UINT32 package5 : 4;

                            /* Bits[23:20], Access Type=RW, default=0x00000008*/

                            /*
                               Interleave list target. Encoding is as follows:
                               4b0xyz - target is a remote socket with
                               NodeId=3bxyz 4b1xyz - target is a local memory
                               controller which maps to MC_ROUTE_TABLE index
                               3bxyz Note: in the 4b1xyz case, if mod3==1, the
                               MC_ROUTE_TABLE index is actually {mod3[1:0],z}
                               (i.e., xy is ignored)
                            */
    UINT32 package6 : 4;

                            /* Bits[27:24], Access Type=RW, default=0x00000008*/

                            /*
                               Interleave list target. Encoding is as follows:
                               4b0xyz - target is a remote socket with
                               NodeId=3bxyz 4b1xyz - target is a local memory
                               controller which maps to MC_ROUTE_TABLE index
                               3bxyz Note: in the 4b1xyz case, if mod3==1, the
                               MC_ROUTE_TABLE index is actually {mod3[1:0],z}
                               (i.e., xy is ignored)
                            */
    UINT32 package7 : 4;

                            /* Bits[31:28], Access Type=RW, default=0x00000008*/

                            /*
                               Interleave list target. Encoding is as follows:
                               4b0xyz - target is a remote socket with
                               NodeId=3bxyz 4b1xyz - target is a local memory
                               controller which maps to MC_ROUTE_TABLE index
                               3bxyz Note: in the 4b1xyz case, if mod3==1, the
                               MC_ROUTE_TABLE index is actually {mod3[1:0],z}
                               (i.e., xy is ignored)
                            */

  } Bits;
  UINT32 Data;

} INTERLEAVE_LIST_CFG_0_CHABC_SAD_STRUCT;

/* ServerSiliconPkg/Include/Registers/Icx/CHABC_SAD.h:5664 */
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32 rule_enable : 1;

                            /* Bits[0:0], Access Type=RW, default=0x00000000*/

                            /* Enable for this DRAM rule. */
    UINT32 rsvd : 2;

                            /* Bits[2:1], Access Type=RO, default=None*/

                            /* Reserved */
    UINT32 attr : 2;

                            /* Bits[4:3], Access Type=RW, default=0x00000000*/

                            /*
                               (p)Sets the memory type for the remote DRAM rule
                               according to the following encoding.(/p) (p)00 -
                               Coherent DRAM(/p) (p)01 - Memory Mapped CFG(/p)
                               (p)10 - Low Bandwidth Coherent DRAM(/p) (p)11 -
                               High Bandwidth Coherent DRAM(/p)
                            */
    UINT32 rsvd_5 : 1;

                            /* Bits[5:5], Access Type=RO, default=None*/

                            /* Reserved */
    UINT32 base : 26;

                            /* Bits[31:6], Access Type=RW, default=0x00000000*/

                            /*
                               This corresponds to Addr[51:26] of the DRAM
                               rule's&nbsp;base address.
                            */

  } Bits;
  UINT32 Data;

} REMOTE_DRAM_RULE_CFG_0_N0_CHABC_SAD_STRUCT;

typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32 pkg : 3;

                            /* Bits[2:0], Access Type=RW, default=0x00000000*/

                            /*
                               (p)Target NodeID for this remote DRAM rule.
                               Target node is only 3 bits because the target is
                               assumed to be a remote target. Placing the local
                               socket NodeID in as the target for this rule is
                               not supported.(/p)
                            */
    UINT32 rsvd : 3;

                            /* Bits[5:3], Access Type=RO, default=None*/

                            /* Reserved */
    UINT32 limit : 26;

                            /* Bits[31:6], Access Type=RW, default=0x00000000*/

                            /*
                               This correspond to Addr[51:26] of the DRAM rule
                               top limit address.
                            */

  } Bits;
  UINT32 Data;

} REMOTE_DRAM_RULE_CFG_0_N1_CHABC_SAD_STRUCT;

/* ServerSiliconPkg/Include/Registers/Icx/CHA_PMA.h:1927 */
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32 full_snc_en : 1;

                            /* Bits[0:0], Access Type=RW, default=0x00000000*/

                            /* full snc enable */
    UINT32 snc_ind_en : 1;

                            /* Bits[1:1], Access Type=RW, default=0x00000000*/

                            /* SNC IND enable */
    UINT32 num_snc_clu : 2;

                            /* Bits[3:2], Access Type=RW, default=0x00000000*/

                            /* Num of SNC Clusters */
    UINT32 sncbase1id : 6;

                            /* Bits[9:4], Access Type=RW, default=0x00000000*/

                            /* SNC cluster 1 base mesh ID */
    UINT32 sncbase2id : 6;

                            /* Bits[15:10], Access Type=RW, default=0x00000000*/

                            /* SNC cluster 2 base mesh ID */
    UINT32 sncbase3id : 6;

                            /* Bits[21:16], Access Type=RW, default=0x00000000*/

                            /* SNC cluster 3 base mesh ID */
    UINT32 rsvd : 10;

                            /* Bits[31:22], Access Type=RO, default=None*/

                            /* Reserved */

  } Bits;
  UINT32 Data;

} CHA_SNC_CONFIG_CHA_PMA_STRUCT;

/* ServerSiliconPkg/Include/Registers/Icx/IIO_VTD.h:8707 */
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32 uma_clustering_enable : 1;

                            /* Bits[0:0], Access Type=RW/P, default=0x00000000*/

                            /*
                               Enable bit for UMA based clustering mode.
                               Overrides FULL_SNC_ENABLE and SNC_IND_ENABLE if
                               set.
                            */
    UINT32 uma_defeature_xor : 1;

                            /* Bits[1:1], Access Type=RW/P, default=0x00000000*/

                            /*
                               Enable bit for XOR defeature mode. By default,
                               UMA mode clustering uses
                               Addr[26:25]^Addr[18:17]^Addr[12:11]^Addr[9:8].
                               But when this bit is set to 1, Addr[12:11] are
                               used.
                            */
    UINT32 rsvd : 30;

                            /* Bits[31:2], Access Type=RO, default=None*/

                            /* Reserved */

  } Bits;
  UINT32 Data;

} UMA_CLUSTER_CONFIG_IIO_VTD_STRUCT;

typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32 starting_addr_0 : 16;

                            /* Bits[15:0], Access Type=RW/P, default=0x00000000*/

                            /*
                               Starting Address for Cluster 0[br] Base Address
                               [45:30]. This is the 1G aligned base address of
                               local cluster 0. Any address less than this
                               address will have SNLocal=0.
                            */
    UINT32 address_mask_45_40 : 6;

                            /* Bits[21:16], Access Type=RW/P, default=0x00000000*/

                            /*
                               Controls bits [45:40] of the 2LM hash address
                               mask
                            */
    UINT32 address_mask_hi_51_46 : 6;

                            /* Bits[27:22], Access Type=RW/P, default=0x00000000*/

                            /*
                               High-order Controls bits [51:46] of the 2LM hash
                               address mask
                            */
    UINT32 rsvd : 4;

                            /* Bits[31:28], Access Type=RO, default=None*/

                            /* Reserved */

  } Bits;
  UINT32 Data;

} SNC_BASE_1_IIO_VTD_STRUCT;

typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32 starting_addr_upper_1 : 6;

                            /* Bits[5:0], Access Type=RW/P, default=0x00000000*/

                            /*
                               Starting Address Upper Address bits [51:46] for
                               Cluster 1
                            */
    UINT32 starting_addr_upper_2 : 6;

                            /* Bits[11:6], Access Type=RW/P, default=0x00000000*/

                            /*
                               Starting Address Upper Address bits [51:46] for
                               Cluster 2
                            */
    UINT32 starting_addr_upper_3 : 6;

                            /* Bits[17:12], Access Type=RW/P, default=0x00000000*/

                            /*
                               Starting Address Upper Address bits [51:46] for
                               Cluster 3
                            */
    UINT32 starting_addr_upper_4 : 6;

                            /* Bits[23:18], Access Type=RW/P, default=0x00000000*/

                            /*
                               Starting Address Upper Address bits [51:46] for
                               Cluster 4
                            */
    UINT32 starting_addr_upper_5 : 6;

                            /* Bits[29:24], Access Type=RW/P, default=0x00000000*/

                            /*
                               Starting Address Upper Address bits [51:46] for
                               Cluster 5[br] Note there is no fifth cluster,
                               but this register is still needed because the
                               base address of the fifth cluster sets the limit
                               address for the fourth cluster.
                            */
    UINT32 rsvd : 2;

                            /* Bits[31:30], Access Type=RO, default=None*/

                            /* Reserved */

  } Bits;
  UINT32 Data;

} SNC_UPPER_BASE_IIO_VTD_STRUCT;

/* ServerSiliconPkg/Include/Registers/Icx/CHABC_SAD.h:2304 */
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32 rule_enable : 1;

                            /* Bits[0:0], Access Type=RW, default=0x00000000*/

                            /* Enable for this DRAM rule. */
    UINT32 interleave_mode : 2;

                            /* Bits[2:1], Access Type=RW, default=0x00000000*/

                            /*
                               DRAM rule interleave mode. If a dram_rule hits a
                               3 bit number is used to index into the
                               corresponding interleave_list to determain which
                               package the DRAM belongs to. This mode selects
                               how that number is computed. 00: Address bits
                               {8,7,6}. 01: Address bits {10,9,8}. 10: Address
                               bits {14,13,12}. 11: Address bits {15,14,13}.
                            */
    UINT32 attr : 2;

                            /* Bits[4:3], Access Type=RW, default=0x00000000*/

                            /*
                               (p)Sets the memory type for the remote DRAM rule
                               according to the following encoding.(/p) (p)00 -
                               Coherent DRAM(/p) (p)01 - Memory Mapped CFG(/p)
                               (p)10 - Low Bandwidth Coherent DRAM(/p) (p)11 -
                               High Bandwidth Coherent DRAM(/p)
                            */
    UINT32 nm_cacheable : 1;

                            /* Bits[5:5], Access Type=RW, default=0x00000000*/

                            /*
                               Specifies whether or not this address range is
                               cacheable in near memory.
                            */
    UINT32 limit : 26;

                            /* Bits[31:6], Access Type=RW, default=0x00000000*/

                            /*
                               This correspond to Addr[51:26] of the DRAM rule
                               top limit address. Must be strickly greater then
                               previous rule, even if this rule is disabled,
                               unless this rule and all following rules are
                               disabled. Lower limit is the previous rule (or 0
                               if this is the first rule)
                            */

  } Bits;
  UINT32 Data;

} DRAM_RULE_CFG_0_CHABC_SAD_STRUCT;

/* ServerSiliconPkg/Include/Registers/Icx/CHABC_SAD1.h:5731 */
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT16 block_mc_ch_en : 16;

                            /* Bits[15:0], Access Type=RW, default=0x00000000*/

                            /*
                               16 bit vector indicating which of the 16
                               possible channels (4 CH x 4 MC) are enabled for
                               block mode. PA[15:12] is the index into the
                               vector - PA[13:12] selects the MC and PA[15:14]
                               is the channel. This results in the following
                               encoding: Bit[0] MC0,CH0 Bit[1] MC1,CH0 Bit[2]
                               MC2,CH0 Bit[3] MC3,CH0 Bit[4] MC0,CH1 Bit[5]
                               MC1,CH1 Bit[6] MC2,CH1 etc. This vector is
                               replicated once per socket. If SAD decodes to a
                               channel that is not enabled, SAD will abort in
                               the requesting socket.
                            */

  } Bits;
  UINT16 Data;

} BLOCK_DECODER_EN_CFG_0_CHABC_SAD1_STRUCT;

/* ServerSiliconPkg/Mem/Library/MemMcIpLib/Include/Registers/Icx/MC_2LM.h:367 */
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32 nm_target_ways : 3;

                            /* Bits[2:0], Access Type=RW, default=0x00000000*/

                            /*
                               NmTargetWays - Target interleave wayness: 000 -
                               1 way no socket interleaving 001 - 2 way
                               interleave 010 - 4 way interleave 011 - 8 way
                               interleave 100 - 16 way interleave This field is
                               programmed on the per iMC basis, i.e. need to
                               maintain consistency with other populated
                               channels in this iMC.
                            */
    UINT32 nm_chn_ways : 3;

                            /* Bits[5:3], Access Type=RW, default=0x00000000*/

                            /*
                               NmChnWays - Channel interleave wayness. This
                               field is programmed on the per channel basis,
                               i.e. need to maintain consistency with other
                               populated channels in this iMC. 000 = 1 way or 3
                               way 001 = 2 way 011 = 8 way (for IPM) Other =
                               Reserved
                            */
    UINT32 nm_target_gran : 4;

                            /* Bits[9:6], Access Type=RW, default=0x00000002*/

                            /*
                               NmTargetGran - Target interleave granularity.
                               Default value is 4k b000 64B (based off PA[6]
                               and up) (used for DDR4 legacy 1LM, DDR4 FM and
                               HBM) b001 256B (based off PA[8] and up) (used
                               for DDR4 legacy 1LM, DDR4 FM and HBM) b010 4KB
                               (based off PA[12] and up) (used for DDR4 2LM,
                               PMem, and Block/DDRT_CSR) b011 Reserved b100
                               128B (based off PA[7] and up) (used for DDR4 FM
                               and HBM) b101 512B (based off PA[9] and up)
                               (used for DDR4 FM and HBM) b110 1KB (based off
                               PA[10] and up) (used for DDR4 FM and HBM) b111
                               2KB (based off PA[11] and up) (used for DDR4 FM
                               and HBM)
                            */
    UINT32 nm_chn_gran : 4;

                            /* Bits[13:10], Access Type=RW, default=0x00000001*/

                            /*
                               NMChnGran - Channel interleave granularity.
                               Default value is 256B b000 64B (based off PA[6]
                               and up) (used for DDR4 legacy 1LM and HBM) b001
                               256B (based off PA[8] and up) (used for DDR4
                               1LM, DDR4 2LM, HBM, DDRT Block, and DDRT 2LM.
                               the latter only in mirror mode) b010 4KB (based
                               off PA[12] and up) (used for DDR4 FM, DDRT 2LM
                               and DDRT PMem) b011 Reserved b100 128B (based
                               off PA[7] and up) (used for HBM) b101 512B
                               (based off PA[9] and up) (used for HBM) b110 1KB
                               (based off PA[10] and up) (used for HBM) b111
                               2KB (based off PA[11] and up) (used for HBM)
                               Note: using the same ch_granularity encoding for
                               both DDRT and DDR4 even though 64B only applies
                               for DDR4 and 4KB only applies for DDRT.
                            */
    UINT32 nm_target_lid : 4;

                            /* Bits[17:14], Access Type=RW, default=0x00000000*/

                            /*
                               NMTargetLID - Target logical ID. The value of
                               bits socket bits for this MC. Used for reverse
                               address decode. This field is programmed on the
                               per iMC basis, i.e. need to maintain consistency
                               with other populated channels in this iMC.
                            */
    UINT32 nm_chn_lid : 3;

                            /* Bits[20:18], Access Type=RW, default=0x00000000*/

                            /*
                               NmChnLID: Channel logical ID. The value of
                               channel bits for this channel. Used for reverse
                               address decode. This field is programmed on the
                               per channel basis.
                            */
    UINT32 rsvd : 11;

                            /* Bits[31:21], Access Type=RO, default=None*/

                            /* Reserved */

  } Bits;
  UINT32 Data;

} MCNMCACHINGINTLV_MC_2LM_STRUCT;

typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32 mcnmcachingoffset : 17;

                            /* Bits[16:0], Access Type=RW, default=0x00000000*/

                            /*
                               NmCachingOffset - NM offset, applied to
                               SA[51:35]. Granularity of 32GB.
                            */
    UINT32 rsvd : 14;

                            /* Bits[30:17], Access Type=RO, default=None*/

                            /* Reserved */
    UINT32 mcnmcachingoffseten : 1;

                            /* Bits[31:31], Access Type=RW, default=0x00000000*/

                            /*
                               NmCachingOffsetEn - Enable bit for NM offset.
                               When this bit is 0, NmCachingOffset should also
                               be 0. When this bit is set to 1, make sure
                               amap.force_lat is set to 1 also.
                            */

  } Bits;
  UINT32 Data;

} MCNMCACHINGOFFSET_MC_2LM_STRUCT;

typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32 chn_cap : 4;

                            /* Bits[3:0], Access Type=RW, default=0x00000000*/

                            /*
                               ChnCap - Per Channel Memory Capacity. 4h0 - 4GB
                               (DDR4 only) 4h1 - 8GB (DDR4 only) 4h2 - 16GB
                               (DDR4 only) 4h3 - 32GB (DDR4 only) 4h4 - 64GB
                               (DDR4 only) 4h5 - 128GB (DDR4 only) 4h6 - 256GB
                               (DDR4 only) 4h7 - Reserved 4h8 - 256MB (HBM
                               only) 4h9 - 512MB (HBM only) 4hA - 1GB (HBM
                               only) others - Reserved This field can be
                               programmed on the per-channel basis.
                            */
    UINT32 nm_ratio_chn_cap : 6;

                            /* Bits[9:4], Access Type=RW, default=0x00000000*/

                            /*
                               NmRatioChnCap = portion of ChnCap used for 2LM
                               NM, in granularity of 1/32. 0 (default) = non-
                               power-of-2 ChnCap is disabled, or all channel
                               capacity is used for 2LM HBM mode only
                               (combination with McNMCachingNmChnCap should
                               result in at least 32MB): 1 = 1/32 of ChnCap
                               allocated to 2LM NM 2 = 1/16 of ChnCap allocated
                               to 2LM NM 3 = 1/8 of ChnCap allocated to 2LM NM
                               4 = 1/4 of ChnCap allocated to 2LM NM 5 = 1/2 of
                               ChnCap allocated to 2LM NM 6-31 = Reserved DDR
                               mode only (exception: value of 62 (30/32 of
                               ChnCap) can be used for HBM bank sparing mode
                               with the full ChnCap as NM): 32 = non-power-of-2
                               ChnCap is disabled, or all channel capacity is
                               used for 2LM 33-63 = 1/32 to 31/32 of ChnCap
                               allocated to 2LM NM (only value >= 49 is valid)
                            */
    UINT32 rsvd : 22;

                            /* Bits[31:10], Access Type=RO, default=None*/

                            /* Reserved */

  } Bits;
  UINT32 Data;

} MCNMCACHINGCFG_MC_2LM_STRUCT;

/* ServerSiliconPkg/Mem/Library/MemMcIpLib/Include/Registers/Icx/MC_MAIN.h:2448 */
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32 rsvd : 1;

                            /* Bits[0:0], Access Type=RO, default=None*/

                            /* Reserved */
    UINT32 rir_limit : 11;

                            /* Bits[11:1], Access Type=RW, default=0x00000000*/

                            /*
                               RIR[4:0].LIMIT[39:29] == highest address of the
                               range in channel address space, 384GB in lock-
                               step/192GB in independent channel, 512MB
                               granularity.
                            */
    UINT32 rsvd_12 : 16;

                            /* Bits[27:12], Access Type=RO, default=None*/

                            /* Reserved */
    UINT32 rir_way : 2;

                            /* Bits[29:28], Access Type=RW, default=0x00000000*/

                            /*
                               rank interleave wayness00 = 1 way, 01 = 2 way,
                               10 = 4 way, 11 = 8 way.
                            */
    UINT32 rsvd_30 : 1;

                            /* Bits[30:30], Access Type=RO, default=None*/

                            /* Reserved */
    UINT32 rir_val : 1;

                            /* Bits[31:31], Access Type=RW, default=0x00000000*/

                            /* Range Valid when set; otherwise, invalid */

  } Bits;
  UINT32 Data;

} RIRWAYNESSLIMIT_0_MC_MAIN_STRUCT;

typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32 rsvd : 2;

                            /* Bits[1:0], Access Type=RO, default=None*/

                            /* Reserved */
    UINT32 rir_offset0 : 14;

                            /* Bits[15:2], Access Type=RW, default=0x00000000*/

                            /*
                               RIR[5:0].RANKOFFSET0[39:26] == rank interleave 0
                               offset, 64MB granularity
                            */
    UINT32 rir_rnk_tgt0 : 4;

                            /* Bits[19:16], Access Type=RW/V, default=0x00000000*/

                            /*
                               target rank ID for rank interleave 0 (used for
                               1/2/4/8-way RIR interleaving).
                            */
    UINT32 rsvd_20 : 12;

                            /* Bits[31:20], Access Type=RO, default=None*/

                            /* Reserved */

  } Bits;
  UINT32 Data;

} RIRILV0OFFSET_0_MC_MAIN_STRUCT;

/* ServerSiliconPkg/Mem/Library/MemMcIpLib/Include/Registers/Icx/MC_2LM.h:6574 */
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32 rsvd : 1;

                            /* Bits[0:0], Access Type=RO, default=None*/

                            /* Reserved */
    UINT32 rir_limit : 13;

                            /* Bits[13:1], Access Type=RW, default=0x00000000*/

                            /*
                               RIR[4:0].LIMIT[41:29] == highest address of the
                               range in channel address space. Needs to address
                               up to 4TB at a 512MB granularity.
                            */
    UINT32 rsvd_14 : 14;

                            /* Bits[27:14], Access Type=RO, default=None*/

                            /* Reserved */
    UINT32 rir_way : 2;

                            /* Bits[29:28], Access Type=RW, default=0x00000000*/

                            /*
                               rank interleave wayness00 = 1 way, 01 = 2 way,
                               10 = 4 way, 11 = 8 way.
                            */
    UINT32 rsvd_30 : 1;

                            /* Bits[30:30], Access Type=RO, default=None*/

                            /* Reserved */
    UINT32 rir_val : 1;

                            /* Bits[31:31], Access Type=RW, default=0x00000000*/

                            /* Range Valid when set; otherwise, invalid */

  } Bits;
  UINT32 Data;

} FMRIRWAYNESSLIMIT_0_MC_2LM_STRUCT;

typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32 rsvd : 2;

                            /* Bits[1:0], Access Type=RO, default=None*/

                            /* Reserved */
    UINT32 rir_offset0 : 16;

                            /* Bits[17:2], Access Type=RW, default=0x00000000*/

                            /*
                               RIR[].RANKOFFSET0[41:26] == rank interleave 0
                               offset
                            */
    UINT32 rir_rnk_tgt0 : 4;

                            /* Bits[21:18], Access Type=RW/V, default=0x00000000*/

                            /*
                               target rank ID for rank interleave 0 (used for
                               1/2-way RIR interleaving).
                            */
    UINT32 rsvd_22 : 10;

                            /* Bits[31:22], Access Type=RO, default=None*/

                            /* Reserved */

  } Bits;
  UINT32 Data;

} FMRIRILV0OFFSET_0_MC_2LM_STRUCT;

/* ServerSiliconPkg/Mem/Library/MemMcIpLib/Include/Registers/Icx/MC_MAIN.h:247 */
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32 ca_width : 2;

                            /* Bits[1:0], Access Type=RW/P, default=None*/

                            /*
                               Fix at 00. DDR4 is always 10 bits and HBM is
                               always 6. 00 - 10 bits for DDR4 and 6 bits for
                               HBM Others - reserved
                            */
    UINT32 ra_width : 3;

                            /* Bits[4:2], Access Type=RW/P, default=None*/

                            /*
                               000 - reserved 001 - 13 bits 010 - 14 bits 011 -
                               15 bits 100 - 16 bits 101 - 17 bits 110 - 18
                               bits 111: reserved
                            */
    UINT32 ddr3_dnsty : 3;

                            /* Bits[7:5], Access Type=RW/P, default=None*/

                            /*
                               000 - Reserved 001 - 2Gb 010 - 4Gb 011 - 8Gb 100
                               - 16Gb 101 - 12Gb THis field applies to ddr4.
                            */
    UINT32 ddr3_width : 2;

                            /* Bits[9:8], Access Type=RW/P, default=None*/

                            /*
                               00 - x4 01 - x8 10 - x16 11 - reserved This
                               field applies to ddr4.
                            */
    UINT32 ba_shrink : 1;

                            /* Bits[10:10], Access Type=RW/P, default=None*/

                            /*
                               0=normal,1=remove a device dependent bit
                               (HBM:BA1, DDR4:BG1, DDR4E:?) Used for x16
                               devices in DDR4.
                            */
    UINT32 rsvd : 1;

                            /* Bits[11:11], Access Type=RO, default=None*/

                            /* Reserved */
    UINT32 rank_cnt : 2;

                            /* Bits[13:12], Access Type=RW/P, default=None*/

                            /* 00 - SR 01 - DR 10 - QR 11 - reserved */
    UINT32 rsvd_14 : 1;

                            /* Bits[14:14], Access Type=RO, default=None*/

                            /* Reserved */
    UINT32 dimm_pop : 1;

                            /* Bits[15:15], Access Type=RW/P, default=None*/

                            /*
                               DDR4 DIMM populated if set; otherwise,
                               unpopulated. Should be set to 0 when this slot
                               is populated with a DDR-T DIMM.
                            */
    UINT32 rank_disable : 4;

                            /* Bits[19:16], Access Type=RW/P, default=None*/

                            /*
                               RANK Disable Control to disable refresh and
                               ZQCAL operation. This bit setting must be set
                               consistently with TERM_RNK_MSK, i.e. both
                               corresponding bits cannot be set at the same
                               time. In the other word, a disabled rank must
                               not be selected for the terminaton rank.
                               RANK_DISABLE[3], i.e. bit 19: rank 3 disable.
                               Note DIMMMTR_2.RANK_DISABLE[3] is dont care
                               since DIMM 2 must not be quad-rank
                               RANK_DISABLE[2], i.e. bit 18: rank 2 disable.
                               Note DIMMMTR_2.RANK_DISABLE[2] is dont care
                               since DIMM 2 must not be quad-rank
                               RANK_DISABLE[1], i.e. bit 17: rank 1 disable
                               RANK_DISABLE[0], i.e. bit 16: rank 0 disable
                               when set, no refresh will be perform on this
                               rank. ODT termination is not affected by this
                               bit. Note that patrols are disabled by
                               dimm*_pat_rank_disable of amap register now and
                               not affected by this bit field.
                            */
    UINT32 rsvd_20 : 1;

                            /* Bits[20:20], Access Type=RO, default=None*/

                            /* Reserved */
    UINT32 hdrl : 1;

                            /* Bits[21:21], Access Type=RW/P, default=None*/

                            /* Reserved - unused. */
    UINT32 hdrl_parity : 1;

                            /* Bits[22:22], Access Type=RW/P, default=None*/

                            /* Reserved - unused. */
    UINT32 ddr4_3dsnumranks_cs : 2;

                            /* Bits[24:23], Access Type=RW/P, default=None*/

                            /*
                               Number of sub ranks per chip select per dimm of
                               DDR4 3DS and non3ds_lrdimm (this is a dimm
                               specific field. There are restriction on
                               rank_cnt field if this field is nonzero fro 3ds
                               devices. Can not mix 2 chip_select parts and 1
                               chip_select part on the same channel) 00-
                               3ds/non3ds_lrdimm ddr4 multiple ranks/chip
                               select disabled 01- 2 ranks per chip select 10-
                               4 ranks per chip select 11- 8 ranks per chip
                               select
                            */
    UINT32 rsvd_25 : 7;

                            /* Bits[31:25], Access Type=RO, default=None*/

                            /* Reserved */

  } Bits;
  UINT32 Data;

} DIMMMTR_0_MC_MAIN_STRUCT;

/* ServerSiliconPkg/Mem/Library/MemMcIpLib/Include/Registers/Icx/MC_MAIN.h:544 */
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32 cgbg_interleave : 2;

                            /* Bits[1:0], Access Type=RW, default=0x00000000*/

                            /*
                               coarse grain bank group interleaving mode.
                               0=CGBG (Coarse grained bank group interleave)
                               1=FGBG_INT_2 (Fine grained bank group interleave
                               of one bank bit) 2=XOR_HASH (HBM) or FGBG_INT_4
                               (DDR4) or FGBG_INT_8 (DDR4E) (Fine grained bank
                               group interleave on two bank bits)
                            */
    UINT32 device_type : 1;

                            /* Bits[2:2], Access Type=RW, default=0x00000000*/

                            /*
                               Device type. 0=DDR4 1=DDR4E This field is a
                               don't care in HBM mode.
                            */
    UINT32 force_lat : 1;

                            /* Bits[3:3], Access Type=RW, default=0x00000000*/

                            /*
                               When setting to 1, force additional 1 cycle
                               latency in decoding logic (same latency as 3
                               channel mode); expected to set to 1 when channel
                               mirroring is enabled. Also expect to set to 1
                               when NmCachingOffsetEn is set to 1.
                            */
    UINT32 mirr_adddc_en : 1;

                            /* Bits[4:4], Access Type=RW, default=0x00000000*/

                            /*
                               Enabling special spare copy mode for ADDDC and
                               mirroring enable. In this mode, system addresses
                               will be gone through 3 times, with first pass
                               for non-mirror addresses. second pass for mirror
                               primary addresses, and third pass for mirror
                               secondary addresses. This function will only
                               work if sparing_control.mirr_adddc_en is set to
                               1
                            */
    UINT32 wait4bothhalves : 1;

                            /* Bits[5:5], Access Type=RW, default=0x00000000*/

                            /* Set to 1 to not allow critical chunk support. */
    UINT32 dev_sub_dly_en : 1;

                            /* Bits[6:6], Access Type=RW, default=0x00000000*/

                            /*
                               Set to 1 to enable datapath delay for device
                               substitution.
                            */
    UINT32 adddc_plus1_dly_en : 1;

                            /* Bits[7:7], Access Type=RW, default=0x00000000*/

                            /* Set to 1 to enable datapath delay for ADDDC+1. */
    UINT32 force_no_lat : 1;

                            /* Bits[8:8], Access Type=RW, default=0x00000000*/

                            /*
                               When setting to 1, disable additional 1 cycle
                               latency in decoding logic (3 channel mode);
                               expected to set to 1 when only TADs that have 3
                               channels are in 2LM mode only.
                            */
    UINT32 spr_sys_cmp : 1;

                            /* Bits[9:9], Access Type=RW, default=0x00000000*/

                            /*
                               When set to 1, Select system address instead of
                               channel address for determining if an incoming
                               M2M request has been spare copied or not during
                               rank or sddc sparing flow. Should be set to 1
                               when mirroring is enabled. Should be kept at 0
                               in 2LM mode.
                            */
    UINT32 rsvd : 5;

                            /* Bits[14:10], Access Type=RO, default=None*/

                            /* Reserved */
    UINT32 dimm0_pat_rank_disable : 4;

                            /* Bits[18:15], Access Type=RW, default=0x00000000*/

                            /*
                               This field is similar to dimmmtr_*.rank_disable;
                               but it affects only patrol operations. Each bit
                               controls 1 Chip_selects patrol in a given dimm.
                               If 1, patrol engine will skip that ChipSelect;
                               other wise it will patrol it if dimm is populted
                               and all other patrol conditions are true. This
                               is intended to be used after rank_sparing to
                               skip patrol of the spared rank.
                            */
    UINT32 dimm1_pat_rank_disable : 4;

                            /* Bits[22:19], Access Type=RW, default=0x00000000*/

                            /*
                               This field is similar to dimmmtr_*.rank_disable;
                               but it affects only patrol operations. Each bit
                               controls 1 Chip_selects patrol in a given dimm.
                               If 1, patrol engine will skip that ChipSelect;
                               other wise it will patrol it if dimm is populted
                               and all other patrol conditions are true. This
                               is intended to be used after rank_sparing to
                               skip patrol of the spared rank.
                            */
    UINT32 rsvd_23 : 9;

                            /* Bits[31:23], Access Type=RO, default=None*/

                            /* Reserved */

  } Bits;
  UINT32 Data;

} AMAP_MC_MAIN_STRUCT;

/* ServerSiliconPkg/Mem/Library/MemMcIpLib/Include/Registers/Icx/MC_MAIN.h:5714 */
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32 slot0 : 1;

                            /* Bits[0:0], Access Type=RW, default=0x00000000*/

                            /*
                               DDR-T DIMM populated if set; otherwise,
                               unpopulated. Should be set to 0 when this slot
                               is populated with a DDR4 DIMM.
                            */
    UINT32 slot1 : 1;

                            /* Bits[1:1], Access Type=RW, default=0x00000000*/

                            /*
                               DDR-T DIMM populated if set; otherwise,
                               unpopulated. Should be set to 0 when this slot
                               is populated with a DDR4 DIMM.
                            */
    UINT32 rsvd : 30;

                            /* Bits[31:2], Access Type=RO, default=None*/

                            /* Reserved */

  } Bits;
  UINT32 Data;

} MCDDRTCFG_MC_MAIN_STRUCT;

/* ServerSiliconPkg/Include/Registers/Icx/M2MEM_MAIN.h:13709 */
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32 rsvd : 3;

                            /* Bits[2:0], Access Type=RO, default=None*/

                            /* Reserved */
    UINT32 enable : 1;

                            /* Bits[3:3], Access Type=RW, default=0x00000000*/

                            /* enable bit for the M2M_PRMRR_BASE register */
    UINT32 rsvd_4 : 8;

                            /* Bits[11:4], Access Type=RO, default=None*/

                            /* Reserved */
    UINT32 base_address : 20;

                            /* Bits[31:12], Access Type=RW, default=0x00000000*/

                            /*
                               PRMRR specifies the secure encrypted region to
                               be handled by the MEE (encryption) engine. This
                               field specifies the system address[51:12] lower
                               limit of the PRMRR region. Bits [11:0] are
                               always zero. A system address needs to be larger
                               than or equal to this limit in order to be able
                               to belong to the PRMRR region.
                            */

  } Bits;
  UINT32 Data;

} M2M_PRMRR_BASE_N0_M2MEM_MAIN_STRUCT;

typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32 base_address : 20;

                            /* Bits[19:0], Access Type=RW, default=0x00000000*/

                            /*
                               PRMRR specifies the secure encrypted region to
                               be handled by the MEE (encryption) engine. This
                               field specifies the system address[51:12] lower
                               limit of the PRMRR region. Bits [11:0] are
                               always zero. A system address needs to be larger
                               than or equal to this limit in order to be able
                               to belong to the PRMRR region.
                            */
    UINT32 rsvd : 12;

                            /* Bits[31:20], Access Type=RO, default=None*/

                            /* Reserved */

  } Bits;
  UINT32 Data;

} M2M_PRMRR_BASE_N1_M2MEM_MAIN_STRUCT;

typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32 rsvd : 10;

                            /* Bits[9:0], Access Type=RO, default=None*/

                            /* Reserved */
    UINT32 lock : 1;

                            /* Bits[10:10], Access Type=RW, default=0x00000000*/

                            /*
                               matching on the PRMRR range is only allowed if
                               the lock bit is 1. If the lock bit is zero the
                               M2M_PRMRR_BASE range check should always fail to
                               produce a match.
                            */
    UINT32 valid : 1;

                            /* Bits[11:11], Access Type=RW, default=0x00000000*/

                            /*
                               this bit does nothing in the M2MEM. it might
                               only be used by the core copy of the
                               M2M_PRMRR_MASK register. M2MEM has a copy though
                               because the ucode in the core might use the
                               M2MEM copy of this register to save/restore the
                               core copy of the same register.
                            */
    UINT32 range_mask : 20;

                            /* Bits[31:12], Access Type=RW, default=0x00000000*/

                            /*
                               sets address bits [51:12] of the range mask for
                               all of the PRMRR range registers. The size
                               alignment requirement on the PRMRR base
                               addresses enables all PRMRR range registers to
                               use a single range mask.
                            */

  } Bits;
  UINT32 Data;

} M2M_PRMRR_MASK_N0_M2MEM_MAIN_STRUCT;

typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32 range_mask : 20;

                            /* Bits[19:0], Access Type=RW, default=0x00000000*/

                            /*
                               sets address bits [51:12] of the range mask for
                               all of the PRMRR range registers. The size
                               alignment requirement on the PRMRR base
                               addresses enables all PRMRR range registers to
                               use a single range mask.
                            */
    UINT32 rsvd : 12;

                            /* Bits[31:20], Access Type=RO, default=None*/

                            /* Reserved */

  } Bits;
  UINT32 Data;

} M2M_PRMRR_MASK_N1_M2MEM_MAIN_STRUCT;

typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32 close_pg : 1;

                            /* Bits[0:0], Access Type=RW/P, default=None*/

                            /*
                               Use close page address mapping if set;
                               otherwise, open page.
                            */
    UINT32 rsvd : 1;

                            /* Bits[1:1], Access Type=RO, default=None*/

                            /* Reserved */
    UINT32 ecc_en : 1;

                            /* Bits[2:2], Access Type=RW/V/P, default=None*/

                            /* ECC enable. */
    UINT32 dir_en : 1;

                            /* Bits[3:3], Access Type=RW/V/P, default=None*/

                            /*
                               Directory Enable. Read-Only (RO) with 0 value if
                               not supported. This bit is not used by the
                               design. M2M controls directory enable behavior.
                            */
    UINT32 read_dbi_en : 1;

                            /* Bits[4:4], Access Type=RW, default=0x00000001*/

                            /*
                               HBM Only. Enable DBI for memory reads. This bit
                               is not used in DDR mode.
                            */
    UINT32 write_dbi_en : 1;

                            /* Bits[5:5], Access Type=RW, default=0x00000000*/

                            /*
                               HBM Only. Enable DBI for memory writes. This bit
                               is not used in DDR mode.
                            */
    UINT32 read_parity_en : 1;

                            /* Bits[6:6], Access Type=RW, default=0x00000000*/

                            /*
                               HBM Only. Enable parity for memory reads. This
                               bit is not used in DDR mode.
                            */
    UINT32 write_parity_en : 1;

                            /* Bits[7:7], Access Type=RW, default=0x00000000*/

                            /*
                               HBM Only. Enable parity for memory writes. This
                               bit is not used in DDR mode.
                            */
    UINT32 normal : 1;

                            /* Bits[8:8], Access Type=RW, default=0x00000000*/

                            /* 0: Training mode 1: Normal Mode */
    UINT32 bank_xor_enable : 1;

                            /* Bits[9:9], Access Type=RW/P, default=None*/

                            /*
                               When set, this bit will enable bank XORing. This
                               is targeted at workloads that bank thrashing
                               caused by certain stride or page mappings. If
                               one detects unexpectedly poor page hit rates,
                               one can attempt to flip this bit to see if it
                               helps. [0]: Our base configuration. Bank
                               selection is done using rank address bits
                               12:17:18 for open page mapping and bits 6:7:8
                               for close page mapping. [1]: Bank XORing
                               enabled. Bank selection is done using rank
                               address bits: (1219):(1720):(1821) for open page
                               mapping (619):(720):(821) for close page mapping
                            */
    UINT32 trng_mode : 2;

                            /* Bits[11:10], Access Type=RW, default=0x00000000*/

                            /*
                               00: reserved 01: Native CPGC Mode. (mcmtr.normal
                               must be zero for this mode) 10: reserved 11:
                               Normal Mode (mcmtr.normal is a dont care for
                               this mode)
                            */
    UINT32 rsvd_12 : 6;

                            /* Bits[17:12], Access Type=RO, default=None*/

                            /* Reserved */
    UINT32 chn_disable : 1;

                            /* Bits[18:18], Access Type=RW/P, default=None*/

                            /*
                               Channel disable control. When set, the channel
                               is disabled. Note: Message Channel may not work
                               if all channels are set to disable in this
                               field.
                            */
    UINT32 xor_mode_enable : 1;

                            /* Bits[19:19], Access Type=RW, default=0x00000000*/

                            /*
                               HBM mode XOR hash enable for up to 4 clusters
                               and 8 channels.
                            */
    UINT32 ddr_xor_mode_enable : 1;

                            /* Bits[20:20], Access Type=RW, default=0x00000000*/

                            /* DDR mode XOR hash enable for channels. */
    UINT32 ddr_half_xor_mode_enable : 1;

                            /* Bits[21:21], Access Type=RW, default=0x00000000*/

                            /* DDR mode XOR hash enable for 2 clusters. */
    UINT32 ch23cmd_ctl_delay : 1;

                            /* Bits[22:22], Access Type=RW/P, default=None*/

                            /* unused on 10nm */
    UINT32 rsvd_23 : 2;

                            /* Bits[24:23], Access Type=RO, default=None*/

                            /* Reserved */
    UINT32 trng_target : 1;

                            /* Bits[25:25], Access Type=RW, default=0x00000000*/

                            /*
                               Per channel based target. 0 = CPGC transactions
                               go to DRAM scheduler. 1 = CPGC transactions go
                               to DDRT scheduler.
                            */
    UINT32 rsvd_26 : 3;

                            /* Bits[28:26], Access Type=RO, default=None*/

                            /* Reserved */
    UINT32 hbm_mc_mode : 1;

                            /* Bits[29:29], Access Type=RO/V, default=None*/

                            /*
                               This bit specifies which kind of MC this is, HBM
                               or DDR. When 1, the MC is an HBM MC. When 0, it
                               is a DDR MC (this includes both DDR4 and DDRT).
                            */
    UINT32 rsvd_30 : 2;

                            /* Bits[31:30], Access Type=RO, default=None*/

                            /* Reserved */

  } Bits;
  UINT32 Data;

} MCMTR_MC_MAIN_STRUCT;


/***************************************************/
/*
struct usra_address {
	USRA_ADDR_TYPE AddrType;     // Address type: CSR, PCIE(0), MMIO(3), IO, SMBus
    UINT32  Offset;       // This Offset  occupies 32 bits. It's platform code's responsibilty to define the meaning of specific
    // bits and use them angly.
    UINT32  InstId:8;     // The Box Instance, 0 based, Index/Port within the box, Set Index as 0 if the box has only one instances
    UINT32  SocketId:8;   // The Socket Id
    UINT32  InstType:8;   // The Instance Type
};
*/

/*--ServerSiliconPkg/Include/Library/AddressDecodeLib.h--*/
typedef struct _TRANSLATED_ADDRESS {
    UINT8                SocketId;
    UINT8                MemoryControllerId;
    UINT8                ChannelId;
    UINT8                DimmSlot;
    UINT32               Row;
    UINT32               Col;
    UINT8                Bank;
    UINT8                BankGroup;

    UINT64               DPA;
    UINT64               RankAddress; //internal



  UINT64               Flag;
  UINT64               SystemAddress;
  UINT64               NmSystemAddress;
  UINT64               SpareSystemAddress;

  UINT8                SadId; //internal
  UINT8                TadId; //internal

  UINT8                NmMemoryControllerId;
  UINT8                TargetId; //internal
  UINT8                LogChannelId; //internal
  UINT8                TargetWays; //internal
  UINT8                ChannelWays; //internal
  UINT64               ChannelAddress; //internal
  UINT64               SecChannelAddress; //internal
  UINT64               NmChannelAddress; //internal

  UINT8                SecChannelId;
  UINT8                NmChannelId;

  UINT64               SecRankAddress; //internal
  UINT64               NmRankAddress; //internal
  UINT8                PhysicalRankId; //rank on the DIMM  - external
  UINT8                SecPhysicalRankId; //rank on the DIMM  - external
  UINT8                NmPhysicalRankId;

  UINT8                SecDimmSlot;
  UINT8                NmDimmSlot;
  UINT8                DimmRank; // internal

  UINT32               SecRow;
  UINT32               NmRow;

  UINT32               SecCol;
  UINT32               NmCol;

  UINT8                SecBank;
  UINT8                NmBank;

  UINT8                SecBankGroup;
  UINT8                NmBankGroup;
  UINT8                LockStepRank;
  UINT8                LockStepPhysicalRank;
  UINT8                LockStepBank;
  UINT8                LockStepBG;
  UINT8                ChipSelect; //rank on the chn - external
  UINT8                NmChipSelect;
  UINT8                SecChipSelect;
  UINT8                Node; //internal
  UINT8                ChipId; //sub-rank
  UINT8                NmChipId;
  UINT8                SecChipId;
  UINT8                RasModesApplied;  //BIT0 = Rank sparing, BIT1 = RANK VLS, BIT2 = BANK VLS BIT3 = MIRROR
  MEM_TYPE             MemType; //need to add persistent Memmory/block decode/
  TRANSLATED_DIMM_TYPE DimmType; //for FM
  UINT32               StatusToDsmInterface;
} TRANSLATED_ADDRESS, *PTRANSLATED_ADDRESS;

EFI_STATUS
EFIAPI
InitAddressDecodeLib (
  char nrCPU
  );

UINT64
EFIAPI
XorModeAddressCalc (
  IN  UINT64      PhyAddr,
  IN  UINT32      ClusterXorModeEn,
  IN  UINT32      ChXorModeEn,
  IN  UINT8       TargetWays,
  IN  UINT8       ChannelWays
  );

EFI_STATUS
EFIAPI
SystemAddressToDimmAddress (
  IN      dimmBDFst *pdimmBdp,
  IN      UINTN               SystemAddress,
  OUT     PTRANSLATED_ADDRESS TranslatedAddress
  );

EFI_STATUS
EFIAPI
DimmAddressToSystemAddress (
  IN      dimmBDFst *pdimmBdp,
  IN OUT  PTRANSLATED_ADDRESS TranslatedAddress
  );

BOOLEAN
EFIAPI
CheckSgxMemory (
  IN  dimmBDFst *pdimmBdp,
  UINT64      PhyAddr,
  UINT8       SktId,
  UINT8       McId
);

VOID
EFIAPI
GetMcmtr (
  IN  dimmBDFst *pdimmBdp,
  IN  UINT8   Skt,
  IN  UINT8   Mc,
  IN  UINT8   ChOnSkt,
  OUT  UINT32   *ClosePg,              OPTIONAL
  OUT  UINT32   *BankXorEn,            OPTIONAL
  OUT  UINT32   *ChXorModeEn,          OPTIONAL
  OUT  UINT32   *ClusterXorModeEn      OPTIONAL
  );

EFI_STATUS
EFIAPI
TranslateSad (
  IN dimmBDFst *pdimmBdf,
  IN TRANSLATED_ADDRESS * const TranslatedAddress,
  OUT UINT8 *SktId,
  OUT UINT8 *McId,
  OUT UINT8 *ChId
 );

#endif /* __ADRESS_DECODE_LIB_H__ */
