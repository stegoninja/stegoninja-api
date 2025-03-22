#include "vigenere.h"

std::string Vigenere::vigenere_encrypt(const std::string &text,
                                       const std::string &key) {
  std::string result = text;
  size_t keyLen = key.length();
  for (size_t i = 0; i < text.length(); ++i) {
    result[i] = text[i] + key[i % keyLen];
  }
  return result;
}

std::string Vigenere::vigenere_decrypt(const std::string &text,
                                       const std::string &key) {
  std::string result = text;
  size_t keyLen = key.length();
  for (size_t i = 0; i < text.length(); ++i) {
    result[i] = text[i] - key[i % keyLen];
  }
  return result;
}

// Extended Vigenère Cipher
std::vector<unsigned char>
Vigenere::vigenereEncrypt(const std::vector<unsigned char> &data,
                          const std::string &key) {
  std::vector<unsigned char> cipherText(data.size());
  size_t keyLength = key.size();

  for (size_t i = 0; i < data.size(); ++i) {
    cipherText[i] =
        (data[i] + static_cast<unsigned char>(key[i % keyLength])) % 256;
  }

  return cipherText;
}

std::vector<unsigned char>
Vigenere::vigenereDecrypt(const std::vector<unsigned char> &data,
                          const std::string &key) {

  std::vector<unsigned char> plainText(data.size());
  size_t keyLength = key.size();

  for (size_t i = 0; i < data.size(); ++i) {
    plainText[i] =
        (data[i] - static_cast<unsigned char>(key[i % keyLength]) + 256) % 256;
  }

  return plainText;
}
