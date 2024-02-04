#ifndef __MFP_REGISTERS_STRUCTS_H__
#define __MFP_REGISTERS_STRUCTS_H__

#include <stdint.h>

#define	DIMM_CH_0_SLOT_0_REG	0x2080C
#define	DIMM_CH_0_SLOT_1_REG	0x20810
#define	DIMM_CH_1_SLOT_0_REG	0x2480C
#define	DIMM_CH_1_SLOT_1_REG	0x24810

#define	RETRY_RD_ERR_LOG_CH_0_REG			0x22C60
#define	RETRY_RD_ERR_LOG_ADDRESS1_CH_0_REG	0x22C58
#define	RETRY_RD_ERR_LOG_ADDRESS2_CH_0_REG	0x22C28
#define	RETRY_RD_ERR_LOG_PARITY_CH_0_REG	0x22C5C
#define RETRY_RD_ERR_LOG_ADDRESS3_CH0_REG	0x20ED8

#define	RETRY_RD_ERR_LOG_CH_1_REG			0x26C60
#define	RETRY_RD_ERR_LOG_ADDRESS1_CH_1_REG	0x26C58
#define	RETRY_RD_ERR_LOG_ADDRESS2_CH_1_REG	0x26C28
#define	RETRY_RD_ERR_LOG_PARITY_CH_1_REG	0x26C5C
#define RETRY_RD_ERR_LOG_ADDRESS3_CH1_REG	0x24ED8

#define	RETRY_RD_ERR_SET2_LOG_CH_0_REG		0x22E54
#define	RETRY_RD_ERR_SET2_LOG_ADDRESS1_CH_DP_REG	0x22E58
#define	RETRY_RD_ERR_SET2_LOG_ADDRESS2_CH_0_REG		0x22E5C
#define	RETRY_RD_ERR_SET2_LOG_PARITY_CH_0_REG		0x22E64
#define RETRY_RD_ERR_SET2_LOG_ADDRESS3_CH_0_REG		0x20EE0

#define	RETRY_RD_ERR_SET2_LOG_CH_1_REG		0x26E54
#define	RETRY_RD_ERR_SET2_LOG_ADDRESS1_CH_1_REG		0x26E58
#define	RETRY_RD_ERR_SET2_LOG_ADDRESS2_CH_1_REG		0x26E5C
#define	RETRY_RD_ERR_SET2_LOG_PARITY_CH_1_REG		0x26E64
#define RETRY_RD_ERR_SET2_LOG_ADDRESS3_CH_1_REG		0x24EE0

//valid address [51:0]
#define SYS_ADDR_MASK		0xFFFFFFFFFFFFF
#define NUMBER_OF_CHANNELS	2
#define NUMBER_OF_MMIO_REGISTERS_SETS	2

typedef struct MMIO_REGISTERS_ADDRESSES
{
    uint64_t RETRY_RD_ERR_LOG_REG;
    uint64_t RETRY_RD_ERR_LOG_ADDRESS1_REG;
    uint64_t RETRY_RD_ERR_LOG_ADDRESS2_REG;
    uint64_t RETRY_RD_ERR_LOG_PARITY_REG;
    uint64_t RETRY_RD_ERR_LOG_ADDRESS3;
} MMIO_REGISTERS_ADDRESSES;


