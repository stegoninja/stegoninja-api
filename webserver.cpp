#include <httpserver.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <cstring>
#include "include/parse_multipart.h"
#include "include/convertToBMP.h"
#include <uuid/uuid.h>
#include "include/BMPstruct.h"
#include "include/imgBPCSEmbed.h"
#include "include/imgBPCSExtract.h"

using namespace httpserver;

class IndexFileHandler : public http_resource {
public:
    std::shared_ptr<http_response> render_GET(const http_request&) override {
        std::ifstream file("index.html", std::ios::in | std::ios::binary);
        if (!file.is_open()) {
            return std::make_shared<string_response>("File not found", 404, "text/plain");
        }
        
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        return std::make_shared<string_response>(content, 200, "text/html; charset=UTF-8");
    }
};

class ImageBPCSEmbedHandler : public http_resource {
public:
    std::shared_ptr<http_response> render_POST(const http_request& req) override {
        uuid_t uuid;
        char uuid_str[37];
        uuid_generate_random(uuid);
        uuid_unparse(uuid, uuid_str);
        std::cout << "UUID: " << uuid_str << std::endl;

        auto content_type_sv = req.get_header("Content-Type");
        std::string content_type(content_type_sv);
        size_t boundary_pos = content_type.find("boundary=");
        if (content_type.find("multipart/form-data") == std::string::npos) {
            return std::make_shared<string_response>("{\"status\":\"error\",\"message\":\"Invalid content type\",\"data\":{}}", 400, "application/json");
        }
    
        std::string encryptForm(req.get_arg_flat("password"));
        bool encrypt = encryptForm == "true" ? true : false;

        std::string randomizeForm(req.get_arg_flat("randomize"));
        bool randomize = randomizeForm == "true" ? true : false;

        auto password_raw = req.get_arg_flat("password");
        std::string password(password_raw);
        password = password.empty() ? "" : password;

        std::string boundary(content_type.substr(boundary_pos + 9));

        auto body_sv = req.get_content();
        std::string body(body_sv);

        auto cover_file = get_file_by_name(body, boundary, "cover");
        if (!cover_file.has_value()) {
            return std::make_shared<string_response>("{\"status\":\"error\",\"message\":\"No Cover Image Sent\",\"data\":{}}", 400, "application/json");
        }
        auto secret_file = get_file_by_name(body, boundary, "secret");
        if (!secret_file.has_value()) {
            return std::make_shared<string_response>("{\"status\":\"error\",\"message\":\"No Secret File Sent\",\"data\":{}}", 400, "application/json");
        }

        if ((cover_file->first).empty() || (cover_file->second).empty()) {
            return std::make_shared<string_response>("{\"status\":\"error\",\"message\":\"No Cover Image uploaded\",\"data\":{}}", 400, "application/json");
        }
        if ((secret_file->first).empty() || (secret_file->second).empty()) {
            return std::make_shared<string_response>("{\"status\":\"error\",\"message\":\"No Secret File uploaded\",\"data\":{}}", 400, "application/json");
        }

        std::ofstream cover_file_fs(std::string("/app/uploads/") + uuid_str, std::ios::binary);
        if (!cover_file_fs.is_open()) {
            return std::make_shared<string_response>("{\"status\":\"error\",\"message\":\"Failed to save Cover file\",\"data\":{}}", 400, "application/json");
        }
        cover_file_fs << cover_file->second;
        cover_file_fs.close();

        std::ofstream secret_file_fs(std::string("/app/secrets/") + uuid_str, std::ios::binary);
        if (!secret_file_fs.is_open()) {
            return std::make_shared<string_response>("{\"status\":\"error\",\"message\":\"Failed to save Secret file\",\"data\":{}}", 400, "application/json");
        }
        secret_file_fs << secret_file->second;
        secret_file_fs.close();

        if (convertToBMP(("uploads/" + std::string(uuid_str)).c_str(), ("results/" + std::string(uuid_str)).c_str())) {
            auto [message, code] = imgBPCSEmbed(std::string(uuid_str), cover_file->first, secret_file->first, password, encrypt, randomize);
            return std::make_shared<string_response>(message, code, "application/json");
            // return std::make_shared<string_response>("{\"status\":\"success\",\"message\":\"Image converted successfully\",\"data\":{\"resultId\":\"" + std::string(uuid_str) + "\",\"originalFilename\":\"" + cover_file->first + "\"}}", 200, "application/json");
        } else {
            return std::make_shared<string_response>("{\"status\":\"error\",\"message\":\"Failed to convert image to BMP!\",\"data\":{}}", 400, "application/json");
        }
    }
};

