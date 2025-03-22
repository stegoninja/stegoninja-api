#include "../include/parse_multipart.h"
#include <vector>
#include <string>
#include <utility>
#include <optional>

std::vector<std::pair<std::string, std::string>> parse_multipart(const std::string& body, const std::string& boundary) {
    std::vector<std::pair<std::string, std::string>> files; // To store multiple files

    std::string delimiter = "--" + boundary;
    size_t start = body.find(delimiter);
    size_t end = body.find(delimiter, start + delimiter.size());

    while (end != std::string::npos) {
        std::string part = body.substr(start, end - start);

        // Check if this part contains a file
        size_t disposition_pos = part.find("Content-Disposition:");
        if (disposition_pos != std::string::npos) {
            size_t filename_pos = part.find("filename=\"", disposition_pos);
            if (filename_pos != std::string::npos) {
                // Extract file name
                size_t filename_start = filename_pos + 10; // Skip "filename=\""
                size_t filename_end = part.find("\"", filename_start);
                std::string file_name = part.substr(filename_start, filename_end - filename_start);

                // Extract file content
                size_t content_start = part.find("\r\n\r\n", filename_end);
                if (content_start != std::string::npos) {
                    content_start += 4; // Skip "\r\n\r\n"
                    size_t content_end = part.size() - 2; // Exclude trailing "\r\n"
                    std::string file_content = part.substr(content_start, content_end - content_start);

                    // Store the file name and content
                    files.emplace_back(file_name, file_content);
                }
            }
        }

        // Move to the next part
        start = end;
        end = body.find(delimiter, start + delimiter.size());
    }

    return files; // Return all extracted files
}

std::optional<std::pair<std::string, std::string>> get_file_by_name(
    const std::string& body,
    const std::string& boundary,
    const std::string& target_name) {

    std::string delimiter = "--" + boundary;
    size_t start = body.find(delimiter);
    size_t end = body.find(delimiter, start + delimiter.size());

    while (end != std::string::npos) {
        std::string part = body.substr(start, end - start);

        // Check if this part contains a file
        size_t disposition_pos = part.find("Content-Disposition:");
        if (disposition_pos != std::string::npos) {
            // Extract the "name" attribute
            size_t name_pos = part.find("name=\"", disposition_pos);
            if (name_pos != std::string::npos) {
                size_t name_start = name_pos + 6; // Skip "name=\""
                size_t name_end = part.find("\"", name_start);
                std::string name = part.substr(name_start, name_end - name_start);

                // Check if the name matches the target
                if (name == target_name) {
                    // Extract file name
                    size_t filename_pos = part.find("filename=\"", name_end);
                    if (filename_pos != std::string::npos) {
                        size_t filename_start = filename_pos + 10; // Skip "filename=\""
                        size_t filename_end = part.find("\"", filename_start);
                        std::string file_name = part.substr(filename_start, filename_end - filename_start);

                        // Extract file content
                        size_t content_start = part.find("\r\n\r\n", filename_end);
                        if (content_start != std::string::npos) {
                            content_start += 4; // Skip "\r\n\r\n"
                            size_t content_end = part.size() - 2; // Exclude trailing "\r\n"
                            std::string file_content = part.substr(content_start, content_end - content_start);

                            // Return the matching file pair
                            return std::make_pair(file_name, file_content);
                        }
                    }
                }
            }
        }

        // Move to the next part
        start = end;
        end = body.find(delimiter, start + delimiter.size());
    }

    // Return an empty optional if no match is found
    return std::nullopt;
}