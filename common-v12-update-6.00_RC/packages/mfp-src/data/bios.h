/*
 * BISO.H & BIOS.C are created based on BIOS codes: ModU64x32.c, BitField.c and so on.
 */

#ifndef __BIOS__
#define __BIOS__

#define IN
#define OUT
#define EFIAPI

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define BOOLEAN             bool
#define VOID                void

/* PECI */
#define DEVICE_OF_FIRST_IMC    26
#define MIN_CPU_ADDRESS		   48
#define NUMBER_OF_CHANNELS     2
#define MMIO_CHANNEL_OFFSET_DIFF    (UINT32) 0x4000
//#define PECI_OPTION_TEST /* OPTION test */


#define PCI_CFG_ADDR(bus,dev,fun,off) \
	(UINT32)(((bus & 0xff) << 20) | ((dev & 0x1f) << 15) | ((fun & 0x7) << 12) | (off & 0xfff))
#define ASSERT(Expression)

/* Base.h */
#define MAX_INT8    ((INT8)0x7F)
#define MAX_UINT8   ((UINT8)0xFF)
#define MAX_INT16   ((INT16)0x7FFF)
#define MAX_UINT16  ((UINT16)0xFFFF)
#define MAX_INT32   ((INT32)0x7FFFFFFF)
#define MAX_UINT32  ((UINT32)0xFFFFFFFF)
#define MAX_INT64   ((INT64)0x7FFFFFFFFFFFFFFFULL)
#define MAX_UINT64  ((UINT64)0xFFFFFFFFFFFFFFFFULL)

#define MIN_INT8    (((INT8)  -127) - 1)
#define MIN_INT16   (((INT16) -32767) - 1)
#define MIN_INT32   (((INT32) -2147483647) - 1)
#define MIN_INT64   (((INT64) -9223372036854775807LL) - 1)

/*
#define  BIT0     0x00000001
#define  BIT1     0x00000002
#define  BIT2     0x00000004
#define  BIT3     0x00000008
#define  BIT4     0x00000010
#define  BIT5     0x00000020
#define  BIT6     0x00000040
#define  BIT7     0x00000080
#define  BIT8     0x00000100
#define  BIT9     0x00000200
#define  BIT10    0x00000400
#define  BIT11    0x00000800
#define  BIT12    0x00001000
#define  BIT13    0x00002000
#define  BIT14    0x00004000
#define  BIT15    0x00008000
*/
#define  BIT16    0x00010000
#define  BIT17    0x00020000
#define  BIT18    0x00040000
#define  BIT19    0x00080000
#define  BIT20    0x00100000
#define  BIT21    0x00200000
#define  BIT22    0x00400000
#define  BIT23    0x00800000
#define  BIT24    0x01000000
#define  BIT25    0x02000000
#define  BIT26    0x04000000
#define  BIT27    0x08000000
#define  BIT28    0x10000000
#define  BIT29    0x20000000
#define  BIT30    0x40000000
#define  BIT31    0x80000000

/* CRB.sdl */
#define MAX_IIO_STACK       6
#define MAX_LOGIC_IIO_STACK 8

/* WhitleyCrb.mak */
// if CPU_SKX_ONLY_SUPPORT == 0,
//    MAX_IMC=4 && MAX_MC_CH =2
// else
//    MAX_IMC=2 && MAX_MC_CH =3
// endif
#define MAX_IMC             4 /* == MAX_AMOUNT_OF_IMCS */
#define MAX_MC_CH           2

/*** WhitleyCrb.sdl ***/
#define MAX_SOCKET          4 /* If PROJECT_TYPE is set to 1, then MAX_SOCKET will be set to 4. */
#define MAX_RANK_DIMM       4
#define MAX_RANK_CH         8

/* ServerSiliconPkg/Include/SiliconSetting.h */
#define NODE_TO_SKT(node)             (node / MAX_IMC)
#define NODE_TO_MC(node)              (node % MAX_IMC)
#define NODECHA_TO_SKTCHA(node, ch)   (((node % MAX_IMC) * MAX_MC_CH) + ch)
#define SKTCH_TO_NODECH(ChOnSkt)      (ChOnSkt % MAX_MC_CH)
#define SKT_CH_TO_NODE(Skt,ChOnSkt)   (Skt * MAX_IMC + ChOnSkt / MAX_MC_CH)
#define SKTMC_TO_NODE(socket, mc)     ((socket * MAX_IMC) | (mc % MAX_IMC))
#define CHRANK_TO_DIMMRANK(Rank)      (Rank % 4)
#define CHRANK_TO_DIMM(Rank)          (Rank / 4)

