//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of useful utility functions.
//=============================================================================

#include "util/file_util.h"

#ifdef _WIN32
#include <Shlobj.h>
#include <Windows.h>
#else
#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>

#include "public/linux/safe_crt.h"
#endif

#include <QDir>

#include "public/rra_assert.h"

#include "settings/settings.h"

QString file_util::GetFileLocation()
{
    QString file_location = "";

#ifdef _WIN32
    LPWSTR  wsz_path = nullptr;
    HRESULT hr       = SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &wsz_path);
    Q_UNUSED(hr);
    Q_ASSERT(hr == S_OK);
    if (hr == S_OK)
    {
        file_location = QString::fromUtf16(reinterpret_cast<const char16_t*>(wsz_path));
        file_location.append("/" + rra::kExecutableBaseFilename);
    }

#else

    struct passwd* pw = getpwuid(getuid());
    if (pw != nullptr)
    {
        const char* homedir = pw->pw_dir;
        file_location       = homedir;
    }
    file_location.append("/." + rra::kExecutableBaseFilename);
#endif

    // Make sure the folder exists. If not, create it.
    std::string dir = file_location.toStdString();
    if (QDir(dir.c_str()).exists() == false)
    {
        QDir qdir;
        qdir.mkpath(dir.c_str());
    }

    return file_location;
}

