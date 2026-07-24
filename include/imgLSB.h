#ifndef IMG_LSB_H
#define IMG_LSB_H

#include <string>
#include <tuple>

// Image LSB steganography (operates on 24-bit BMP via the shared readBMP/writeBMP).
// The handler is responsible for converting arbitrary uploads to BMP first.
//
//   coverBmp = /app/uploads/<fileId>_cover.bmp   (embed)
//   secret   = /app/secrets/<fileId>             (embed)
//   output   = /app/results/<fileId>             (embed, BMP)
//   stegoBmp = /app/uploads/<fileId>_stego.bmp   (extract)
//   extract  = /app/extracts/<fileId>            (extract)
std::tuple<std::string, int> imgLSBEmbed(const std::string &fileId,
                                         const std::string &coverFilename,
                                         const std::string &secretFilename,
                                         const std::string &password,
                                         bool encrypt, bool randomize);

std::tuple<std::string, int> imgLSBExtract(const std::string &fileId,
                                           const std::string &password,
                                           bool encrypt, bool randomize);

#endif // IMG_LSB_H
