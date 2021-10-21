#pragma once
#include <fstream>
#include <stdint.h>
#include "mafia/utils.hpp"

uint8_t* loadBMP(std::istream& file, int* w, int* h, bool useTransparencyKey = false);
uint8_t* loadBMP(MFUtil::ScopedBuffer& buffer, int* w, int* h, bool useTransparencyKey = false);
uint8_t* loadBMPEx(const char* fileName, int* w, int* h, bool useTransparencyKey = false);