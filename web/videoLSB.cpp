#include <algorithm>
#include <cstring>
#include <filesystem>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#include <opencv2/opencv.hpp>

#include "../include/stegcommon.h"
#include "../include/videoLSB.h"

// Sequential-frame LSB. Bits of the shared "frame" payload (see stegcommon.h)
// are written MSB-first across the video's frames. Within each frame, a "slot"
// is one colour-channel LSB: slot = (y*width + x)*3 + channel. When `randomize`
// is set the per-frame slot order is permuted with an mt19937 seeded from the
// password (the same permutation is regenerated on extraction).
namespace {

std::vector<size_t> frameSlotOrder(size_t slotsPerFrame,
                                   const std::string &password, bool randomize) {
  std::vector<size_t> order(slotsPerFrame);
  std::iota(order.begin(), order.end(), 0);
  if (randomize) {
    std::mt19937 rng(stegcommon::seedFromPassword(password));
    std::shuffle(order.begin(), order.end(), rng);
  }
  return order;
}

std::string errJson(const std::string &msg) {
  return "{\"status\":\"error\",\"message\":\"" + stegcommon::jsonEscape(msg) +
         "\",\"data\":{}}";
}

std::vector<cv::Mat> readAllFrames(const std::string &path, double &fps,
                                   int &width, int &height) {
  cv::VideoCapture cap(path);
  if (!cap.isOpened())
    throw std::runtime_error("Failed to open video (unsupported format?)");

  fps = cap.get(cv::CAP_PROP_FPS);
  if (fps <= 0.0)
    fps = 25.0;

  std::vector<cv::Mat> frames;
  cv::Mat frame;
  while (cap.read(frame)) {
    if (frame.empty())
      break;
    if (frame.type() != CV_8UC3)
      cv::cvtColor(frame, frame, cv::COLOR_GRAY2BGR);
    frames.push_back(frame.clone());
  }
  cap.release();
  if (frames.empty())
    throw std::runtime_error("Video contains no readable frames");
  width = frames[0].cols;
  height = frames[0].rows;
  return frames;
}

} // namespace

std::tuple<std::string, int> videoLSBEmbed(const std::string &fileId,
                                           const std::string &coverFilename,
                                           const std::string &secretFilename,
                                           const std::string &password,
                                           bool encrypt, bool randomize) {
  try {
    std::string inputPath = "/app/uploads/" + fileId;
    double fps = 25.0;
    int width = 0, height = 0;
    std::vector<cv::Mat> frames = readAllFrames(inputPath, fps, width, height);

    std::vector<uint8_t> secret =
        stegcommon::readAllBytes("/app/secrets/" + fileId);
    std::vector<uint8_t> payload =
        stegcommon::buildFrame(secretFilename, secret, password, encrypt);

    size_t slotsPerFrame = static_cast<size_t>(width) * height * 3;
    size_t totalBits = payload.size() * 8;
    size_t capacityBits = slotsPerFrame * frames.size();
    if (totalBits > capacityBits) {
      return std::make_tuple(
          "{\"status\":\"error\",\"message\":\"Secret data too large. Maximum "
          "capacity: " +
              std::to_string(capacityBits / 8) +
              " bytes\",\"data\":{\"maxCapacity\":\"" +
              std::to_string(capacityBits / 8) + "\"}}",
          400);
    }

    std::vector<size_t> order = frameSlotOrder(slotsPerFrame, password, randomize);

    for (size_t k = 0; k < totalBits; ++k) {
      uint8_t bit = (payload[k / 8] >> (7 - (k % 8))) & 1;
      size_t f = k / slotsPerFrame;
      size_t slot = order[k % slotsPerFrame];
      size_t pixel = slot / 3;
      int channel = static_cast<int>(slot % 3);
      int y = static_cast<int>(pixel / width);
      int x = static_cast<int>(pixel % width);
      uint8_t &ref = frames[f].at<cv::Vec3b>(y, x)[channel];
      ref = (ref & 0xFE) | bit;
    }

    std::string aviPath = "/app/results/" + fileId + ".avi";
    int codec = cv::VideoWriter::fourcc('F', 'F', 'V', '1');
    cv::VideoWriter writer(aviPath, codec, fps, cv::Size(width, height), true);
    if (!writer.isOpened())
      throw std::runtime_error("Failed to open FFV1 video writer");
    for (const auto &f : frames)
      writer.write(f);
    writer.release();

    // Serve without the extension (matches the /results/<fileId> route).
    std::string outputPath = "/app/results/" + fileId;
    std::error_code ec;
    std::filesystem::rename(aviPath, outputPath, ec);
    if (ec)
      std::filesystem::copy_file(
          aviPath, outputPath,
          std::filesystem::copy_options::overwrite_existing, ec);

    return std::make_tuple(
        "{\"status\":\"success\",\"message\":\"Data embedded successfully\","
        "\"data\":{\"result\":\"/results/" +
            fileId + "\",\"originalFilename\":\"" +
            stegcommon::jsonEscape(coverFilename) +
            "\",\"format\":\"FFV1/AVI\",\"frames\":\"" +
            std::to_string(frames.size()) + "\"}}",
        200);
  } catch (const std::exception &e) {
    return std::make_tuple(errJson(e.what()), 400);
  }
}

std::tuple<std::string, int> videoLSBExtract(const std::string &fileId,
                                             const std::string &password,
                                             bool encrypt, bool randomize) {
  try {
    // The handler stores the uploaded stego with a .avi extension so OpenCV's
    // muxer reliably recognises the FFV1 stream.
    std::string inputPath = "/app/uploads/" + fileId + ".avi";
    double fps = 25.0;
    int width = 0, height = 0;
    std::vector<cv::Mat> frames = readAllFrames(inputPath, fps, width, height);

    size_t slotsPerFrame = static_cast<size_t>(width) * height * 3;
    size_t capacityBits = slotsPerFrame * frames.size();
    std::vector<size_t> order = frameSlotOrder(slotsPerFrame, password, randomize);

    size_t bitPos = 0;
    auto readBytes = [&](size_t n) {
      std::vector<uint8_t> out(n, 0);
      for (size_t i = 0; i < n; ++i) {
        uint8_t byte = 0;
        for (int b = 0; b < 8; ++b) {
          if (bitPos >= capacityBits)
            throw std::runtime_error("Unexpected end of video data");
          size_t f = bitPos / slotsPerFrame;
          size_t slot = order[bitPos % slotsPerFrame];
          size_t pixel = slot / 3;
          int channel = static_cast<int>(slot % 3);
          int y = static_cast<int>(pixel / width);
          int x = static_cast<int>(pixel % width);
          uint8_t bit = frames[f].at<cv::Vec3b>(y, x)[channel] & 1;
          byte |= (bit << (7 - b));
          ++bitPos;
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
          "Invalid payload length (wrong options or not a stego video)");

    std::vector<uint8_t> payload = readBytes(static_cast<size_t>(payloadLen));

    std::string filename;
    std::vector<uint8_t> secret;
    stegcommon::parsePayload(payload, password, encrypt, filename, secret);

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
