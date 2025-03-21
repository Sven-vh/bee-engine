#pragma once
#include "cereal/external/rapidjson/prettywriter.h"
#include "cereal/external/rapidjson/ostreamwrapper.h"
#include "cereal/external/rapidjson/istreamwrapper.h"
#include "cereal/external/rapidjson/stringbuffer.h"
#include "cereal/external/rapidjson/document.h"
#include "cereal/external/base64.hpp"
#include <iostream>
#include "common.hpp"

namespace bee
{
class CerealHelper
{
public:
    static void CompareValues(const rapidjson::Value& before,
                              const rapidjson::Value& after,
                              rapidjson::Value& diff,
                              rapidjson::Document::AllocatorType& allocator);

    static std::string LogEntityChanges(const std::string& beforeStr, const std::string& afterStr);
};
}  // namespace bee