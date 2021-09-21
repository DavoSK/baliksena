#pragma once
#include <fstream>
#include <stdint.h>

uint8_t* loadBMP(std::ifstream& file, int* w, int* h, bool useTransparencyKey = false);
uint8_t* loadBMPEx(const char* fileName, int* w, int* h, bool useTransparencyKey = false);