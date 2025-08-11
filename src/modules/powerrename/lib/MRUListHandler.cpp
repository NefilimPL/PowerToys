#include "pch.h"
#include "MRUListHandler.h"
#include "Helpers.h"

#include <dll/PowerRenameConstants.h>
#include <common/SettingsAPI/settings_helpers.h>

namespace
{
    const wchar_t c_mruList[] = L"MRUList";
    const wchar_t c_insertionIdx[] = L"InsertionIdx";
    const wchar_t c_maxMRUSize[] = L"MaxMRUSize";
    const wchar_t c_pinnedList[] = L"PinnedList";
}

MRUListHandler::MRUListHandler(unsigned int size, const std::wstring& filePath, const std::wstring& regPath) :
    nextIdx(1),
    size(size),
    jsonFilePath(PTSettingsHelper::get_module_save_folder_location(PowerRenameConstants::ModuleKey) + filePath),
    registryFilePath(regPath)
{
    Load();
}

void MRUListHandler::Push(const std::wstring& data)
{
    if (Exists(data))
    {
        return;
    }
    items.push_back(data);
    if (items.size() > size)
    {
        items.erase(items.begin());
    }
    UpdateAllItems();
    Save();
}

void MRUListHandler::TogglePin(const std::wstring& data)
{
    if (Exists(pinnedItems, data))
    {
        Unpin(data);
    }
    else
    {
        Pin(data);
    }
}

bool MRUListHandler::Next(std::wstring& data)
{
    if (nextIdx > allItems.size())
    {
        Reset();
        return false;
    }
    data = allItems[allItems.size() - nextIdx];
    nextIdx++;
    return true;
}

void MRUListHandler::Reset()
{
    nextIdx = 1;
}

const std::vector<std::wstring>& MRUListHandler::GetItems()
{
    return allItems;
}

void MRUListHandler::Load()
{
    if (!std::filesystem::exists(jsonFilePath))
    {
        MigrateFromRegistry();

        Save();
    }
    else
    {
        ParseJson();
    }
}

void MRUListHandler::Save()
{
    json::JsonObject jsonData;

    jsonData.SetNamedValue(c_maxMRUSize, json::value(size));
    jsonData.SetNamedValue(c_insertionIdx, json::value(0));
    jsonData.SetNamedValue(c_mruList, Serialize(items));
    jsonData.SetNamedValue(c_pinnedList, Serialize(pinnedItems));

    json::to_file(jsonFilePath, jsonData);
}

json::JsonArray MRUListHandler::Serialize(const std::vector<std::wstring>& list)
{
    json::JsonArray searchMRU{};

    for (const std::wstring& item : list)
    {
        searchMRU.Append(json::value(item));
    }

    return searchMRU;
}

void MRUListHandler::MigrateFromRegistry()
{
    std::wstring searchListKeys = GetRegString(c_mruList, registryFilePath);
    std::sort(std::begin(searchListKeys), std::end(searchListKeys));
    for (const wchar_t& key : searchListKeys)
    {
        Push(GetRegString(std::wstring(1, key), registryFilePath));
    }
}

void MRUListHandler::ParseJson()
{
    auto json = json::from_file(jsonFilePath);
    if (json)
    {
        const json::JsonObject& jsonObject = json.value();
        try
        {
            if (json::has(jsonObject, c_pinnedList, json::JsonValueType::Array))
            {
                auto jsonPinned = jsonObject.GetNamedArray(c_pinnedList);
                for (uint32_t i = 0; i < jsonPinned.Size(); ++i)
                {
                    pinnedItems.push_back(std::wstring(jsonPinned.GetStringAt(i)));
                }
            }
            if (json::has(jsonObject, c_mruList, json::JsonValueType::Array))
            {
                auto jsonArray = jsonObject.GetNamedArray(c_mruList);
                for (uint32_t i = 0; i < jsonArray.Size(); ++i)
                {
                    items.push_back(std::wstring(jsonArray.GetStringAt(i)));
                }
            }
            if (items.size() > size)
            {
                items.erase(items.begin(), items.begin() + (items.size() - size));
            }
            UpdateAllItems();
        }
        catch (const winrt::hresult_error&)
        {
        }
    }
}

bool MRUListHandler::Exists(const std::wstring& data)
{
    return Exists(items, data) || Exists(pinnedItems, data);
}

bool MRUListHandler::Exists(const std::vector<std::wstring>& list, const std::wstring& data)
{
    return std::find(std::begin(list), std::end(list), data) != std::end(list);
}

void MRUListHandler::Pin(const std::wstring& data)
{
    auto it = std::find(items.begin(), items.end(), data);
    if (it != items.end())
    {
        items.erase(it);
    }
    pinnedItems.insert(pinnedItems.begin(), data);
    UpdateAllItems();
    Save();
}

void MRUListHandler::Unpin(const std::wstring& data)
{
    auto it = std::find(pinnedItems.begin(), pinnedItems.end(), data);
    if (it != pinnedItems.end())
    {
        pinnedItems.erase(it);
        Push(data);
    }
}

void MRUListHandler::UpdateAllItems()
{
    allItems.clear();
    allItems.insert(allItems.end(), pinnedItems.begin(), pinnedItems.end());
    allItems.insert(allItems.end(), items.begin(), items.end());
    Reset();
}
