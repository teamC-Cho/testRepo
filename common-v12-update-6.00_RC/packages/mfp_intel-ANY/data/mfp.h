// INTEL CONFIDENTIAL
// 
// Copyright (c) 2019-2021 Intel(R) Corporation
// All Rights Reserved.
// 
// The source code contained or described herein and all documents related
// to the source code ("Material") are owned by Intel Corporation or its
// suppliers or licensors.
// Title to the Material remains with Intel Corporation or its suppliers and
// licensors. The Material contains trade secrets and proprietary and
// confidential information of Intel or its suppliers and licensors.
// The Material is protected by worldwide copyright and trade secret laws and
// treaty provisions. No part of the Material may be used, copied, reproduced,
// modified, published, uploaded, posted, transmitted, distributed, or
// disclosed in any way without Intel's prior express written permission.
// No license under any patent, copyright, trade secret or other intellectual
// property right is granted to or conferred upon you by disclosure or delivery
// of the Materials, either expressly, by implication, inducement, estoppel or
// otherwise. Any license under such intellectual property rights must be
// express and approved by Intel in writing.
// 
#include <stdint.h>
#include <string.h>
#include "stdio.h"

#ifndef MFP_H
#define MFP_H

#ifdef CONFIG_SPX_FEATURE_MFP_2_0
#define MAJOR_VERSION 2
#define MINOR_VERSION 0
#elif defined CONFIG_SPX_FEATURE_MFP_2_1
#define MAJOR_VERSION 2
#define MINOR_VERSION 1
#else
#define MAJOR_VERSION 1
#define MINOR_VERSION 0
#endif

#define MODE_SDDC_2LM 0
#define MODE_SDDC_1LM 1
#define MODE_SDDC_PLUS1_2LM 2
#define MODE_SDDC_PLUS1_1LM 3
#define MODE_ADDDC_2LM 4
#define MODE_ADDDC_1LM 5
#define MODE_ADDDC_PLUS1_2LM 6
#define MODE_ADDDC_PLUS1_1LM 7
#define MODE_DDRT_DIMM 8

#define CE 0
#define UE 1

// error code
#define MFP_OK 0
#define MFP_SYS_ERR 1  // please check linux variable errno
#define MFP_LOAD_ERR 2
#define MFP_DUMP_ERR 3
#define MFP_INITED_ERR 4
#define MFP_NOT_INITED_ERR 5
#define MFP_CHECKSUM_ERR 6
#define MFP_BAD_PARAM 7
#define MFP_OUT_OF_MEMORY 8
#define MFP_LOAD_INCOMPATIBLE 9

// should sync with PFA_LOG_SIZE and entry structure
#if defined (CONFIG_SPX_FEATURE_MFP_2_0) || defined (CONFIG_SPX_FEATURE_MFP_2_1)
#define MFP_PFA_ENTRY_SIZE 24
#else 
#define MFP_PFA_ENTRY_SIZE 16
#endif
#define MFP_PFA_ENTRY_COUNT 128


#if defined (CONFIG_SPX_FEATURE_MFP_2_0) || defined (CONFIG_SPX_FEATURE_MFP_2_1)
struct mfp_error {
    uint64_t
        // location
        socket : 3,
        imc : 2, channel : 2, dimm : 2, rank : 4, device: 6, bank_group : 3, bank : 2,
        row : 20, col : 10,
        // Correctable/ Uncorrectable: 0/1
        error_type : 1,

        // unused bits
        reserved_0 : 9;
    // defined for future extension, currently not used  
#if defined (CONFIG_SPX_FEATURE_MFP_2_0)
    uint32_t par_syn;
#elif defined (CONFIG_SPX_FEATURE_MFP_2_1)
    uint64_t par_syn;
#endif
    uint32_t mode: 4, reserved_1: 28;
};
#else 
struct mfp_error {
    uint64_t
        // location
        socket : 3,
        imc : 2, channel : 2, dimm : 2, rank : 4, bank_group : 3, bank : 2,
        row : 20, col : 10,
        // Correctable/ Uncorrectable: 0/1
        error_type : 1,

        // unused bits
        reserved : 15;
    // defined for future extension, currently not used  
    uint64_t syndrome;
};
#endif

struct mfp_dimm {
    /*       skt id      imc id   channel id   slot on channel       */
    uint16_t socket : 3, imc : 2, channel : 2, dimm : 2, reserved: 7;
};

struct mfp_part_number {
    char s[32];
};

struct mfp_dimm_entry {
    struct mfp_dimm loc;
    uint32_t sn;
#if defined (CONFIG_SPX_FEATURE_MFP_2_0) || defined (CONFIG_SPX_FEATURE_MFP_2_1)
	struct mfp_part_number pn;
#endif
};

