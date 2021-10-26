#pragma once
#include <istream>
#include <string>
#include <iostream>
#include <optional>
#include "mafia/utils.hpp"

class Vfs {
public:
    static void init(const std::string& rootDir);
    static std::optional<MFUtil::ScopedBuffer> getFile(const std::string& filePath);
    static std::vector<std::string>& getMissionsList(); 
    static void destroy();
};