typedef union
{
    ///
    /// Individual bit fields
    ///
    struct
    {
        uint32_t v : 1;

        /* Bits[0:0], Access Type=RW/V/P, default=None*/

        /*
           The contents of this register have valid data as
           well as RETRY_RD_ERR_LOG_PARITY,
           RETRY_RD_ERR_LOG_MISC,
           RETRY_RD_ERR_LOG_ADDRESS1, and
           RETRY_RD_ERR_LOG_ADDRESS2.
        */
        uint32_t uc : 1;

        /* Bits[1:1], Access Type=RW/V/P, default=None*/

        /*
           Uncorrectable error. Logs cannot be overwritten
           while UC is set.
        */
        uint32_t over : 1;

        /* Bits[2:2], Access Type=RW/V/P, default=None*/

        /* The log has been overwritten. */
        uint32_t mode : 4;

        /* Bits[6:3], Access Type=RW/V/P, default=None*/

        /*
           indicate the ecc mode the register is logging
           for : 4b0000 => sddc 2LM 4b0001 => sddc 1LM
           4b0010 => sddc +1 2LM 4b0011 => sddc +1 1LM
           4b0100 => adddc 2LM 4b0101 => adddc 1LM 4b0110
           => adddc +1 2LM 4b0111 => adddc +1 1LM 4b1000 =>
           read is from ddrt dimm 4b1001 => x8 sddc 4b1010
           => x8 sddc +1 4b1011 => not a valid ecc mode
        */
        uint32_t corr_err : 1;

        /* Bits[7:7], Access Type=RW/V/P, default=None*/

        /*
           During the last log, entered correction but was
           not able to correct hte error => DUE
        */
        uint32_t corr_noerr : 1;

        /* Bits[8:8], Access Type=RW/V/P, default=None*/

        /*
           During the last log, entered correction but no
           ecc error was detected
        */
        uint32_t corr_correctable : 1;

        /* Bits[9:9], Access Type=RW/V/P, default=None*/

        /*
           During the last log, entered correction and
           detect a correctable ecc error and corrected
        */
        uint32_t corr_correctable_par_dev : 1;

        /* Bits[10:10], Access Type=RW/V/P, default=None*/

        /*
           During the last log, entered correction, error
           was in parity device which consider a
           correctable error Parity device logging support
           for different modes: x8 none SVL does NOT
           support device logging All +1 modes do NOT
           support device logging x8 SVL supports device
           logging x4 SDDC supports device logging x4 ADDDC
           supports device logging
        */
        uint32_t ecc_err : 1;

        /* Bits[11:11], Access Type=RW/V/P, default=None*/

        /*
           During the last log, the ecc error indication
           sent to m2m
        */
        uint32_t patspr : 1;

        /* Bits[12:12], Access Type=RW/V/P, default=None*/

        /*
           Indicates if we have logged a patrol/spare
           error.If patspr=1 we have logged patrol/spare
           error
        */
        uint32_t en_patspr : 1;

        /* Bits[13:13], Access Type=RW/P, default=None*/

        /*
           If en_patspr=1: Log patrol/sparing errors and
           NOT demand/ufill reads,If en_patspr=0: Log
           demand/ufill reads and NOT patrol/sparing
           .Default is to log demand/ufill reads.
        */
        uint32_t noover : 1;

        /* Bits[14:14], Access Type=RW/P, default=None*/

        /*
           Lock register after the first error, do not
           overflow.
        */
        uint32_t en : 1;

        /* Bits[15:15], Access Type=RW/P, default=None*/

        /* Enable error logging. Will log on every retry. */
        uint32_t col_correction : 1;

        /* Bits[16:16], Access Type=RW/V/P, default=None*/

        /*
           This field indicate a column correction done
           instead of the device correction and is only
           valid for 1LM adddc mode (retry_rd_err_log.mode
           = 4'b0101). In all other modes, this field
           should be ignored.
        */
        uint32_t single_bit_error : 1;

        /* Bits[17:17], Access Type=RW/V/P, default=None*/

        /*
           Indicates if there was a single bit correction -
           SDDC and SDDC +1 modes, used in PCLS
        */
        uint32_t transfer_number : 3;

        /* Bits[20:18], Access Type=RW/V/P, default=None*/

        /*
           Indicates Transfer number of the single bit
           correction,used in PCLS
        */
        uint32_t persistent_error : 1;

        /* Bits[21:21], Access Type=RW/V/P, default=None*/

        /*
           Indicates persistent error indicated by BIST,
           for HBM
        */
        uint32_t system_addr_valid : 1;

        /* Bits[22:22], Access Type=RW/V/P, default=None*/

        /*
           Indicates there is valid system address in the
           retry_rd_err_address3_log register
        */
        uint32_t mirror : 1;

        /* Bits[23:23], Access Type=RW/V/P, default=None*/

        /* Indicates if the transaction is mirrored */
        uint32_t mirror_pri : 1;

        /* Bits[24:24], Access Type=RW/V/P, default=None*/

        /*
           Indicates if the mirrored transaction is to
           primary channel
        */
        uint32_t mirror_failover : 1;

        /* Bits[25:25], Access Type=RW/V/P, default=None*/

        /*
           Indicates if the mirrored transaction is to
           failed over channel
        */
        uint32_t ecc_bist_valid : 1;

        /* Bits[26:26], Access Type=RW/V/P, default=None*/

        /*
           Indicates that ECCBIST is enabled. The fields
           ecc_bist_errtype and ecc_bist_corr_err_devnum
           are only valid if this is true
        */
        uint32_t ddrt_modeb_uc : 1;

        /* Bits[27:27], Access Type=RW/V/P, default=None*/

        /*
           Indicates that in DDRT ECC mode B - The
           correction fsm indicates correctable but the
           failed device doesnt match the one identified by
           BIOS. So we mark the result as UC
        */
        uint32_t rsvd : 4;

        /* Bits[31:28], Access Type=RO, default=None*/

        /* Reserved */

    } Bits;
    uint32_t Data;

} RETRY_RD_ERR_LOG_MCDDC_DP_STRUCT;

