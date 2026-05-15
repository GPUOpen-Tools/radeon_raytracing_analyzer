//=============================================================================
// Copyright (c) 2021-2026 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Backend test TLAS to BLAS mappings.
///
/// Make sure all the pointers from the TLAS instance nodes point to valid
/// BLAS objects.
//=============================================================================

#ifndef RRA_BACKEND_TEST_RRA_TEST_TLAS_BLAS_MAPPING_H_
#define RRA_BACKEND_TEST_RRA_TEST_TLAS_BLAS_MAPPING_H_

#include <string>

#include "log.h"
#include "rra_test_base.h"
#include "rra_test_config.h"

namespace backend_test
{
    class RRATestTlasBlasMapping : public RRATestBase
    {
    public:
        /// @brief Constructor.
        explicit RRATestTlasBlasMapping(const std::string& test_name);

        /// @brief Destructor.
        ~RRATestTlasBlasMapping();

        /// @brief Test the TLAS and BLAS stats and validate the Data.
        ///
        /// @param [in] config  The current test configuration.
        ///
        /// @return true if the tests passed, false if not.
        bool RunTests(const RRATestConfig& config);

    private:
        /// @brief Get the BLAS stats.
        ///
        /// @param [in] config  The current test configuration.
        ///
        /// @return true if the tests passed, false if not.
        bool GetTLASStats(const size_t tlas_index, const RRATestConfig& config);

        /// @brief Get the BLAS stats.
        ///
        /// @param [in] config  The current test configuration.
        ///
        /// @return true if the tests passed, false if not.
        bool GetBLASStats(const RRATestConfig& config);

    };
}  // namespace backend_test

#endif  //  RRA_BACKEND_TEST_RRA_TEST_TLAS_BLAS_MAPPING_H_

