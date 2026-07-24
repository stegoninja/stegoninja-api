#include <algorithm>
#include <cmath>
#include <cstring>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#include "../include/BMPstruct.h"
#include "../include/imgBPCSEmbed.h" // writeBMP()
#include "../include/imgLSB.h"
#include "../include/stegcommon.h"

namespace {

// A "slot" is one LSB position: slot = pixelIndex * 3 + channel (0=r,1=g,2=b).
std::vector<size_t> buildSlotOrder(size_t slotCount, const std::string &password,
                                   bool randomize) {
  std::vector<size_t> order(slotCount);
  std::iota(order.begin(), order.end(), 0);
  if (randomize) {
    std::mt19937 rng(stegcommon::seedFromPassword(password));
    std::shuffle(order.begin(), order.end(), rng);
  }
  return order;
}

uint8_t &channelRef(std::vector<RGB> &pixels, size_t slot) {
  RGB &px = pixels[slot / 3];
  switch (slot % 3) {
  case 0: return px.r;
  case 1: return px.g;
  default: return px.b;
  }
}

uint8_t channelVal(const std::vector<RGB> &pixels, size_t slot) {
  const RGB &px = pixels[slot / 3];
  switch (slot % 3) {
  case 0: return px.r;
  case 1: return px.g;
  default: return px.b;
  }
}

std::string errJson(const std::string &msg) {
  return "{\"status\":\"error\",\"message\":\"" + stegcommon::jsonEscape(msg) +
         "\",\"data\":{}}";
}

} // namespace

std::tuple<std::string, int> imgLSBEmbed(const std::string &fileId,
                                         const std::string &coverFilename,
                                         const std::string &secretFilename,
                                         const std::string &password,
                                         bool encrypt, bool randomize) {
  try {
    std::string coverFile = "/app/uploads/" + fileId + "_cover.bmp";
    std::string secretFile = "/app/secrets/" + fileId;
    std::string outputFile = "/app/results/" + fileId;

    int width = 0, height = 0;
    std::vector<RGB> originalPixels = readBMP(coverFile, width, height);
    std::vector<RGB> pixels = originalPixels;

    std::vector<uint8_t> secret = stegcommon::readAllBytes(secretFile);
    std::vector<uint8_t> frame =
        stegcommon::buildFrame(secretFilename, secret, password, encrypt);

    size_t totalBits = frame.size() * 8;
    size_t capacityBits = static_cast<size_t>(width) * height * 3;
    if (totalBits > capacityBits) {
      return std::make_tuple(
          "{\"status\":\"error\",\"message\":\"Secret data too large. Maximum "
          "capacity: " +
              std::to_string(capacityBits / 8) +
              " bytes\",\"data\":{\"maxCapacity\":\"" +
              std::to_string(capacityBits / 8) + "\"}}",
          400);
    }

    std::vector<size_t> order = buildSlotOrder(capacityBits, password, randomize);

    for (size_t k = 0; k < totalBits; ++k) {
      uint8_t bit = (frame[k / 8] >> (7 - (k % 8))) & 1;
      uint8_t &ref = channelRef(pixels, order[k]);
      ref = (ref & 0xFE) | bit;
    }

    // PSNR between original and stego.
    double sum = 0.0;
    for (size_t i = 0; i < pixels.size(); ++i) {
      double dr = originalPixels[i].r - pixels[i].r;
      double dg = originalPixels[i].g - pixels[i].g;
      double db = originalPixels[i].b - pixels[i].b;
      sum += dr * dr + dg * dg + db * db;
    }
    double mse = sum / (3.0 * width * height);
    std::string psnrStr = (mse == 0.0)
                              ? "Infinite"
                              : std::to_string(20 * std::log10(255.0 /
                                                               std::sqrt(mse)));

    writeBMP(outputFile, pixels, width, height);

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

std::tuple<std::string, int> imgLSBExtract(const std::string &fileId,
                                           const std::string &password,
                                           bool encrypt, bool randomize) {
  try {
    std::string stegoFile = "/app/uploads/" + fileId + "_stego.bmp";
    std::string extractFile = "/app/extracts/" + fileId;

    int width = 0, height = 0;
    std::vector<RGB> pixels = readBMP(stegoFile, width, height);

    size_t capacityBits = static_cast<size_t>(width) * height * 3;
    std::vector<size_t> order = buildSlotOrder(capacityBits, password, randomize);

    // Sequential bit reader over the (possibly shuffled) slot order.
    size_t bitPos = 0;
    auto readBytes = [&](size_t n) {
      std::vector<uint8_t> out(n, 0);
      for (size_t i = 0; i < n; ++i) {
        uint8_t byte = 0;
        for (int b = 0; b < 8; ++b) {
          if (bitPos >= capacityBits)
            throw std::runtime_error("Unexpected end of carrier data");
          uint8_t bit = channelVal(pixels, order[bitPos++]) & 1;
          byte |= (bit << (7 - b));
        }
        out[i] = byte;
      }
      return out;
    };

    std::vector<uint8_t> lenBytes = readBytes(sizeof(uint64_t));
    uint64_t payloadLen = 0;
    std::memcpy(&payloadLen, lenBytes.data(), sizeof(payloadLen));
    if (payloadLen == 0 || payloadLen > (capacityBits / 8))
      throw std::runtime_error(
          "Invalid payload length (wrong options or not a stego image)");

    std::vector<uint8_t> payload = readBytes(static_cast<size_t>(payloadLen));

    std::string filename;
    std::vector<uint8_t> secret;
    stegcommon::parsePayload(payload, password, encrypt, filename, secret);

    stegcommon::writeAllBytes(extractFile, secret);

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
