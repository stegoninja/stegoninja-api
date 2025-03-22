#include "../include/convertToBMP.h"
#define STB_IMAGE_IMPLEMENTATION
#include "../include/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../include/stb_image_write.h"
#include <vector>
#include <iostream>

bool convertToBMP(const char* inputPath, const char* outputPath) {
    // Step 1: Load the image using stb_image
    int width, height, channels;
    unsigned char* imageData = stbi_load(inputPath, &width, &height, &channels, 0);
    if (!imageData) {
        std::cerr << "Error: Failed to load image: " << inputPath << std::endl;
        return false;
    }

    // Step 2: Save the image as BMP using stb_image_write
    // BMP format requires 3 channels (RGB). If the image has 4 channels (RGBA), we need to convert it.
    std::vector<unsigned char> rgbData;
    if (channels == 4) {
        // Convert RGBA to RGB by removing the alpha channel
        rgbData.resize(width * height * 3);
        for (int i = 0; i < width * height; ++i) {
            rgbData[i * 3 + 0] = imageData[i * 4 + 0]; // R
            rgbData[i * 3 + 1] = imageData[i * 4 + 1]; // G
            rgbData[i * 3 + 2] = imageData[i * 4 + 2]; // B
        }
        imageData = rgbData.data();
        channels = 3;
    }

    // Write the image to BMP format
    int success = stbi_write_bmp(outputPath, width, height, channels, imageData);
    // stbi_image_free(imageData);
    if (!success) {
        std::cerr << "Error: Failed to write BMP file: " << outputPath << std::endl;
        return false;
    } else {
        std::cout << "Successfully converted image to BMP: " << outputPath << std::endl;
        return true;
    }
}