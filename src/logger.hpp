#pragma once
#include <sstream>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>


class Logger {
public:
    static spdlog::logger& get();
    static std::ostringstream& getStream();
    static const std::vector<std::string>& getGuiOutputBuffer();
};