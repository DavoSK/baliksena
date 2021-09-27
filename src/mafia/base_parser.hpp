#ifndef FORMAT_PARSERS_H
#define FORMAT_PARSERS_H

#include "math.hpp"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace MFFormat {

/// Abstract class representing a game data format.
class DataFormat {
public:
    virtual bool load(std::ifstream& srcFile) = 0;
    //virtual bool save(std::ofstream& dstFile) { return false; /* optional */ };
    virtual std::string getErrorStr() { return "Unknown error"; };
protected:
    template <typename T>
    void read(std::ifstream& stream, T* a, size_t size = sizeof(T)) {
        stream.read((char*)a, size);
    }

    std::streamsize fileLength(std::ifstream& f);
    uint32_t mErrorCode = 0;
};

}  // namespace MFFormat

#endif