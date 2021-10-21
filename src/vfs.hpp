#pragma once
#include <istream>
#include <string>
#include <iostream>
#include "mafia/utils.hpp"

class Vfs {
public:
    static void init(const std::string& rootDir);
    static MFUtil::ScopedBuffer getFile(const std::string& filePath); 
    static void destroy();
};