#define MAX_SPARE_RANK      2
#define MAX_DIMM            2
#define TAD_RULES           8
#define IMC_MAX_CH          ((MAX_IMC) * (MAX_MC_CH))
#define MC_MAX_NODE         (MAX_SOCKET * MAX_IMC)

/* ServerSiliconPkg/Include/IioRegs.h */
#define MaxIIO              MAX_SOCKET

/* CpRcPkg/Include/Upi/KtiSi.h */
#define MAX_CHA_MAP         4
#if (MAX_SOCKET == 1)
  #define MAX_FW_KTI_PORTS     3    // Maximum KTI PORTS to be used in structure definition.
#else
  #define MAX_FW_KTI_PORTS     6    // Maximum KTI PORTS to be used in structure definition (update to 8 when scratchpads are defined for GNR/SRF).
#endif //(MAX_SOCKET == 1)

/* ServerSiliconPkg/Include/IioSetupDefinitions.h:48 */
//-----------------------------------------------------------------------------------
// Number of ports per socket for CPUs
//------------------------------------------------------------------------------------
#define NUMBER_PORTS_PER_SOCKET_ICX       21
#define NUMBER_PORTS_PER_SOCKET_ICXG      13
#define NUMBER_PORTS_PER_SOCKET_ICXDE     9
#define NUMBER_PORTS_PER_SOCKET_ICXDE_LCC 5
#define NUMBER_PORTS_PER_SOCKET_SNR       5
#define NUMBER_PORTS_PER_SOCKET_SPR       41
#define NUMBER_PORTS_PER_SOCKET_SKX       21
#define NUMBER_PORTS_PER_SOCKET_CPX       13
#define NUMBER_PORTS_PER_SOCKET           NUMBER_PORTS_PER_SOCKET_ICX

/* MemCommon.h */
#define MAX_CH              ((MAX_IMC)*(MAX_MC_CH))         // Max channels per socket
typedef enum {
  TYPE_SCF_BAR = 0,
  TYPE_PCU_BAR,
  TYPE_MEM_BAR0,
  TYPE_MEM_BAR1,
  TYPE_MEM_BAR2,
  TYPE_MEM_BAR3,
  TYPE_MEM_BAR4,
  TYPE_MEM_BAR5,
  TYPE_MEM_BAR6,
  TYPE_MEM_BAR7,
  TYPE_SBREG_BAR,
  TYPE_MAX_MMIO_BAR
} MMIO_BARS;

/* typedefs */
typedef unsigned char       UINT8 ;
typedef unsigned short      UINT16;
typedef unsigned int        UINT32;
typedef unsigned long long  UINT64;
typedef char                INT8 ;
typedef short               INT16;
typedef int                 INT32;
typedef long long           INT64;

/* ProcessorBind.h */
typedef unsigned long long  UINTN;
typedef long long           INTN;

/* DataTypes.h */
typedef struct u64_struct {
  UINT32  lo;
  UINT32  hi;
} UINT64_STRUCT, *PUINT64_STRUCT;

typedef union {
  struct {
    UINT32  Low;
    UINT32  High;
  } Data32;
  UINT64 Data;
} UINT64_DATA;

/************************/
typedef enum  {
	MESSAGE_LOCAL_PCI = 3,
	MESSAGE_PCI       = 4,
	MESSAGE_MMIO      = 5
} messageType;

typedef enum  {
	PCI_BDF_REG = 4,
	BAR_32_BITS = 5,
	BAR_64_BITS = 6
} addressType;

typedef enum  {
	OPTION_BYTE   = 1,
	OPTION_WORD   = 2,
	OPTION_DWORD  = 4,
	OPTION_QWORD  = 8,
	OPTION_DQWORD = 16,
} optionType;

