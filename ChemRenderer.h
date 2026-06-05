#pragma once

#include <string>

std::string renderChemSvg(const std::string& source);
std::string jsonEscape(const std::string& value);
std::string extractJsonStringField(const std::string& json, const std::string& fieldName);
