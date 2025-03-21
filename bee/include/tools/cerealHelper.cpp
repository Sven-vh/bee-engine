#include "cerealHelper.hpp"
#include "core.hpp"

void bee::CerealHelper::CompareValues(const rapidjson::Value& before,
                                      const rapidjson::Value& after,
                                      rapidjson::Value& diff,
                                      rapidjson::Document::AllocatorType& allocator)
{
    if (before.IsObject() && after.IsObject())
    {
        for (auto it = before.MemberBegin(); it != before.MemberEnd(); ++it)
        {
            const char* key = it->name.GetString();
            if (after.HasMember(key))
            {
                rapidjson::Value nestedDiff(rapidjson::kObjectType);
                CompareValues(before[key], after[key], nestedDiff, allocator);

                if (!nestedDiff.ObjectEmpty() || before[key] != after[key])
                {
                    diff.AddMember(rapidjson::Value().SetString(key, allocator), nestedDiff, allocator);
                }
            }
        }
    }
    else if (before != after)
    {
        diff.CopyFrom(after, allocator);  // Directly assign the changed value
    }
}

std::string bee::CerealHelper::LogEntityChanges(const std::string& beforeStr, const std::string& afterStr)
{
    rapidjson::Document before, after, diff;
    diff.SetObject();
    before.Parse(beforeStr.c_str());
    after.Parse(afterStr.c_str());
    CompareValues(before, after, diff, diff.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    diff.Accept(writer);
    std::cout << "Changes detected: " << buffer.GetString() << std::endl;
    return buffer.GetString();
}