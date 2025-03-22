#ifndef imgBPCSEmbed_H
#define imgBPCSEmbed_H

#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <algorithm>
#include <random>
#include <filesystem>
#include <cmath>
#include <stdexcept>

// Function to read a BMP file and return pixel data
// std::vector<RGB> readBMP(const std::string& filename, int& width, int& height);

// Function to write pixel data to a BMP file
void writeBMP(const std::string& filename, const std::vector<RGB>& pixels, int width, int height);

// Function to read a secret file into a byte vector
std::vector<uint8_t> readSecretFile(const std::string& filename);

// Function to encrypt data using Vigenere cipher
std::vector<uint8_t> vigenereEncrypt(const std::vector<uint8_t>& data, const std::string& key);

// Main function to embed secret data into a BMP image
std::tuple<std::string, int> imgBPCSEmbed(const std::string& fileId, const std::string& coverFilename, const std::string& secretFilename, const std::string& password, bool encrypt, bool randomize);

#endif