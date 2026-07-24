#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#include "../include/audioLSB.h"
#include "../include/stegcommon.h"

// Headless port of the CLI audio.cpp algorithm. The on-carrier format is kept
// identical to the original tool:
//   full_data = [u64 header_size][header]
//   header    = [u32 filename_len][filename][u64 secret_size][secret]
//   header is optionally Vigenere-encrypted; bit positions inside the WAV data
//   chunk are optionally permuted with an mt19937 seeded from the password.
namespace {

void vigenereCipher(std::vector<uint8_t> &data, const std::string &key,
                    bool encrypt) {
  if (key.empty())
    return;
  size_t keyLen = key.size();
  for (size_t i = 0; i < data.size(); ++i) {
    uint8_t k = static_cast<uint8_t>(key[i % keyLen]);
    data[i] = encrypt ? (data[i] + k) % 256 : (data[i] - k + 256) % 256;
  }
}

std::vector<size_t> generateIndices(size_t dataSize, unsigned int seed,
                                    bool randomize) {
  std::vector<size_t> indices(dataSize);
  std::iota(indices.begin(), indices.end(), 0);
  if (randomize) {
    std::mt19937 rng(seed);
    std::shuffle(indices.begin(), indices.end(), rng);
  }
  return indices;
}

// Locate the "data" chunk inside a WAV byte buffer; returns offset of the first
// sample byte and sets dataSize to the chunk length.
size_t findDataChunk(const std::vector<uint8_t> &wav, size_t &dataSize) {
  if (wav.size() < 12)
    throw std::runtime_error("Not a valid WAV file");
  size_t pos = 12;
  while (pos + 8 <= wav.size()) {
    uint32_t chunkId, chunkSize;
    std::memcpy(&chunkId, &wav[pos], 4);
    std::memcpy(&chunkSize, &wav[pos + 4], 4);
    if (chunkId == 0x61746164) { // "data" little-endian
      dataSize = chunkSize;
      if (pos + 8 + dataSize > wav.size())
        dataSize = wav.size() - (pos + 8);
      return pos + 8;
    }
    pos += 8 + chunkSize;
  }
  throw std::runtime_error("Could not find WAV data chunk");
}

std::string errJson(const std::string &msg) {
  return "{\"status\":\"error\",\"message\":\"" + stegcommon::jsonEscape(msg) +
         "\",\"data\":{}}";
}

} // namespace

std::tuple<std::string, int> audioLSBEmbed(const std::string &fileId,
                                           const std::string &coverFilename,
                                           const std::string &secretFilename,
                                           const std::string &password,
                                           bool encrypt, bool randomize) {
  try {
    std::vector<uint8_t> wav =
        stegcommon::readAllBytes("/app/uploads/" + fileId);
    size_t dataSize = 0;
    size_t dataStart = findDataChunk(wav, dataSize);

    std::vector<uint8_t> originalData(wav.begin() + dataStart,
                                      wav.begin() + dataStart + dataSize);

    std::vector<uint8_t> secret =
        stegcommon::readAllBytes("/app/secrets/" + fileId);
    std::string filename = stegcommon::baseName(secretFilename);

    std::vector<uint8_t> header;
    uint32_t filenameLen = static_cast<uint32_t>(filename.size());
    header.insert(header.end(), reinterpret_cast<uint8_t *>(&filenameLen),
                  reinterpret_cast<uint8_t *>(&filenameLen) + 4);
    header.insert(header.end(), filename.begin(), filename.end());
    uint64_t secretSize = static_cast<uint64_t>(secret.size());
    header.insert(header.end(), reinterpret_cast<uint8_t *>(&secretSize),
                  reinterpret_cast<uint8_t *>(&secretSize) + 8);
    header.insert(header.end(), secret.begin(), secret.end());

    if (encrypt)
      vigenereCipher(header, password, true);

    std::vector<uint8_t> fullData;
    uint64_t headerSize = static_cast<uint64_t>(header.size());
    fullData.insert(fullData.end(), reinterpret_cast<uint8_t *>(&headerSize),
                    reinterpret_cast<uint8_t *>(&headerSize) + 8);
    fullData.insert(fullData.end(), header.begin(), header.end());

    size_t requiredBits = fullData.size() * 8;
    if (requiredBits > dataSize) {
      return std::make_tuple(
          "{\"status\":\"error\",\"message\":\"Insufficient capacity: " +
              std::to_string(requiredBits / 8) + " bytes required, " +
              std::to_string(dataSize / 8) +
              " available\",\"data\":{\"maxCapacity\":\"" +
              std::to_string(dataSize / 8) + "\"}}",
          400);
    }

    unsigned int seed = stegcommon::seedFromPassword(password);
    std::vector<size_t> indices = generateIndices(dataSize, seed, randomize);

    for (size_t i = 0; i < fullData.size(); ++i) {
      uint8_t byte = fullData[i];
      for (int bit = 7; bit >= 0; --bit) {
        size_t idx = i * 8 + (7 - bit);
        size_t pos = dataStart + indices[idx];
        wav[pos] = (wav[pos] & 0xFE) | ((byte >> bit) & 1);
      }
    }

    // PSNR of the modified audio (mirrors the CLI tool's metric).
    double mse = 0.0;
    for (size_t i = 0; i < dataSize; ++i) {
      double d = static_cast<double>(originalData[i]) - wav[dataStart + i];
      mse += d * d;
    }
    mse /= dataSize;
    std::string psnrStr =
        (mse == 0.0) ? "Infinite"
                     : std::to_string(10.0 * std::log10((255.0 * 255.0) / mse));

    stegcommon::writeAllBytes("/app/results/" + fileId, wav);

    return std::make_tuple(
        "{\"status\":\"success\",\"message\":\"Data embedded successfully\","
        "\"data\":{\"result\":\"/results/" +
            fileId + "\",\"originalFilename\":\"" +
            stegcommon::jsonEscape(coverFilename) + "\",\"psnr\":\"" + psnrStr +
            "\"}}",
        200);
  } catch (const std::exception &e) {
    return std::make_tuple(errJson(e.what()), 400);
  }
}

