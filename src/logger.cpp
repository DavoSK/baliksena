#include "logger.hpp"
#include <spdlog/sinks/base_sink.h>
#include <spdlog/details/null_mutex.h>
#include <memory>

std::unique_ptr<spdlog::logger> gLogger = nullptr;

spdlog::logger& Logger::get() {
    if(gLogger == nullptr) {
        gLogger = std::make_unique<spdlog::logger>("senko");
    }

    return *gLogger.get();
}