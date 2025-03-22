#ifndef PARSE_MULTIPART_H
#define PARSE_MULTIPART_H

#include <vector>
#include <string>
#include <utility>
#include <optional>

std::vector<std::pair<std::string, std::string>> parse_multipart(
    const std::string& body,
    const std::string& boundary
);

std::optional<std::pair<std::string, std::string>> get_file_by_name(
    const std::string& body,
    const std::string& boundary,
    const std::string& target_name
);

#endif