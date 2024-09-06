//==============================================================================
// Copyright (c) 2016-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Linux definition of Windows safe CRT functions.
//==============================================================================

#ifndef BACKEND_PUBLIC_LINUX_SAFE_CRT_H_
#define BACKEND_PUBLIC_LINUX_SAFE_CRT_H_

#if !defined(_WIN32)

#include <stdio.h>

/// errno_t defined so that the function prototypes match the Windows function prototypes.
typedef int errno_t;

/// fopen_s secure version of fopen.
/// \param file A pointer to the file pointer that will receive the pointer to the opened file.
/// \param filename The filename of the file to be opened.
/// \param mode Type of access permitted.
/// \return Zero if successful; an error code on failure.
errno_t fopen_s(FILE** file, const char* filename, const char* mode);

/// sprintf_s secure version of sprintf.
/// \param buffer Storage location for output.
/// \param size_of_buffer Maximum number of characters to store.
/// \param format Format-control string.
/// \param ... Optional arguments to be formatted.
/// \return The number of characters written or -1 if an error occurred.
int sprintf_s(char* buffer, size_t size_of_buffer, const char* format, ...);

/// fprintf_s secure version of fprintf.
/// \param stream Pointer to FILE structure.
/// \param format Format-control string.
/// \param ... Optional arguments to be formatted.
/// \return The number of bytes written, or a negative value when an output error occurs.
int fprintf_s(FILE* stream, const char* format, ...);

/// fread_s secure version of fread.
/// \param buffer Storage location for data.
/// \param buffer_size Size of the destination buffer in bytes.
/// \param element_size Size of the item to read in bytes.
/// \param count Maximum number of items to be read.
/// \param stream Pointer to FILE structure.
/// \return The number of (whole) items that were read into the buffer, which may be less than count if a
///         read error or the end of the file is encountered before count is reached. Use the feof or ferror
///         function to distinguish an error from an end-of-file condition. If size or count is 0, fread_s
///         returns 0 and the buffer contents are unchanged.
size_t fread_s(void* buffer, size_t buffer_size, size_t element_size, size_t count, FILE* stream);

/// strcpy_s secure version of strcpy.
/// \param destination The destination (output) string.
/// \param size The maximum number of bytes to copy.
/// \param source The source (input) string.
/// \return 0 on success, non-zero on error.
errno_t strcpy_s(char* destination, size_t size, const char* source);

/// @brief strncpy_s secure version of strncpy.
///
/// Safely copies characters from the source string to the destination string and adds a terminator to the end of the destination string.
/// Truncated strings (i.e. source strings that are longer than the size of the destination buffer) can be copied by specifying a
/// <c><i>max_count</i></c> which is one less than the actual size of the destination buffer.  A terminating character is added to the
/// truncated destination string.
///
/// @param [out] out_destination    The destination (output) string.
/// @param [in]  destination_size   The size of the destination buffer.
/// @param [in]  source             The source (input) string.
/// @param [in]  max_count          The maximum number of bytes to copy.
///
/// @retval
/// 0                               The operation completed succesfully.
/// @retval
/// ERANGE                          The number of characters to copy is zero.
/// @retval
/// EINVAL                          The operation failed because <c><i>out_destination</i></c> or <c>source<i>out_destination</i></c> was a null pointer.
errno_t strncpy_s(char* out_destination, const size_t destination_size, const char* source, const size_t max_count);

/// strcat_s secure version of strcat.
/// \param destination The destination (output) string.
/// \param size The maximum number of bytes to copy.
/// \param source The source (input) string.
/// \return 0 on success, non-zero on error.
errno_t strcat_s(char* destination, size_t size, const char* source);

#endif  // !_WIN32

#endif  // BACKEND_PUBLIC_LINUX_SAFE_CRT_H_
