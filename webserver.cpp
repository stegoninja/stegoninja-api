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
#include "include/imgLSB.h"
#include "include/audioLSB.h"
#include "include/videoLSB.h"

using namespace httpserver;

// ---------------------------------------------------------------------------
// Shared helpers for the multipart-based endpoints.
// ---------------------------------------------------------------------------
static std::shared_ptr<http_response> jsonResp(const std::string &body,
                                               int code) {
    return std::make_shared<string_response>(body, code, "application/json");
}

static std::string errorJson(const std::string &msg) {
    return "{\"status\":\"error\",\"message\":\"" + msg + "\",\"data\":{}}";
}

// Common request context: request id, multipart boundary, body and the
// encrypt/randomize/password form fields.
struct MultipartCtx {
    std::string uuid;
    std::string boundary;
    std::string body;
    std::string password;
    bool encrypt = false;
    bool randomize = false;
    bool ok = false;
    std::shared_ptr<http_response> error;
};

static MultipartCtx prepareRequest(const http_request &req) {
    MultipartCtx c;

    uuid_t uuid;
    char uuid_str[37];
    uuid_generate_random(uuid);
    uuid_unparse(uuid, uuid_str);
    c.uuid = uuid_str;

    std::string content_type(req.get_header("Content-Type"));
    if (content_type.find("multipart/form-data") == std::string::npos) {
        c.error = jsonResp(errorJson("Invalid content type"), 400);
        return c;
    }
    size_t boundary_pos = content_type.find("boundary=");
    if (boundary_pos == std::string::npos) {
        c.error = jsonResp(errorJson("Missing multipart boundary"), 400);
        return c;
    }
    c.boundary = content_type.substr(boundary_pos + 9);
    c.body = std::string(req.get_content());
    c.password = std::string(req.get_arg_flat("password"));
    c.encrypt = std::string(req.get_arg_flat("encrypt")) == "true";
    c.randomize = std::string(req.get_arg_flat("randomize")) == "true";
    c.ok = true;
    return c;
}

// Extract a named file part and persist it to destPath. On failure `err` is set
// with an appropriate JSON response and false is returned.
static bool saveNamedFile(const std::string &body, const std::string &boundary,
                          const std::string &name, const std::string &destPath,
                          std::string &outFilename,
                          std::shared_ptr<http_response> &err) {
    auto file = get_file_by_name(body, boundary, name);
    if (!file.has_value() || file->first.empty() || file->second.empty()) {
        err = jsonResp(errorJson("Missing or empty file part: " + name), 400);
        return false;
    }
    std::ofstream fs(destPath, std::ios::binary);
    if (!fs.is_open()) {
        err = jsonResp(errorJson("Failed to save uploaded file: " + name), 400);
        return false;
    }
    fs << file->second;
    fs.close();
    outFilename = file->first;
    return true;
}

