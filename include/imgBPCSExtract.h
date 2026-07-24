#ifndef imgBPCSExtract_H
#define imgBPCSExtract_H

#include <vector>
#include <cstdint>
#include <string>
#include <tuple>
#include <filesystem>

// std::vector<RGB> readBMP(const std::string& filename, int& width, int& height);
std::vector<uint8_t> vigenereDecrypt(const std::vector<uint8_t>& data, const std::string& key);
std::tuple<std::string, int> imgBPCSExtract(const std::string& fileId, const std::string& password, bool encrypt, bool randomize);

#endif // IMG_BPCS_EXTRACT_H