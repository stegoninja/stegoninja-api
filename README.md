# StegoNinja-CPP

StegoNinja is a command-line interface (CLI) and terminal user interface (TUI) tool for performing steganography on images, videos, and audio files. Additionally, it can be deployed as a web server for usability.

This C++-based implementation of StegoNinja supports the following steganographic techniques:
- **Image Steganography:** LSB (Least Significant Bit) and BPCS (Bit-Plane Complexity Segmentation)
- **Video Steganography:** LSB-based encoding
- **Audio Steganography:** LSB-based encoding

This repository specifically hosts the C++ implementation of StegoNinja.

## Installation

For Image LSB steganography and Video LSB steganography, go to root directory and run the code below to build the executables

```shell
cmake
make
```

For Image BPCS steganography, go to src/ directory and run the code below to build the executables

```shell
g++ imageBPCSEmbed.cpp -o imageBPCSEmbed
g++ imageBPCSExtract.cpp -o imageBPCSExtract
```

For Audio LSB steganography, go to src/ directory and run the code below to build the executables

```shell
g++ audio.cpp -o audio
```

To build the webserver docker image, go to root directory and run

```shell
docker build . -t stegoninja
```

To run the built docker image, run

```shell
docker run -d -p 8080:8080 --name stegoninja stegoninja
```