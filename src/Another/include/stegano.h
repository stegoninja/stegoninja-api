#ifndef STEGANO_H
#define STEGANO_H

#include <opencv2/opencv.hpp>
#include <string>

namespace Stegano {
bool embedMessage(const std::string &inputImage, const std::string &outputImage,
                  const std::string &message);
bool embedImage(const cv::Mat &carrier, const cv::Mat &secret,
                const std::string &key, const std::string &outputPath);

bool embedImage(const cv::Mat &carrier, const cv::Mat &secret,
                const std::string &outputPath);

bool extractImage(const cv::Mat &carrier_img, const std::string &key,
                  const std::string &outputPath);

bool extractImage(const cv::Mat &carrier_img, const std::string &outputPath);

bool extractMessage(const std::string &inputImage, std::string &message);

std::vector<unsigned char> intToBytes(int value);

int bytesToInt(const std::vector<unsigned char> &bytes);

bool embedData(const cv::Mat &coverImage,
               const std::vector<unsigned char> &data, cv::Mat &stegoImage);

} // namespace Stegano

#endif // STEGANO_H
