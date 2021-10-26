#include "vfs.hpp"
#include "mafia/parser_dta.hpp"
#include "logger.hpp"

#include <filesystem>
#include <map>

struct DtaFileEntry 
{
    std::shared_ptr<std::ifstream> file;
    std::shared_ptr<MFFormat::DataFormatDTA> parser;
    unsigned int fleIdx;
};

struct DTAFile
{
    std::string fileName;  
    uint32_t fileKey1;
    uint32_t fileKey2; 
};

static std::unordered_map<size_t, DtaFileEntry> gFileMap;
static std::vector<std::string> gMissionsList;

std::vector<DTAFile> mFilesToFetchVer2 = {
    { "a8.dta", 0x6A63FA71, 0x0EC45D8CE },
    //{ "b8.dta", 0x79E21CDB, 0x0B60F823F },
    { "A2.dta", 0x1417D340, 0x0B6399E19 },
    { "A6.dta", 0x728E2DB9, 0x5055DA68 },
    { "A0.dta", 0x7F3D9B74, 0x0EC48FE17 },
    { "A1.dta", 0x0E7375F59, 0x900210E },
    { "A3.dta", 0x0A94B8D3C, 0x771F3888 },
    { "AC.dta", 0x0A94B8D3C, 0x771F3888 },
    { "A4.dta", 0x0A94B8D3C, 0x771F3888 },
    { "AA.dta", 0x0D4AD90C6, 0x67DA216E },
    { "A5.dta", 0x4F4BB0C6, 0x0EA340420 },
    { "A7.dta", 0x0F4F03A72, 0x0E266FE62 },
    { "A9.dta", 0x959D1117, 0x5B763446 },
    { "AB.dta", 0x7F3D9B74, 0x0EC48FE17 }
};

std::vector<std::string>& Vfs::getMissionsList() {
    return gMissionsList;
}

void Vfs::init(const std::string& rootDir) {
    
    //NOTE: open file descriptor for each DTA file
    //and create mapping helper that can map to opened file descriptor based on needed filename
    //note each file will be store in unoreder map as hash
    for(const auto& dtaFile : mFilesToFetchVer2) {    
        auto filePathToOpen = rootDir + dtaFile.fileName;
        auto dtaFileSteam = std::make_shared<std::ifstream>(filePathToOpen, std::ifstream::binary);
        if(dtaFileSteam->good()) {
            auto currentDtaParser = std::make_shared<MFFormat::DataFormatDTA >();
            currentDtaParser->setDecryptKeys(dtaFile.fileKey1, dtaFile.fileKey2);
            if(currentDtaParser->load(*dtaFileSteam)) {
                for(auto i = 0; i < currentDtaParser->getNumFiles(); i++) {
                    auto fileName = MFUtil::strToLower(currentDtaParser->getFileName(i));

                    //NOTE: create list of missions
                    //missions are only in A1 dta file
                    if(dtaFile.fileName == "A1.dta") {
                        auto splitedPath = MFUtil::strSplit(fileName, '\\');
                        if(splitedPath.size() == 3) {
                            auto missionName = splitedPath[1];
                            if(std::find(gMissionsList.begin(), gMissionsList.end(), missionName) == gMissionsList.end()) {
                                gMissionsList.push_back(missionName);
                            }
                        }
                    }

                    std::hash<std::string> hasher;
                    gFileMap[hasher(fileName)] = { 
                        dtaFileSteam,
                        currentDtaParser,
                        i
                    };
                }
            }
        } else {
            Logger::get().error("unable to open DTA: {}", filePathToOpen);
        }
    }

    //NOTE: sort mission list alphabetically
    auto cmpFunc = [](std::string a, std::string b) { return a < b; };
    std::sort(gMissionsList.begin(),gMissionsList.end(), cmpFunc);

    Logger::get().info("VFS mounted");
}

std::optional<MFUtil::ScopedBuffer> Vfs::getFile(const std::string& filePath) {
    std::hash<std::string> hasher;
    auto lowerFilePath = MFUtil::strToLower(filePath);
    const auto fileHash = hasher(lowerFilePath);

    if(gFileMap.find(fileHash) != gFileMap.end()) {
        DtaFileEntry& entry = gFileMap[fileHash];
        return entry.parser->getFile(*entry.file, entry.fleIdx);
    }

    Logger::get().error("VFS unable to get file {}", filePath);
    return {};
}

void Vfs::destroy() {
    gFileMap.clear();
}