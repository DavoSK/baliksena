#include "vfs.hpp"
#include "mafia/parser_dta.hpp"
#include "logger.hpp"

struct DtaFileEntry 
{
    std::unique_ptr<std::ifstream> file;
    std::unique_ptr<MFFormat::DataFormatDTA> parser;
    unsigned int fleIdx;
};

struct DTAFile
{
    std::string fileName;  
    uint32_t fileKey1;
    uint32_t fileKey2; 
};

std::vector<DTAFile> mFilesToFetchVer2 =
{
    { "a8.dta", 0x6A63FA71, 0x0EC45D8CE },
    //{ "b8.dta", 0x79E21CDB, 0x0B60F823F },
    { "a2.dta", 0x1417D340, 0x0B6399E19 },
    { "a6.dta", 0x728E2DB9, 0x5055DA68 },
    { "a0.dta", 0x7F3D9B74, 0x0EC48FE17 },
    { "a1.dta", 0x0E7375F59, 0x900210E },
    { "a3.dta", 0x0A94B8D3C, 0x771F3888 },
    { "ac.dta", 0x0A94B8D3C, 0x771F3888 },
    { "a4.dta", 0x0A94B8D3C, 0x771F3888 },
    { "aa.dta", 0x0D4AD90C6, 0x67DA216E },
    { "a5.dta", 0x4F4BB0C6, 0x0EA340420 },
    { "a7.dta", 0x0F4F03A72, 0x0E266FE62 },
    { "a9.dta", 0x959D1117, 0x5B763446 },
    { "ab.dta", 0x7F3D9B74, 0x0EC48FE17 }
};

std::unordered_map<std::string, DtaFileEntry> gFileMap;

void Vfs::init() {
    for(const auto& dtaFile : mFilesToFetchVer2) {    
        auto filePathToOpen = "C:\\Mafia\\" + dtaFile.fileName;
        auto dtaFileSteam = std::make_unique<std::ifstream>(filePathToOpen, std::ifstream::binary);
        if(dtaFileSteam->good()) {
            auto currentDtaParser = std::make_unique<MFFormat::DataFormatDTA >();
            currentDtaParser->setDecryptKeys(dtaFile.fileKey1, dtaFile.fileKey2);
            if(currentDtaParser->load(*dtaFileSteam)) {
                for(unsigned int i = 0; i < currentDtaParser->getNumFiles(); i++) {
                    gFileMap[currentDtaParser->getFileName(i)] = { 
                        std::move(dtaFileSteam),
                        std::move(currentDtaParser),
                        i
                    };
                }
            }
        } else {
            Logger::get().error("unable to open DTA: {}", filePathToOpen);
        }
    }
}

memstream Vfs::getFile(const std::string& filePath) {
    if(gFileMap.find(filePath) != gFileMap.end()) {
        DtaFileEntry& entry = gFileMap[filePath];
        auto buffer = entry.parser->getFile(*entry.file, entry.fleIdx);
        return { buffer.as<uint8_t>(), buffer.size() };
    }

    return {nullptr, 0};
}

void Vfs::destroy() {
    gFileMap.clear();
}