std::tuple<std::string, int> audioLSBExtract(const std::string &fileId,
                                             const std::string &password,
                                             bool encrypt, bool randomize) {
  try {
    std::vector<uint8_t> wav =
        stegcommon::readAllBytes("/app/uploads/" + fileId);
    size_t dataSize = 0;
    size_t dataStart = findDataChunk(wav, dataSize);

    unsigned int seed = stegcommon::seedFromPassword(password);
    std::vector<size_t> indices = generateIndices(dataSize, seed, randomize);

    auto readBytes = [&](size_t byteOffset, size_t n) {
      std::vector<uint8_t> out(n, 0);
      for (size_t i = 0; i < n; ++i) {
        uint8_t byte = 0;
        for (int bit = 7; bit >= 0; --bit) {
          size_t idx = (byteOffset + i) * 8 + (7 - bit);
          if (idx >= indices.size())
            throw std::runtime_error("Unexpected end of audio data");
          size_t pos = dataStart + indices[idx];
          byte |= (wav[pos] & 1) << bit;
        }
        out[i] = byte;
      }
      return out;
    };

    std::vector<uint8_t> headerSizeBytes = readBytes(0, 8);
    uint64_t headerSize = 0;
    std::memcpy(&headerSize, headerSizeBytes.data(), 8);
    if (headerSize == 0 || headerSize > dataSize)
      throw std::runtime_error(
          "Invalid header size (wrong options or not a stego audio file)");

    std::vector<uint8_t> header = readBytes(8, static_cast<size_t>(headerSize));
    if (encrypt)
      vigenereCipher(header, password, false);

    if (header.size() < 4)
      throw std::runtime_error("Corrupt header");
    uint32_t filenameLen = 0;
    std::memcpy(&filenameLen, header.data(), 4);
    if (filenameLen == 0 || filenameLen > 4096 ||
        4 + filenameLen + 8 > header.size())
      throw std::runtime_error(
          "Corrupt filename (wrong password or not a stego audio file)");

    std::string filename(header.begin() + 4, header.begin() + 4 + filenameLen);
    uint64_t secretSize = 0;
    std::memcpy(&secretSize, header.data() + 4 + filenameLen, 8);

    size_t secretStart = 4 + filenameLen + 8;
    if (secretStart + secretSize > header.size())
      throw std::runtime_error("Secret data truncated (wrong password/options)");

    std::vector<uint8_t> secret(header.begin() + secretStart,
                                header.begin() + secretStart + secretSize);

    stegcommon::writeAllBytes("/app/extracts/" + fileId, secret);

    return std::make_tuple(
        "{\"status\":\"success\",\"message\":\"Data extracted successfully\","
        "\"data\":{\"result\":\"/extracts/" +
            fileId + "\",\"originalFilename\":\"" +
            stegcommon::jsonEscape(filename) + "\"}}",
        200);
  } catch (const std::exception &e) {
    return std::make_tuple(errJson(e.what()), 400);
  }
}
