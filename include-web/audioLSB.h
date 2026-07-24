#ifndef AUDIO_LSB_H
#define AUDIO_LSB_H

#include <string>
#include <tuple>

// Audio LSB steganography over the PCM samples of a WAV file.
//   cover  = /app/uploads/<fileId>   (embed, WAV)
//   secret = /app/secrets/<fileId>   (embed)
//   output = /app/results/<fileId>   (embed, WAV)
//   stego  = /app/uploads/<fileId>   (extract, WAV)
//   extract= /app/extracts/<fileId>  (extract)
std::tuple<std::string, int> audioLSBEmbed(const std::string &fileId,
                                           const std::string &coverFilename,
                                           const std::string &secretFilename,
                                           const std::string &password,
                                           bool encrypt, bool randomize);

std::tuple<std::string, int> audioLSBExtract(const std::string &fileId,
                                             const std::string &password,
                                             bool encrypt, bool randomize);

#endif // AUDIO_LSB_H
