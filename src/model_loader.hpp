#pragma once
#include <memory>
#include <string>

class Model;
class ModelLoader {
public: 
	static std::shared_ptr<Model> loadModel(const std::string& path);
};