// API-only service info / health endpoint (no HTML UI is served).
class RootHandler : public http_resource {
public:
    std::shared_ptr<http_response> render_GET(const http_request&) override {
        const std::string body =
            "{\"status\":\"ok\",\"service\":\"stegoninja-api\","
            "\"endpoints\":["
            "\"POST /image/bpcs/embed\",\"POST /image/bpcs/extract\","
            "\"POST /image/lsb/embed\",\"POST /image/lsb/extract\","
            "\"POST /audio/lsb/embed\",\"POST /audio/lsb/extract\","
            "\"POST /video/lsb/embed\",\"POST /video/lsb/extract\","
            "\"GET /results/{fileId}\",\"GET /extracts/{fileId}\""
            "],\"spec\":\"openapi.yaml\"}";
        return jsonResp(body, 200);
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

// ---------------------------------------------------------------------------
// Image LSB
// ---------------------------------------------------------------------------
class ImageLSBEmbedHandler : public http_resource {
public:
    std::shared_ptr<http_response> render_POST(const http_request &req) override {
        MultipartCtx c = prepareRequest(req);
        if (!c.ok) return c.error;

        std::shared_ptr<http_response> err;
        std::string coverName, secretName;
        if (!saveNamedFile(c.body, c.boundary, "cover",
                           "/app/uploads/" + c.uuid, coverName, err))
            return err;
        if (!saveNamedFile(c.body, c.boundary, "secret",
                           "/app/secrets/" + c.uuid, secretName, err))
            return err;

        std::string coverBmp = "/app/uploads/" + c.uuid + "_cover.bmp";
        if (!convertToBMP(("/app/uploads/" + c.uuid).c_str(), coverBmp.c_str()))
            return jsonResp(errorJson("Failed to convert cover image to BMP"), 400);

        auto [message, code] = imgLSBEmbed(c.uuid, coverName, secretName,
                                           c.password, c.encrypt, c.randomize);
        return jsonResp(message, code);
    }
};

class ImageLSBExtractHandler : public http_resource {
public:
    std::shared_ptr<http_response> render_POST(const http_request &req) override {
        MultipartCtx c = prepareRequest(req);
        if (!c.ok) return c.error;

        std::shared_ptr<http_response> err;
        std::string stegoName;
        if (!saveNamedFile(c.body, c.boundary, "stego",
                           "/app/uploads/" + c.uuid, stegoName, err))
            return err;

        std::string stegoBmp = "/app/uploads/" + c.uuid + "_stego.bmp";
        if (!convertToBMP(("/app/uploads/" + c.uuid).c_str(), stegoBmp.c_str()))
            return jsonResp(errorJson("Failed to convert stego image to BMP"), 400);

        auto [message, code] =
            imgLSBExtract(c.uuid, c.password, c.encrypt, c.randomize);
        return jsonResp(message, code);
    }
};

// ---------------------------------------------------------------------------
// Audio LSB
// ---------------------------------------------------------------------------
class AudioLSBEmbedHandler : public http_resource {
public:
    std::shared_ptr<http_response> render_POST(const http_request &req) override {
        MultipartCtx c = prepareRequest(req);
        if (!c.ok) return c.error;

        std::shared_ptr<http_response> err;
        std::string coverName, secretName;
        if (!saveNamedFile(c.body, c.boundary, "cover",
                           "/app/uploads/" + c.uuid, coverName, err))
            return err;
        if (!saveNamedFile(c.body, c.boundary, "secret",
                           "/app/secrets/" + c.uuid, secretName, err))
            return err;

        auto [message, code] = audioLSBEmbed(c.uuid, coverName, secretName,
                                             c.password, c.encrypt, c.randomize);
        return jsonResp(message, code);
    }
};

class AudioLSBExtractHandler : public http_resource {
public:
    std::shared_ptr<http_response> render_POST(const http_request &req) override {
        MultipartCtx c = prepareRequest(req);
        if (!c.ok) return c.error;

        std::shared_ptr<http_response> err;
        std::string stegoName;
        if (!saveNamedFile(c.body, c.boundary, "stego",
                           "/app/uploads/" + c.uuid, stegoName, err))
            return err;

        auto [message, code] =
            audioLSBExtract(c.uuid, c.password, c.encrypt, c.randomize);
        return jsonResp(message, code);
    }
};

// ---------------------------------------------------------------------------
// Video LSB
// ---------------------------------------------------------------------------
class VideoLSBEmbedHandler : public http_resource {
public:
    std::shared_ptr<http_response> render_POST(const http_request &req) override {
        MultipartCtx c = prepareRequest(req);
        if (!c.ok) return c.error;

        std::shared_ptr<http_response> err;
        std::string coverName, secretName;
        if (!saveNamedFile(c.body, c.boundary, "cover",
                           "/app/uploads/" + c.uuid, coverName, err))
            return err;
        if (!saveNamedFile(c.body, c.boundary, "secret",
                           "/app/secrets/" + c.uuid, secretName, err))
            return err;

        auto [message, code] = videoLSBEmbed(c.uuid, coverName, secretName,
                                             c.password, c.encrypt, c.randomize);
        return jsonResp(message, code);
    }
};

class VideoLSBExtractHandler : public http_resource {
public:
    std::shared_ptr<http_response> render_POST(const http_request &req) override {
        MultipartCtx c = prepareRequest(req);
        if (!c.ok) return c.error;

        std::shared_ptr<http_response> err;
        std::string stegoName;
        // Store with a .avi extension so OpenCV recognises the FFV1 stream.
        if (!saveNamedFile(c.body, c.boundary, "stego",
                           "/app/uploads/" + c.uuid + ".avi", stegoName, err))
            return err;

        auto [message, code] =
            videoLSBExtract(c.uuid, c.password, c.encrypt, c.randomize);
        return jsonResp(message, code);
    }
};

class ResultHandler : public http_resource {
    public:
        std::shared_ptr<http_response> render_GET(const http_request& req) override {
            std::string fileId(req.get_arg("fileId"));

            std::ifstream file("results/" + fileId, std::ios::in | std::ios::binary);
            if (!file.is_open()) {
                return std::make_shared<string_response>("File not found", 404, "text/plain");
            }

            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            return std::make_shared<string_response>(content, 200, "application/octet-stream");
        }
};

// Download an extracted secret file by request id.
class ExtractHandler : public http_resource {
    public:
        std::shared_ptr<http_response> render_GET(const http_request& req) override {
            std::string fileId(req.get_arg("fileId"));

            std::ifstream file("extracts/" + fileId, std::ios::in | std::ios::binary);
            if (!file.is_open()) {
                return std::make_shared<string_response>("File not found", 404, "text/plain");
            }

            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            return std::make_shared<string_response>(content, 200, "application/octet-stream");
        }
};

int main() {
    webserver ws = create_webserver(8080)
        .max_threads(5)
        .content_size_limit(1024 * 1024 * 256);

    RootHandler index;
    ImageBPCSEmbedHandler imgBPCSEm;
    ImageBPCSExtractHandler imgBPCSEx;
    ImageLSBEmbedHandler imgLSBEm;
    ImageLSBExtractHandler imgLSBEx;
    AudioLSBEmbedHandler audioLSBEm;
    AudioLSBExtractHandler audioLSBEx;
    VideoLSBEmbedHandler videoLSBEm;
    VideoLSBExtractHandler videoLSBEx;
    ResultHandler results;
    ExtractHandler extracts;

    ws.register_resource("/", &index);
    ws.register_resource("/image/bpcs/embed", &imgBPCSEm);
    ws.register_resource("/image/bpcs/extract", &imgBPCSEx);
    ws.register_resource("/image/lsb/embed", &imgLSBEm);
    ws.register_resource("/image/lsb/extract", &imgLSBEx);
    ws.register_resource("/audio/lsb/embed", &audioLSBEm);
    ws.register_resource("/audio/lsb/extract", &audioLSBEx);
    ws.register_resource("/video/lsb/embed", &videoLSBEm);
    ws.register_resource("/video/lsb/extract", &videoLSBEx);
    ws.register_resource("/results/{fileId}", &results);
    ws.register_resource("/extracts/{fileId}", &extracts);

    std::cout << "Server started on port 8080" << std::endl;
    ws.start(true);

    return 0;
}