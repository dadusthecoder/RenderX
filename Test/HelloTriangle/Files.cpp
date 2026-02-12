
#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#include <commdlg.h>
#elif defined(__linux__)
#include <cstdio>
#endif

#include <filesystem>
#include <fstream>
#include <iostream>
#include "Files.h"

namespace fs = std::filesystem;
namespace Files {

	bool Exists(const std::string& path) {
		return fs::exists(path);
	}

	bool CreateDirectory(const std::string& path) {
		try {
			return fs::create_directories(path);
		}
		catch (...) {
			return false;
		}
	}

	bool DeleteFile(const std::string& path) {
		try {
			return fs::remove(path);
		}
		catch (...) {
			return false;
		}
	}

	std::string ReadTextFile(const std::string& path) {
		std::ifstream file(path);
		if (!file.is_open()) {
			std::cerr << "[FileSystem] Failed to open: " << path << std::endl;
			return "";
		}
		return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	}

	std::vector<uint8_t> ReadBinaryFile(const std::string& path) {
		std::ifstream file(path, std::ios::binary);
		if (!file.is_open()) {
			std::cerr << "[FileSystem] Failed to open binary: " << path << std::endl;
			return {};
		}

		file.seekg(0, std::ios::end);
		size_t size = file.tellg();
		file.seekg(0, std::ios::beg);

		std::vector<uint8_t> buffer(size);
		file.read(reinterpret_cast<char*>(buffer.data()), size);
		return buffer;
	}

	bool WriteTextFile(const std::string& path, const std::string& data) {
		std::ofstream file(path);
		if (!file.is_open()) {
			std::cerr << "[FileSystem] Failed to write text: " << path << std::endl;
			return false;
		}
		file << data;
		return true;
	}

	bool WriteBinaryFile(const std::string& path, const std::vector<uint8_t>& data) {
		std::ofstream file(path, std::ios::binary);
		if (!file.is_open()) {
			std::cerr << "[FileSystem] Failed to write binary: " << path << std::endl;
			return false;
		}
		file.write(reinterpret_cast<const char*>(data.data()), data.size());
		return true;
	}

	std::string GetFileName(const std::string& path) {
		return fs::path(path).filename().string();
	}

	std::string GetExtension(const std::string& path) {
		return fs::path(path).extension().string();
	}

	std::string GetParentPath(const std::string& path) {
		return fs::path(path).parent_path().string();
	}

	std::vector<std::string> ListFiles(const std::string& directory, const std::string& extension) {
		std::vector<std::string> result;
		if (!fs::exists(directory))
			return result;

		for (auto& entry : fs::directory_iterator(directory)) {
			if (entry.is_regular_file()) {
				if (extension.empty() || entry.path().extension() == extension)
					result.push_back(entry.path().string());
			}
		}
		return result;
	}

#if defined(_WIN32)

	std::string WideToString(const std::wstring& wstr) {
		if (wstr.empty()) return "";

		int size_needed = WideCharToMultiByte(
			CP_UTF8, 0,
			wstr.c_str(), -1,
			NULL, 0,
			NULL, NULL);

		std::string result(size_needed, 0);

		WideCharToMultiByte(
			CP_UTF8, 0,
			wstr.c_str(), -1,
			result.data(), size_needed,
			NULL, NULL);

		return result;
	}

#else

	std::string WideToString(const std::wstring&) {
		return "";
	}

#endif

	std::string OpenFile() {
#if defined(_WIN32)

		wchar_t fileName[MAX_PATH] = L"";
		OPENFILENAMEW ofn{};
		ofn.lStructSize = sizeof(ofn);
		ofn.lpstrFilter = L"All Files\0*.*\0Text Files\0*.TXT\0";
		ofn.lpstrFile = fileName;
		ofn.nMaxFile = MAX_PATH;
		ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

		if (GetOpenFileNameW(&ofn)) {
			int size_needed = WideCharToMultiByte(CP_UTF8, 0,
				fileName, -1, NULL, 0, NULL, NULL);

			std::string result(size_needed, 0);

			WideCharToMultiByte(CP_UTF8, 0,
				fileName, -1, result.data(), size_needed, NULL, NULL);

			return result;
		}

		return "";

#elif defined(__linux__)

		// Requires zenity installed
		FILE* pipe = popen("zenity --file-selection 2>/dev/null", "r");
		if (!pipe) return "";

		char buffer[1024];
		std::string result;

		if (fgets(buffer, sizeof(buffer), pipe))
			result = buffer;

		pclose(pipe);

		// Remove trailing newline
		if (!result.empty() && result.back() == '\n')
			result.pop_back();

		return result;

#else

		return "";

#endif
	}

}
