#pragma once
#include "pch.h"

#include <filesystem>
#include <string>

#include <common/utils/json.h>

class MRUListHandler
{
public:
    MRUListHandler(unsigned int size, const std::wstring& filePath, const std::wstring& regPath);

    void Push(const std::wstring& data);
    bool Next(std::wstring& data);

    // Pins the entry if it's not pinned yet, otherwise unpins it.
    void TogglePin(const std::wstring& data);

    void Reset();

    const std::vector<std::wstring>& GetItems();
private:
    void Load();
    void Save();
    void MigrateFromRegistry();
    json::JsonArray Serialize(const std::vector<std::wstring>& list);
    void ParseJson();

    bool Exists(const std::vector<std::wstring>& list, const std::wstring& data);
    bool Exists(const std::wstring& data);
    void Pin(const std::wstring& data);
    void Unpin(const std::wstring& data);
    void UpdateAllItems();

    std::vector<std::wstring> pinnedItems;
    std::vector<std::wstring> items;
    std::vector<std::wstring> allItems;
    unsigned int nextIdx;
    unsigned int size;
    const std::wstring jsonFilePath;
    const std::wstring registryFilePath;
};
