//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the XML settings writer.
//=============================================================================

#include "settings/settings_writer.h"

namespace rra
{
    SettingsWriter::SettingsWriter(Settings* settings)
        : settings_(settings)
    {
    }

    SettingsWriter::~SettingsWriter()
    {
    }

    bool SettingsWriter::Write(QIODevice* device)
    {
        writer_.setDevice(device);

        writer_.writeStartDocument();
        writer_.writeStartElement("RRA");

        WriteSettingsAndRecents();

        writer_.writeEndElement();
        writer_.writeEndDocument();

        return writer_.hasError() == false;
    }

    void SettingsWriter::WriteSettingsAndRecents()
    {
        writer_.writeStartElement("GlobalSettings");
        WriteSettings();
        writer_.writeEndElement();

        writer_.writeStartElement("RecentFiles");
        WriteRecentFiles();
        writer_.writeEndElement();
    }

    void SettingsWriter::WriteSettings()
    {
        const SettingsMap& settings = settings_->GetSettings();

        for (SettingsMap::const_iterator i = settings.begin(); i != settings.end(); ++i)
        {
            writer_.writeStartElement("Setting");
            WriteSetting(i.value());
            writer_.writeEndElement();
        }
    }

    void SettingsWriter::WriteSetting(const Setting& setting)
    {
        writer_.writeTextElement("Name", setting.name);
        writer_.writeTextElement("Value", setting.value);
    }

    void SettingsWriter::WriteRecentFiles()
    {
        const QVector<RecentFileData>& recent_files = settings_->RecentFiles();

        for (int loop = 0; loop < recent_files.size(); loop++)
        {
            writer_.writeStartElement("RecentFile");
            WriteRecentFile(recent_files.at(loop));
            writer_.writeEndElement();
        }
    }

    void SettingsWriter::WriteRecentFile(const RecentFileData& recent_file)
    {
        writer_.writeTextElement("Path", recent_file.path);
        writer_.writeTextElement("Keywords", recent_file.keywords);
        writer_.writeTextElement("API", recent_file.api);
        writer_.writeTextElement("Created", recent_file.created);
        writer_.writeTextElement("Accessed", recent_file.accessed);
        writer_.writeTextElement("Events", recent_file.events);
        writer_.writeTextElement("DeviceID", recent_file.device_id);
        writer_.writeTextElement("DeviceRevisionID", recent_file.device_revision_id);
        writer_.writeTextElement("DeviceString", recent_file.device_string);
    }
}  // namespace rra