class ImageBPCSExtractHandler : public http_resource {
    public:
        std::shared_ptr<http_response> render_POST(const http_request& req) override {
            uuid_t uuid;
            char uuid_str[37];
            uuid_generate_random(uuid);
            uuid_unparse(uuid, uuid_str);
            std::cout << "UUID: " << uuid_str << std::endl;
    
            auto content_type_sv = req.get_header("Content-Type");
            std::string content_type(content_type_sv);
            size_t boundary_pos = content_type.find("boundary=");
            if (content_type.find("multipart/form-data") == std::string::npos) {
                return std::make_shared<string_response>("{\"status\":\"error\",\"message\":\"Invalid content type\",\"data\":{}}", 400, "application/json");
            }
    
            std::string encryptForm(req.get_arg_flat("password"));
            bool encrypt = encryptForm == "true" ? true : false;
    
            std::string randomizeForm(req.get_arg_flat("randomize"));
            bool randomize = randomizeForm == "true" ? true : false;

            auto password_raw = req.get_arg_flat("password");
            std::string password(password_raw);
            password = password.empty() ? "" : password;
    
            std::string boundary(content_type.substr(boundary_pos + 9));
    
            auto body_sv = req.get_content();
            std::string body(body_sv);
    
            auto stego_file = get_file_by_name(body, boundary, "stego");
            if (!stego_file.has_value()) {
                return std::make_shared<string_response>("{\"status\":\"error\",\"message\":\"No Stego Image Sent\",\"data\":{}}", 400, "application/json");
            }
    
            if ((stego_file->first).empty() || (stego_file->second).empty()) {
                return std::make_shared<string_response>("{\"status\":\"error\",\"message\":\"No Stego Image uploaded\",\"data\":{}}", 400, "application/json");
            }
    
            std::ofstream stego_file_fs(std::string("/app/uploads/") + uuid_str, std::ios::binary);
            if (!stego_file_fs.is_open()) {
                return std::make_shared<string_response>("{\"status\":\"error\",\"message\":\"Failed to save Stego file\",\"data\":{}}", 400, "application/json");
            }
            stego_file_fs << stego_file->second;
            stego_file_fs.close();
    
            auto [message, code] = imgBPCSExtract(std::string(uuid_str), password, encrypt, randomize);
            return std::make_shared<string_response>(message, code, "application/json");
        }
};

class ResultHandler : public http_resource {
    public:
        std::shared_ptr<http_response> render_GET(const http_request& req) override {
            std::string fileId(req.get_arg("fileId"));
            // if (!fileId) {
            //     return std::make_shared<string_response>("fileId not found", 404, "text/plain");
            // }

            std::ifstream file("results/" + fileId, std::ios::in | std::ios::binary);
            if (!file.is_open()) {
                return std::make_shared<string_response>("File not found", 404, "text/plain");
            }
            
            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            return std::make_shared<string_response>(content, 200, "image/bmp");
        }
};

int main() {
    webserver ws = create_webserver(8080)
        .max_threads(5)
        .content_size_limit(1024 * 1024 * 256);

    IndexFileHandler index;
    ImageBPCSEmbedHandler imgBPCSEm;
    ImageBPCSExtractHandler imgBPCSEx;
    ResultHandler results;

    ws.register_resource("/", &index);
    ws.register_resource("/image/bpcs/embed", &imgBPCSEm);
    ws.register_resource("/image/bpcs/extract", &imgBPCSEx);
    ws.register_resource("/results/{fileId}", &results);

    std::cout << "Server started on port 8080" << std::endl;
    ws.start(true);

    return 0;
}