//=============================================================================
// Copyright (c) 2020-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of a number of string utilities.
//=============================================================================

#include "string_util.h"

#include <cctype>
#include <QtMath>
#include <QTextStream>
#include <QLocale>

QString rra::string_util::ToUpperCase(const QString& string)
{
    QString out;

    for (int i = 0; i < string.length(); i++)
    {
        char c = string.at(i).toLatin1();
        out.append(c >= 'a' && c <= 'z' ? QChar(c - 32) : QChar(c));
    }

    return out;
}

std::vector<std::string> rra::string_util::Split(const std::string& s, const std::string& delim)
{
    std::vector<std::string> result;

    size_t start = 0U;
    size_t end   = s.find(delim);
    while (end != std::string::npos)
    {
        result.push_back(s.substr(start, end - start));
        start = end + delim.length();
        end   = s.find(delim, start);
    }

    result.push_back(s.substr(start, end));
    return result;
}

std::string rra::string_util::Trim(std::string s)
{
    // trim from left (in place)
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    // trim from right (in place)
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());

    return s;
}

QString rra::string_util::LocalizedValue(int64_t value)
{
    QString     str = "";
    QTextStream out(&str);
    out.setRealNumberNotation(QTextStream::FixedNotation);
    out.setLocale(QLocale::English);
    out << value;
    return str;
}

QString rra::string_util::LocalizedValuePrecise(double value)
{
    QString     str = "";
    QTextStream out(&str);
    out.setRealNumberPrecision(2);
    out.setRealNumberNotation(QTextStream::FixedNotation);
    out.setLocale(QLocale::English);
    out << value;
    return str;
}

QString rra::string_util::LocalizedValueMemory(double value, bool base_10, bool use_round)
{
    double multiple;
    if (base_10)
    {
        multiple = 1000;
    }
    else
    {
        multiple = 1024;
    }

    double scaled_size = value;

    int postfix_index = 0;
    while (fabs(scaled_size) >= multiple)
    {
        scaled_size /= multiple;
        postfix_index++;
    }

    if (use_round)
    {
        scaled_size = round(scaled_size);
    }

    static const QString kBinarySizePostfix[] = {" bytes", " KiB", " MiB", " GiB", " TiB", " PiB"};
    static const QString kBase10SizePostfix[] = {" bytes", " KB", " MB", " GB", " TB", " PB"};

    // If index is too large, it's probably down to bad data so display as bytes in this case.
    if (postfix_index >= 6)
    {
        postfix_index = 0;
        scaled_size   = value;
    }

    // Display value string to 2 decimal places if not bytes. No fractional part for bytes.
    QString value_string;
    if (postfix_index != 0)
    {
        value_string = LocalizedValuePrecise(scaled_size);
    }
    else
    {
        value_string = LocalizedValue(scaled_size);
    }

    if (base_10)
    {
        return value_string + kBase10SizePostfix[postfix_index];
    }
    else
    {
        return value_string + kBinarySizePostfix[postfix_index];
    }
}

QString rra::string_util::GetBuildTypeString(VkBuildAccelerationStructureFlagBitsKHR build_flags)
{
    bool fast_trace = build_flags & VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    bool fast_build = build_flags & VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;

    if (fast_trace && fast_build)
    {
        return QString("Both");
    }
    else if (fast_trace)
    {
        return QString("Fast trace");
    }
    else if (fast_build)
    {
        return QString("Fast build");
    }

    return QString("Default");
}
