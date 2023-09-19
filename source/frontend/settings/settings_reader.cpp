//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the XML settings reader.
//=============================================================================

#include "settings/settings_reader.h"

namespace rra
{
    SettingsReader::SettingsReader(Settings* settings)
        : settings_(settings)
    {
    }

    SettingsReader::~SettingsReader()
    {
    }

    bool SettingsReader::Read(QIODevice* device)
    {
        reader_.setDevice(device);

        if (reader_.readNextStartElement())
        {
            if (reader_.name() == QString("RRA"))
            {
                ReadSettingsAndRecents();
            }
        }

        return reader_.error() == false;
    }

    void SettingsReader::ReadSettingsAndRecents()
    {
        while (reader_.readNextStartElement())
        {
            if (reader_.name() == QString("GlobalSettings"))
            {
                ReadSettings();
            }
            else if (reader_.name() == QString("RecentFiles"))
            {
                ReadRecentFiles();
            }
            else
            {
                reader_.skipCurrentElement();
            }
        }
    }

    void SettingsReader::ReadSettings()
    {
        while (reader_.readNextStartElement())
        {
            if (reader_.name() == QString("Setting"))
            {
                ReadSetting();
            }
            else
            {
                reader_.skipCurrentElement();
            }
        }
    }

    void SettingsReader::ReadSetting()
    {
        Setting setting;

        while (reader_.readNextStartElement())
        {
            if (reader_.name() == QString("Name"))
            {
                setting.name = reader_.readElementText();
            }
            else if (reader_.name() == QString("Value"))
            {
                setting.value = reader_.readElementText();
            }
            else
            {
                reader_.skipCurrentElement();
            }
        }

        settings_->AddPotentialSetting(setting.name, setting.value);
    }

    void SettingsReader::ReadRecentFiles()
    {
        while (reader_.readNextStartElement())
        {
            if (reader_.name() == QString("RecentFile"))
            {
                ReadRecentFile();
            }
            else
            {
                reader_.skipCurrentElement();
            }
        }
    }

    void SettingsReader::ReadRecentFile()
    {
        RecentFileData recent_file;

        while (reader_.readNextStartElement())
        {
            if (reader_.name() == QString("Path"))
            {
                recent_file.path = reader_.readElementText();
            }
            else if (reader_.name() == QString("Keywords"))
            {
                recent_file.keywords = reader_.readElementText();
            }
            else if (reader_.name() == QString("API"))
            {
                recent_file.api = reader_.readElementText();
            }
            else if (reader_.name() == QString("Created"))
            {
                recent_file.created = reader_.readElementText();
            }
            else if (reader_.name() == QString("Accessed"))
            {
                recent_file.accessed = reader_.readElementText();
            }
            else if (reader_.name() == QString("Events"))
            {
                recent_file.events = reader_.readElementText();
            }
            else if (reader_.name() == QString("DeviceID"))
            {
                recent_file.device_id = reader_.readElementText();
            }
            else if (reader_.name() == QString("DeviceRevisionID"))
            {
                recent_file.device_revision_id = reader_.readElementText();
            }
            else if (reader_.name() == QString("DeviceString"))
            {
                recent_file.device_string = reader_.readElementText();
            }
            else
            {
                reader_.skipCurrentElement();
            }
        }

        settings_->AddRecentFile(recent_file);
    }
}  // namespace rra