typedef union
{
    ///
    /// Individual bit fields
    ///
    struct
    {
        uint32_t failed_dev : 6;

        /* Bits[5:0], Access Type=RW/V/P, default=0x00000000*/

        /*
           Holds the failed device number: for sddc/ddrt/x8
           the device number is between 0-17. for adddc
           device number between 0-16 and 18-35 dev 17 is
           the spare device
        */
        uint32_t chip_select : 3;

        /* Bits[8:6], Access Type=RW/V/P, default=0x00000000*/

        /*
           Rank CS0-CS7 (encoded chip select) of retried
           read
        */
        uint32_t cbit : 4;

        /* Bits[12:9], Access Type=RW/V/P, default=0x00000000*/

        /*
           C0-C2 (encoded subrank) for last retry. Bit 12
           is spare to accomodate for future growth in DDR5
           Logged value is {1'b0,cbit[2:0]}
        */
        uint32_t bank : 6;

        /* Bits[18:13], Access Type=RW/V/P, default=0x00000000*/

        /*
           Bank ID for last retry. Bottom three bits are
           Bank Group, top two bits are Bank Address Logged
           value is {1'b0,bank_group[2],bank_address[1:0],b
           ank_group[1:0]} Bit 18 is spare to accomodate
           for future growth in DDR5
        */
        uint32_t col : 10;

        /* Bits[28:19], Access Type=RW/V/P, default=0x00000000*/

        /*
           Column address for the last retry Logged value
           is DDR - {2'b0,Column_addr[9:3],1'b0}.
           Column_addr[2] indicates Critical chunk, so
           making that as 0 HBM - {2'b0,Column_addr[9:2]}
           Bits 9 and 8 as spare to accomodate possible
           future growth in size for DDR5 In DDR - Actual
           Column address = {Column_addr[9:3],3'b0}
           Column_addr[2] indicates Critical chunk In HBM -
           Actual Column address = {Column_addr[9:2],2'b0}
        */
        uint32_t rsvd : 3;

        /* Bits[31:29], Access Type=RO, default=None*/

        /* Reserved */

    } Bits;
    uint32_t Data;

} RETRY_RD_ERR_LOG_ADDRESS1_MCDDC_DP_STRUCT;

typedef union
{
    ///
    /// Individual bit fields
    ///
    struct
    {
        uint32_t row : 21;

        /* Bits[20:0], Access Type=RW/V/P, default=0x00000000*/

        /*
           Row address for the last retry. Logged value:
           {3'b0,Row_Addr[17:0]}
        */
        uint32_t ecc_bist_errtype : 2;

        /* Bits[22:21], Access Type=RW/V/P, default=None*/

        /*
           Indicates the err type for the BIST result.
           Valid only if ecc_bist_valid field is true in
           the retry_rd_err_log. BIST Result No Error =
           2'b00 BIST Result Uncorrectable = 2'b01 BIST
           Result Correctable = 2'b10
        */
        uint32_t ecc_bist_corr_err_devnum : 5;

        /* Bits[27:23], Access Type=RW/V/P, default=None*/

        /*
           Indicates the device number for the correctable
           error signaled by BIST. Valid only when
           ecc_bist_valid is true and ecc_bist_errtype is
           correctable
        */
        uint32_t corr_fsm_errtype : 2;

        /* Bits[29:28], Access Type=RW/V/P, default=None*/

        /*
           Indicates the err type for the Corection fsm
           result. BIST Result No Error = 2'b00 BIST Result
           Uncorrectable = 2'b01 BIST Result Correctable =
           2'b10 This isnt logged when in BIST bypass mode
           or error reporting mode as the retry_rd_err_log
           contents represents only the correction fsm
           results. valid only for BIST SDC control mode.
        */
        uint32_t rsvd : 2;

        /* Bits[31:30], Access Type=RO, default=None*/

        /* Reserved */

    } Bits;
    uint32_t Data;

} RETRY_RD_ERR_LOG_ADDRESS2_MCDDC_DP_STRUCT;

typedef union
{
    ///
    /// Individual bit fields
    ///
    struct
    {
        uint32_t par_syn;

        /* Bits[31:0], Access Type=RW/V/P, default=00000000h*/

        /*
            this register hold the parity syndrome(mask)
            from correction path. for sddc/x8/ddrt parity
            syndrome is 32 bits. but for 2LM adddc it is
            16 bits so the 16 upper bits are always 0 for
            2LM adddc: 16 bits 0 + Parity Syndrome (16
            bits). in 1LM adddc reused the 8 upper bits
            to log the Parity Column Syndrome. so the
            order for 1LM adddc will be: PC syn for upper
            half of CL (4bits)+ PC syn for lower half of
            CL (4bits)+ 8 bits 0 + Parity Syndrome (16
            bits)
        */

    } Bits;
    uint32_t Data;

} RETRY_RD_ERR_LOG_PARITY_STRUCT;
#endif
