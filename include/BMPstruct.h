#ifndef BMPstruct_H
#define BMPstruct_H

#pragma pack(push, 1)

// BMP File Header Structure
struct BMPFileHeader {
    uint16_t fileType{0x4D42};
    uint32_t fileSize{0};
    uint16_t reserved1{0};
    uint16_t reserved2{0};
    uint32_t offsetData{0};
};

// BMP Info Header Structure
struct BMPInfoHeader {
    uint32_t headerSize{40};
    int32_t width{0};
    int32_t height{0};
    uint16_t planes{1};
    uint16_t bitCount{24};
    uint32_t compression{0};
    uint32_t imageSize{0};
    int32_t xPixelsPerMeter{2835};
    int32_t yPixelsPerMeter{2835};
    uint32_t colorsUsed{0};
    uint32_t colorsImportant{0};
};

#pragma pack(pop)

// RGB Pixel Structure
struct RGB {
    uint8_t r, g, b;
};

// Block Position Structure for Steganography
struct BlockPosition {
    int channel;
    int bitPlane;
    int x;
    int y;
};

std::vector<RGB> readBMP(const std::string& filename, int& width, int& height);

#endif