#include "../include/stegcommon.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <stdexcept>

namespace stegcommon {

std::vector<uint8_t> vigenere(const std::vector<uint8_t> &data,
                              const std::string &key, bool encrypt) {
  std::vector<uint8_t> result(data.size());
  for (size_t i = 0; i < data.size(); ++i) {
    uint8_t keyByte = key.empty() ? 0 : static_cast<uint8_t>(key[i % key.size()]);
    result[i] = encrypt ? (data[i] + keyByte) % 256
                        : (data[i] - keyByte + 256) % 256;
  }
  return result;
}

unsigned int seedFromPassword(const std::string &password) {
  unsigned int seed = 0;
  for (char c : password)
    seed += static_cast<unsigned int>(static_cast<unsigned char>(c));
  return seed;
}

static void appendBytes(std::vector<uint8_t> &out, const void *src, size_t n) {
  const uint8_t *p = reinterpret_cast<const uint8_t *>(src);
  out.insert(out.end(), p, p + n);
}

std::vector<uint8_t> buildFrame(const std::string &secretFilename,
                                const std::vector<uint8_t> &secret,
                                const std::string &password, bool encrypt) {
  std::string name = baseName(secretFilename);

  std::vector<uint8_t> payload;
  uint32_t fnLen = static_cast<uint32_t>(name.size());
  appendBytes(payload, &fnLen, sizeof(fnLen));
  payload.insert(payload.end(), name.begin(), name.end());
  uint64_t secretLen = static_cast<uint64_t>(secret.size());
  appendBytes(payload, &secretLen, sizeof(secretLen));
  payload.insert(payload.end(), secret.begin(), secret.end());

  if (encrypt)
    payload = vigenere(payload, password, true);

  std::vector<uint8_t> frame;
  uint64_t payloadLen = static_cast<uint64_t>(payload.size());
  appendBytes(frame, &payloadLen, sizeof(payloadLen));
  frame.insert(frame.end(), payload.begin(), payload.end());
  return frame;
}

void parsePayload(const std::vector<uint8_t> &payloadIn,
                  const std::string &password, bool encrypt,
                  std::string &outFilename, std::vector<uint8_t> &outSecret) {
  std::vector<uint8_t> payload = payloadIn;
  if (encrypt)
    payload = vigenere(payload, password, false);

  if (payload.size() < sizeof(uint32_t))
    throw std::runtime_error("Corrupt payload: missing filename length "
                             "(wrong password or not a stego file)");

  size_t pos = 0;
  uint32_t fnLen = 0;
  std::memcpy(&fnLen, &payload[pos], sizeof(fnLen));
  pos += sizeof(fnLen);

  if (fnLen == 0 || fnLen > 4096 || pos + fnLen + sizeof(uint64_t) > payload.size())
    throw std::runtime_error("Corrupt payload: invalid filename "
                             "(wrong password or not a stego file)");

  outFilename.assign(payload.begin() + pos, payload.begin() + pos + fnLen);
  pos += fnLen;

  uint64_t secretLen = 0;
  std::memcpy(&secretLen, &payload[pos], sizeof(secretLen));
  pos += sizeof(secretLen);

  if (pos + secretLen > payload.size())
    throw std::runtime_error("Corrupt payload: secret data truncated "
                             "(wrong password/options)");

  outSecret.assign(payload.begin() + pos, payload.begin() + pos + secretLen);
}

std::vector<uint8_t> readAllBytes(const std::string &path) {
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file)
    throw std::runtime_error("Failed to open file: " + path);
  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);
  std::vector<uint8_t> buffer(static_cast<size_t>(size));
  if (size > 0 && !file.read(reinterpret_cast<char *>(buffer.data()), size))
    throw std::runtime_error("Failed to read file: " + path);
  return buffer;
}

void writeAllBytes(const std::string &path, const std::vector<uint8_t> &data) {
  std::ofstream file(path, std::ios::binary);
  if (!file)
    throw std::runtime_error("Failed to write file: " + path);
  if (!data.empty())
    file.write(reinterpret_cast<const char *>(data.data()), data.size());
}

std::string baseName(const std::string &path) {
  size_t slash = path.find_last_of("/\\");
  std::string name = (slash == std::string::npos) ? path : path.substr(slash + 1);
  return name.empty() ? "extracted.bin" : name;
}

std::string jsonEscape(const std::string &s) {
  std::string out;
  out.reserve(s.size() + 8);
  for (char c : s) {
    switch (c) {
    case '"': out += "\\\""; break;
    case '\\': out += "\\\\"; break;
    case '\b': out += "\\b"; break;
    case '\f': out += "\\f"; break;
    case '\n': out += "\\n"; break;
    case '\r': out += "\\r"; break;
    case '\t': out += "\\t"; break;
    default:
      if (static_cast<unsigned char>(c) < 0x20) {
        char buf[7];
        std::snprintf(buf, sizeof(buf), "\\u%04x", c & 0xff);
        out += buf;
      } else {
        out += c;
      }
    }
  }
  return out;
}

} // namespace stegcommon
