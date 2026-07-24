#ifndef VIDEO_LSB_H
#define VIDEO_LSB_H

#include <string>
#include <tuple>

// Video LSB steganography over frame pixels (OpenCV). Output is a lossless
// FFV1-encoded AVI so the embedded LSBs survive re-encoding.
//   cover  = /app/uploads/<fileId>        (embed, any format OpenCV reads)
//   secret = /app/secrets/<fileId>        (embed)
//   output = /app/results/<fileId>        (embed, FFV1/AVI bytes)
//   stego  = /app/uploads/<fileId>.avi    (extract)
//   extract= /app/extracts/<fileId>       (extract)
std::tuple<std::string, int> videoLSBEmbed(const std::string &fileId,
                                           const std::string &coverFilename,
                                           const std::string &secretFilename,
                                           const std::string &password,
                                           bool encrypt, bool randomize);

std::tuple<std::string, int> videoLSBExtract(const std::string &fileId,
                                             const std::string &password,
                                             bool encrypt, bool randomize);

#endif // VIDEO_LSB_H
