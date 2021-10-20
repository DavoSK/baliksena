#include "logger.hpp"
#include <spdlog/sinks/base_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/details/null_mutex.h>
#include <memory>

std::unique_ptr<spdlog::logger> gLogger = nullptr;

spdlog::logger& Logger::get() {
    if(gLogger == nullptr) {
        gLogger = std::make_unique<spdlog::logger>("senko");
        auto stdOutSink = std::make_shared<spdlog::sinks::stdout_color_sink_st>();
        gLogger->sinks().push_back(stdOutSink);  
    }

    return *gLogger.get();
}