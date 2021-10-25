#include "logger.hpp"
#include "gui.hpp"

#include <spdlog/sinks/base_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/details/null_mutex.h>
#include <memory>

std::unique_ptr<spdlog::logger> gLogger = nullptr;

namespace spdlog::sinks {
    template<typename Mutex>
    class gui_sink final : public base_sink<Mutex> {
    public:
        // Formatting codes
        const string_view_t reset = "\033[m";
        const string_view_t bold = "\033[1m";
        const string_view_t dark = "\033[2m";
        const string_view_t underline = "\033[4m";
        const string_view_t blink = "\033[5m";
        const string_view_t reverse = "\033[7m";
        const string_view_t concealed = "\033[8m";
        const string_view_t clear_line = "\033[K";

        // Foreground colors
        const string_view_t black = "\033[30m";
        const string_view_t red = "\033[31m";
        const string_view_t green = "\033[32m";
        const string_view_t yellow = "\033[33m";
        const string_view_t blue = "\033[34m";
        const string_view_t magenta = "\033[35m";
        const string_view_t cyan = "\033[36m";
        const string_view_t white = "\033[37m";

        // Background colors
        const string_view_t on_black = "\033[40m";
        const string_view_t on_red = "\033[41m";
        const string_view_t on_green = "\033[42m";
        const string_view_t on_yellow = "\033[43m";
        const string_view_t on_blue = "\033[44m";
        const string_view_t on_magenta = "\033[45m";
        const string_view_t on_cyan = "\033[46m";
        const string_view_t on_white = "\033[47m";

        /// Bold colors
        const string_view_t yellow_bold = "\033[33m\033[1m";
        const string_view_t red_bold = "\033[31m\033[1m";
        const string_view_t bold_on_red = "\033[1m\033[41m";

        gui_sink() {
            colors_[level::trace] = to_string_(white);
            colors_[level::debug] = to_string_(cyan);
            colors_[level::info] = to_string_(green);
            colors_[level::warn] = to_string_(yellow_bold);
            colors_[level::err] = to_string_(red_bold);
            colors_[level::critical] = to_string_(bold_on_red);
            colors_[level::off] = to_string_(reset);
        }
        gui_sink(const gui_sink &) = delete;
        gui_sink &operator=(const gui_sink &) = delete;
    protected:
        void sink_it_(const details::log_msg &msg) override {
            memory_buf_t formatted;
            base_sink<Mutex>::formatter_->format(msg, formatted);

            std::string outString;
            outString += std::string(formatted.data(), msg.color_range_start);
            outString += colors_[msg.level];
            outString += std::string(formatted.data() + msg.color_range_start, msg.color_range_end - msg.color_range_start);
            outString += reset;
            outString += std::string(formatted.data() + msg.color_range_end, formatted.size() - msg.color_range_end);
            Gui::addLogMessage((uint32_t)msg.level, outString);
        }

        void flush_() override {
        }

        std::string to_string_(std::string_view view) {
            return std::string(view.data(), view.size());
        }
    private:
        std::array<std::string, level::n_levels> colors_;
    };

    using gui_sink_mt = gui_sink<std::mutex>;
    using gui_sink_st = gui_sink<details::null_mutex>;
};

spdlog::logger& Logger::get() {
    if(gLogger == nullptr) {
        gLogger = std::make_unique<spdlog::logger>("senko");

        auto& sinks = gLogger->sinks();
        sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_st>());
        sinks.push_back(std::make_shared<spdlog::sinks::gui_sink_st>());
    }

    return *gLogger.get();
}