struct mfp_evaluate_result {
    struct mfp_dimm loc;
    // dimm score
    uint8_t score;
};

struct mfp_pfa_log {
    uint8_t data[MFP_PFA_ENTRY_SIZE * MFP_PFA_ENTRY_COUNT];
};

int mfp_init(
            uint32_t timestamp,
            FILE* f, 
            size_t dimm_count, 
            struct mfp_dimm_entry* dimm_arr);
int mfp_fin();

int mfp_evaluate_dimm(uint32_t timestamp,
                      size_t err_count,
                      struct mfp_error* err_arr,
                      size_t dimm_count,
                      struct mfp_evaluate_result* result);

int mfp_save(FILE* f);

int mfp_dump_pfa_log(struct mfp_pfa_log* log);

#if defined CONFIG_SPX_FEATURE_MFP_2_1
struct mfp_dimm_change_log {
    struct mfp_dimm loc;
    uint32_t old_sn;
    uint32_t new_sn;
};

/******************************************************************************
 * Description:
 *  THis API is used by BMC as a way to detect DIMM topology changes between
 *  last system state file saved and current platform configuration.
 *
 * Function parameters:
 *  *dimm_change_count - Return the changed count analyzed by MFP lib, it will
 *                       indicate the size of returned dimm_change_arr
 *  **dimm_change_arr  - The list contains all the changed info compared between
 *                       current dimm configuration and original state file
 *                       which input during mfp_init().
 *
 *                       The following cases will be covered by each
 *                       dimm_change_log instance:
 *                       - New DIMM added: old_sn = 0, new_sn= new sn
 *                       - DIMM is removed: old_sn= old sn, new_sn = 0
 *                       - DIMM is replaced: old_sn = oldsn, new_sn=new sn
 ******************************************************************************/
int mfp_evaluate_dimm_topology(
    size_t *dimm_change_count,
    struct mfp_dimm_change_log** dimm_change_arr
);


/******************************************************************************
 * Description:
 *  This API is used to reset the original evaluation results
 *  for a specified DIMM.
 *
 * Function parameter:
 *  dimm              - The target dimm which needs to clean statistic data
 *                      and reset to initial score.
 ******************************************************************************/
int mfp_reset_dimm_score(
    struct mfp_dimm dimm
);

struct mfp_component_fault {
    uint16_t valid: 1, // indicates whether the entry is valid or not, since component
                       // fault fields (example: topN_row_fault) are fixed-size array, if total
                       // count of component faults (row_fault_count) is less than array size N,
                       // only valid entry is meaningful
             rank: 4, device: 6, bank_group: 3, bank: 2; // the location where the component fault exists
    uint16_t grain,    // unique location count of correctable errors in this fault component
             prone;    // indicates how likely the fault component is failure prone, the higher is more severe
    uint64_t max_row: 20, min_row: 20, // max_row == min_row if row fault
             max_col: 10, min_col: 10, // max_col == min_col if col fault
             reserved: 4;
};

#define TOPN 6

struct mfp_stat_result {
    uint16_t grain;                  // unique location count of correctable errors in this DIMM = hard_error (repeat) + soft_error (non-repeat)
    uint16_t hard_error_grain;       // unique location count of hard (repeat) errors in this DIMM
    uint32_t row_fault_count: 10,    // total count of faulty rows
             col_fault_count: 10,    // total count of faulty columns
             bank_fault_count: 10,   // total count of faulty banks
             err_storm: 1,           // indicates whether correctable error storm occurred (at least 60 errors in 1 minute)
			 err_daily_threshold: 1; // indicates whether daily correctable error threshold is met (at least 2000 CE occurred in latest 24 hours)
    uint64_t err_count;

    struct mfp_component_fault topN_row_fault[TOPN];  // Array of top N row faults with component fault detail
    struct mfp_component_fault topN_col_fault[TOPN];  // Array of top N column faults with component fault detail
    struct mfp_component_fault topN_bank_fault[TOPN]; // Array of top N bank faults with component fault detail
};

/******************************************************************************
 * Description:
 *  This API exposes the component level failure analysis result for specified
 *  DIMM based on the historic error occurrence.
 *  Some of indicators, example:
 *  row fault indicator could be a reference for further memory component
 *  isolation methods, such as PPR or page offlining.
 *
 * Function parameters:
 *  dimm              - Specify the dimm from which the result is generated
 *  result            - Holds all the component fault indicators and
 *                      the data will be filled after the API call.
 ******************************************************************************/
int mfp_stat(struct mfp_dimm dimm, struct mfp_stat_result *result);
#else /* CONFIG_SPX_FEATURE_MFP_2_1 */
struct mfp_stat_result { };
#endif

#endif /* MFP_H */
