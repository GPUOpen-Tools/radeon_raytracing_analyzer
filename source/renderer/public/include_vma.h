//=============================================================================
// Copyright (c) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Include the vma header with the required pragmas.
///
/// VMA recommends disabling warnings for "warnings as errors" compilations.
//=============================================================================

#ifdef _WIN32
#pragma warning(push, 3)
#else
#pragma GCC diagnostic push

#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wtype-limits"
#endif
#include <vma/include/vk_mem_alloc.h>
#ifdef _WIN32
#pragma warning(pop)
#else
#pragma GCC diagnostic pop
#endif
