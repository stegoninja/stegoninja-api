# StegoNinja-CPP

StegoNinja hides (embeds) and recovers (extracts) a secret file inside a cover
**image**, **audio**, or **video** file using steganography.

It can be used two ways:

- **Web API** (primary) — a `libhttpserver`-based HTTP service, run via Docker.
- **CLI / TUI tools** — standalone C++ programs for interactive/offline use.

Supported techniques:

| Media | Technique |
|-------|-----------|
| Image | LSB (Least Significant Bit) and BPCS (Bit-Plane Complexity Segmentation) |
| Audio | LSB (over WAV PCM samples) |
| Video | LSB (over frame pixels, lossless FFV1/AVI output) |

---

## Web API

The web service is **API-only** (JSON responses, no HTML UI). The full contract
is described in [`openapi.yaml`](./openapi.yaml).

### Run it (Docker Compose)

```shell
docker compose up -d --build                          # host port 8080
STEGO_HOST_PORT=8099 docker compose up -d --build     # override if 8080 is in use
```

The host port (`STEGO_HOST_PORT`, default `8080`) maps to container port `8080`.
Check it's up:

```shell
curl http://localhost:8080/
```

### Endpoints

| Method & Path | Purpose |
|---------------|---------|
| `GET  /` | Service info / health |
| `POST /image/bpcs/embed` · `/image/bpcs/extract` | Image steganography (BPCS) |
| `POST /image/lsb/embed` · `/image/lsb/extract` | Image steganography (LSB) |
| `POST /audio/lsb/embed` · `/audio/lsb/extract` | Audio steganography (LSB) |
| `POST /video/lsb/embed` · `/video/lsb/extract` | Video steganography (LSB) |
| `GET  /results/{fileId}` | Download a produced stego file |
| `GET  /extracts/{fileId}` | Download a recovered secret file |

### Request contract

Embed and extract requests are `multipart/form-data`:

- **Embed** parts: `cover` (the carrier file) + `secret` (the file to hide).
- **Extract** part: `stego` (a stego file produced by an embed call).
- Optional fields (all endpoints):
  - `password` — key for encryption / randomization.
  - `encrypt` — `"true"` to Vigenère-encrypt the payload with `password`.
  - `randomize` — `"true"` to randomize carrier positions using `password`.

Extraction must use the **same** `encrypt` / `randomize` / `password` as embedding.

Output formats: image (LSB & BPCS) → BMP, audio → WAV, video → lossless FFV1/AVI
(re-upload that AVI to extract).

Responses are JSON: `{ "status": ..., "message": ..., "data": { ... } }`. Embed
responses include `data.result` (the download path) and `data.psnr`; extract
responses include `data.result` and `data.originalFilename`.

### Example round-trip (image LSB)

```shell
BASE=http://localhost:8080

# 1. Embed secret.txt into cover.png
curl -s -X POST $BASE/image/lsb/embed \
  -F cover=@cover.png -F secret=@secret.txt
# -> {"status":"success","data":{"result":"/results/<id>","psnr":"76.35",...}}

# 2. Download the stego image
curl -s $BASE/results/<id> -o stego.bmp

# 3. Extract the hidden file back out
curl -s -X POST $BASE/image/lsb/extract -F stego=@stego.bmp
# -> {"status":"success","data":{"result":"/extracts/<id2>","originalFilename":"secret.txt"}}

# 4. Download the recovered secret
curl -s $BASE/extracts/<id2> -o recovered.txt
```

With encryption + randomization, add `-F encrypt=true -F randomize=true -F password=hunter2`
to **both** the embed and extract calls.

See [`openapi.yaml`](./openapi.yaml) for the complete reference.

---

## CLI / TUI tools

These build separately from the web server.

### CMake targets (image LSB TUI + video)

From the repository root:

```shell
cmake -S . -B build && cmake --build build
```

Produces two executables:

- `SteganoImgLsb` — image LSB TUI (`src/main.cpp`, `src/stegano.cpp`, `src/vigenere.cpp`)
- `SteganoVid` — video LSB (`src/video.cpp`)

Requires OpenCV, ncurses/Curses, and pthreads.

### Standalone tools (not in CMake)

Compile directly with g++:

```shell
g++ -std=c++17 src/audio.cpp          -o audio
g++ -std=c++17 src/imgBPCSEmbed.cpp   -o imgBPCSEmbed
g++ -std=c++17 src/imgBPCSExtract.cpp -o imgBPCSExtract
```

Prebuilt versions of these are also available in `bin/`
(`audio`, `imgBPCSEmbed`, `imgBPCSExtract`).

---

## Repository layout

| Path | Role |
|------|------|
| `webserver.cpp`, `web/`, `include/` | Web API server and its implementation |
| `src/` | CLI / TUI tools |
| `openapi.yaml` | API specification (OpenAPI 3.0) |
| `compose.yml`, `Dockerfile` | Container build & deployment |
| `bin/` | Prebuilt standalone CLI binaries |
