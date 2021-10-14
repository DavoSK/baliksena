#pragma once
#include <sstream>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

class Logger {
public:
    static spdlog::logger& get();
};