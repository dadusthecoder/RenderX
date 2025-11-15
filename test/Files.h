#pragma once
#include <iostream>
#include <string>
#include <windows.h>
#include <commdlg.h>
#include <locale>
#include <codecvt>
#include <vector>


namespace Files{

    // ----- Basic File Operations -----
    bool Exists(const std::string& path);
    bool CreateDirectory(const std::string& path);
    bool DeleteFile(const std::string& path);

    // ----- File Reading / Writing -----
    std::string ReadTextFile(const std::string& path);
    std::vector<uint8_t> ReadBinaryFile(const std::string& path);
    bool WriteTextFile(const std::string& path, const std::string& data);
    bool WriteBinaryFile(const std::string& path, const std::vector<uint8_t>& data);

    // ----- Path Utilities -----
    std::string GetFileName(const std::string& path);
    std::string GetExtension(const std::string& path);
    std::string GetParentPath(const std::string& path);

    // ----- Directory Listing -----
    std::vector<std::string> ListFiles(const std::string& directory, const std::string& extension = "");

    std::string WideToString(const std::wstring& wstr);
    std::string OpenFile();
}