typedef struct {
	UINT8   cpuType;
    UINT8   socket;
    UINT8   imc;
    UINT8   channel;
    UINT8   bus;
} dimmBDFst;

typedef struct {
    messageType msgType;
    addressType addrType;
    optionType  optType;
    dimmBDFst   dimmBdf;
    UINT8   device;
    UINT8   function;
    UINT64  offset;
} endPointRegInfo;


/* BitField.h */
// BitFieldReadXX
UINT8
EFIAPI
BitFieldRead8 (
  IN      UINT8                     Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit
  );

UINT16
EFIAPI
BitFieldRead16 (
  IN      UINT16                    Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit
  );

UINT32
EFIAPI
BitFieldRead32 (
  IN      UINT32                    Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit
  );

UINT64
EFIAPI
BitFieldRead64 (
  IN      UINT64                    Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit
  );

// BitFieldWriteXX
UINT8
EFIAPI
BitFieldWrite8 (
  IN      UINT8                     Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT8                     Value
  );

UINT16
EFIAPI
BitFieldWrite16 (
  IN      UINT16                    Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT16                    Value
  );

UINT32
EFIAPI
BitFieldWrite32 (
  IN      UINT32                    Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT32                    Value
  );

UINT64
EFIAPI
BitFieldWrite64 (
  IN      UINT64                    Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT64                    Value
  );

// BitFieldAndXX
UINT8
EFIAPI
BitFieldAnd8 (
  IN      UINT8                     Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT8                     AndData
  );

UINT16
EFIAPI
BitFieldAnd16 (
  IN      UINT16                    Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT16                    AndData
  );

UINT32
EFIAPI
BitFieldAnd32 (
  IN      UINT32                    Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT32                    AndData
  );

UINT64
EFIAPI
BitFieldAnd64 (
  IN      UINT64                    Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT64                    AndData
  );

// BitFieldOrXX
UINT8
EFIAPI
BitFieldOr8 (
  IN      UINT8                     Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT8                     OrData
  );

UINT16
EFIAPI
BitFieldOr16 (
  IN      UINT16                    Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT16                    OrData
  );

UINT32
EFIAPI
BitFieldOr32 (
  IN      UINT32                    Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT32                    OrData
  );

UINT64
EFIAPI
BitFieldOr64 (
  IN      UINT64                    Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT64                    OrData
  );


// BitFieldAndThenOrXX
UINT8
EFIAPI
BitFieldAndThenOr8 (
  IN      UINT8                     Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT8                     AndData,
  IN      UINT8                     OrData
  );

UINT16
EFIAPI
BitFieldAndThenOr16 (
  IN      UINT16                    Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT16                    AndData,
  IN      UINT16                    OrData
  );

UINT32
EFIAPI
BitFieldAndThenOr32 (
  IN      UINT32                    Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT32                    AndData,
  IN      UINT32                    OrData
  );

UINT64
EFIAPI
BitFieldAndThenOr64 (
  IN      UINT64                    Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT64                    AndData,
  IN      UINT64                    OrData
  );


/*** ModU64x32.c ***/
UINT32
EFIAPI
ModU64x32 (
  IN      UINT64                    Dividend,
  IN      UINT32                    Divisor
  );

UINT64
EFIAPI
LShiftU64 (
  IN      UINT64                    Operand,
  IN      UINTN                     Count
  );

UINT64
EFIAPI
RShiftU64 (
  IN      UINT64                    Operand,
  IN      UINTN                     Count
  );

INTN
EFIAPI
HighBitSet64 (
  IN      UINT64                    Operand
  );

UINT64
EFIAPI
DivU64x32 (
  IN      UINT64                    Dividend,
  IN      UINT32                    Divisor
  );


/*** PECI ***/
UINT32 EFIAPI ReadCpuCsrMmio(
  //UINT8 SocId,
  //UINT8 BoxInst,
  endPointRegInfo *reg
  );

int EFIAPI WriteCpuCsrMmio(
  //UINT8 SocId,
  //UINT8 BoxInst,
  endPointRegInfo *reg,
  UINT32 Data
  );

UINT32 EFIAPI ReadCpuCsrPciLocal(
  endPointRegInfo *reg
  );

int EFIAPI WriteCpuCsrPciLocal(
  endPointRegInfo *reg,
  UINT32 Data
  );

#endif /* __BIOS__ */
