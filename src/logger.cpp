#include "logger.hpp"
#include <spdlog/sinks/base_sink.h>
#include <spdlog/details/null_mutex.h>
#include <memory>

std::unique_ptr<spdlog::logger> gLogger = nullptr;
std::vector<std::string> gUiBuffer;

namespace spdlog::sinks {
    template<typename Mutex>
    class gui_sink final : public base_sink<Mutex> {
    public:
        gui_sink() {}
        gui_sink(const gui_sink &) = delete;
        gui_sink &operator=(const gui_sink &) = delete;
    protected:
        void sink_it_(const details::log_msg &msg) override {
            memory_buf_t formatted;
            base_sink<Mutex>::formatter_->format(msg, formatted);
            gUiBuffer.push_back(std::string(formatted.data(), formatted.size()));
        }
        void flush_() override {
            gUiBuffer.clear();
        }
    };

    using gui_sink_mt = gui_sink<std::mutex>;
    using gui_sink_st = gui_sink<details::null_mutex>;
};

spdlog::logger& Logger::get() {
    if(gLogger == nullptr) {
        auto guiSink = std::make_shared<spdlog::sinks::gui_sink_st>();
        gLogger = std::make_unique<spdlog::logger>("senko", guiSink);
    }

    return *gLogger.get();
}

const std::vector<std::string>& Logger::getGuiOutputBuffer() {
    return gUiBuffer;
}
