#pragma once
#include <string>

class Gui {
public:
	static void init();
	static void render();
	static void addLogMessage(const std::string& msg);
};
