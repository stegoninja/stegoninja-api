#ifndef VIGENERE_H
#define VIGENERE_H

#include <string>
#include <vector>

namespace Vigenere {
std::string vigenere_encrypt(const std::string &text, const std::string &key);
std::string vigenere_decrypt(const std::string &text, const std::string &key);

std::vector<unsigned char>
vigenereEncrypt(const std::vector<unsigned char> &data, const std::string &key);

std::vector<unsigned char>
vigenereDecrypt(const std::vector<unsigned char> &data, const std::string &key);

} // namespace Vigenere

#endif // VIGENERE_H
