#pragma once
#include <memory>
#include <string>

class Frame;
class ModelLoader {
public: 
	static std::shared_ptr<Frame> loadModel(const std::string& path);
};