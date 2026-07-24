#ifndef STEGCOMMON_H
#define STEGCOMMON_H

#include <cstdint>
#include <string>
#include <vector>

// Shared helpers for the LSB steganography endpoints (image + video).
// The on-carrier layout is a self-describing "frame":
//
//   frame   = [uint64 payloadLen][payload]
//   payload = optVigenere( [uint32 fnLen][filename][uint64 secretLen][secret] )
//
// `payloadLen` is always stored in the clear so the extractor knows how many
// bytes to read before attempting to decrypt/parse. Bits are written MSB-first.
namespace stegcommon {

// Byte-wise Vigenere (add mod 256 to encrypt, subtract to decrypt).
std::vector<uint8_t> vigenere(const std::vector<uint8_t> &data,
                              const std::string &key, bool encrypt);

// Deterministic seed derived from the password (sum of char codes), matching
// the scheme already used by the audio and BPCS endpoints.
unsigned int seedFromPassword(const std::string &password);

// Build the full frame (length prefix + payload) ready to be embedded.
std::vector<uint8_t> buildFrame(const std::string &secretFilename,
                                const std::vector<uint8_t> &secret,
                                const std::string &password, bool encrypt);

// Parse a payload (frame with the 8-byte length prefix already stripped) back
// into filename + secret bytes. Throws std::runtime_error on malformed data.
void parsePayload(const std::vector<uint8_t> &payload,
                  const std::string &password, bool encrypt,
                  std::string &outFilename, std::vector<uint8_t> &outSecret);

// Simple binary file IO. read throws on failure.
std::vector<uint8_t> readAllBytes(const std::string &path);
void writeAllBytes(const std::string &path, const std::vector<uint8_t> &data);

// Strip directory components from an upload filename (defends against path
// traversal in the stored secret name).
std::string baseName(const std::string &path);

// Minimal JSON string escaping for values echoed back into responses.
std::string jsonEscape(const std::string &s);

} // namespace stegcommon

#endif // STEGCOMMON_H
