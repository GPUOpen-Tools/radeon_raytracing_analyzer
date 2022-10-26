//=============================================================================
// Copyright (c) 2021-2022 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Structures and functions for working with a data set.
//=============================================================================

#ifndef RRA_BACKEND_RRA_DATA_SET_H_
#define RRA_BACKEND_RRA_DATA_SET_H_

#include <stdio.h>
#include <time.h>

#include "public/rra_error.h"

#include "rra_configuration.h"

#include "bvh/bvh_bundle.h"
#include "api_info.h"
#include "asic_info.h"

#ifdef __cplusplus
extern "C" {
#endif  // #ifdef __cplusplus

/// A structure encapsulating a single RRA dataset.
typedef struct RraDataSet
{
    char                            file_path[RRA_MAXIMUM_FILE_PATH];  ///< The file path to the file being worked with.
    bool                            file_loaded;                       ///< Has a file been successfully loaded.
    size_t                          file_size_in_bytes;                ///< The size of the file pointed to by <c><i>fileHandle</i></c> in bytes.
    time_t                          create_time;                       ///< The time the trace was created.
    std::unique_ptr<rta::BvhBundle> bvh_bundle;                        ///< The BVH bundle class encapsulating all the BLAS and TLAS for the loaded trace.
    rra::ApiInfo                    api_info  = {};                    ///< The API info.
    rra::AsicInfo                   asic_info = {};                    ///< The ASIC info.
} RraDataSet;

/// Initialize the RRA data set from a file path.
///
/// @param [in]  path                           A pointer to a string containing the path to the RRA file that we would like to load to initialize the data set.
/// @param [in]  data_set                       A pointer to a <c><i>RraDataSet</i></c> structure that will contain the data set.
///
/// @retval
/// kRraOk                                      The operation completed successfully.
/// @retval
/// kRraErrorInvalidPointer                     The operation failed due to <c><i>data_set</i></c> being set to <c><i>NULL</i></c>.
RraErrorCode RraDataSetInitialize(const char* path, RraDataSet* data_set);

/// Destroy the data set.
///
/// @param [in]  data_set                       A pointer to a <c><i>RraDataSet</i></c> structure that will contain the data set.
///
/// kRraOk                                      The operation completed successfully.
/// @retval
/// kRraErrorInvalidPointer                     The operation failed due to <c><i>data_set</i></c> being set to <c><i>NULL</i></c>.
RraErrorCode RraDataSetDestroy(RraDataSet* data_set);

#ifdef __cplusplus
}
#endif  // #ifdef __cplusplus
#endif  // #ifndef RRA_BACKEND_RRA_DATA_SET_H_
