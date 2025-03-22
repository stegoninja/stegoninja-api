#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <algorithm>
#include <filesystem>

#include "../include/BMPstruct.h"

std::vector<RGB> readBMP(const std::string& filename, int& width, int& height) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) throw std::runtime_error("Failed to open BMP file");

    BMPFileHeader fileHeader;
    BMPInfoHeader infoHeader;
    
    file.read(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader));
    file.read(reinterpret_cast<char*>(&infoHeader), sizeof(infoHeader));

    if (fileHeader.fileType != 0x4D42 || infoHeader.headerSize != 40 || 
        infoHeader.bitCount != 24 || infoHeader.compression != 0) {
        throw std::runtime_error("Unsupported BMP format");
    }

    width = infoHeader.width;
    height = std::abs(infoHeader.height);
    int rowSize = (width * 3 + 3) & ~3;
    int dataSize = rowSize * height;

    std::vector<uint8_t> data(dataSize);
    file.seekg(fileHeader.offsetData);
    file.read(reinterpret_cast<char*>(data.data()), dataSize);

    std::vector<RGB> pixels(width * height);
    bool isBottomUp = infoHeader.height > 0;

    for (int y = 0; y < height; ++y) {
        int srcY = isBottomUp ? height - 1 - y : y;
        const uint8_t* row = data.data() + srcY * rowSize;
        
        for (int x = 0; x < width; ++x) {
            pixels[y * width + x].r = row[x * 3 + 2];
            pixels[y * width + x].g = row[x * 3 + 1];
            pixels[y * width + x].b = row[x * 3 + 0];
        }
    }

    return pixels;
}