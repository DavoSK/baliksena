#pragma once
#include <string>
#include <cstdint>

constexpr uint32_t LOGGER_ALL_INDEX = 7;

class Scene;
class Gui {
public:
	static void init();
	static void render();
	static void debugRender(Scene* scene);
	static void addLogMessage(uint32_t logLevel, const std::string& msg);
};
