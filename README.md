# overlay-media-controller

A C++ application that manages media overlays through an HTTP REST API, using
**FFmpeg** for media processing, **SQLite** for persistent storage, and
**cpp-httplib** for the HTTP server.

---

## Dependencies

| Library | Purpose |
|---------|---------|
| [FFmpeg](https://ffmpeg.org/) (`libavcodec`, `libavformat`, `libavutil`, `libswscale`) | Probe media files, generate thumbnails |
| [SQLite 3](https://www.sqlite.org/) | Persist overlay configurations |
| [cpp-httplib](https://github.com/yhirose/cpp-httplib) | Embedded HTTP/1.1 server |

---

## Building

### Prerequisites (Debian / Ubuntu)

```bash
sudo apt-get install -y \
    build-essential cmake pkg-config \
    libavcodec-dev libavformat-dev libavutil-dev libswscale-dev \
    libsqlite3-dev \
    libcpp-httplib-dev
```

### Configure & compile

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

The binary is produced at `build/overlay-media-controller`.

### Using vcpkg (cross-platform)

The `vcpkg.json` manifest lists all three dependencies so that vcpkg can
install them automatically when the toolchain file is passed to CMake:

```bash
cmake -B build \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --parallel
```

---

## Running

```bash
# Default: database file "overlays.db", port 8080
./build/overlay-media-controller

# Custom database path and port via CLI arguments
./build/overlay-media-controller /var/db/overlays.db 9090

# Or via environment variables
OMC_DB_PATH=/var/db/overlays.db OMC_PORT=9090 ./build/overlay-media-controller
```

---

## API Reference

All responses are `application/json`.

### Health check

```
GET /health
→ {"status":"ok"}
```

### Overlays

| Method | Path | Description |
|--------|------|-------------|
| `GET`    | `/api/overlays`              | List all overlays |
| `POST`   | `/api/overlays`              | Create a new overlay |
| `GET`    | `/api/overlays/:id`          | Get a single overlay |
| `DELETE` | `/api/overlays/:id`          | Delete an overlay |
| `PUT`    | `/api/overlays/:id/active`   | Enable / disable an overlay |
| `GET`    | `/api/overlays/:id/probe`    | Probe the overlay's media source |

#### Create overlay – request body

```json
{
  "name":        "lower-third",
  "source_path": "/media/lower-third.mp4",
  "x":           0,
  "y":           900,
  "width":       1920,
  "height":      180
}
```

#### Set active flag – request body

```json
{ "active": true }
```

#### Probe response

```json
{
  "width":       1920,
  "height":      1080,
  "duration":    30.5,
  "codec_name":  "h264",
  "format_name": "QuickTime / MOV"
}
```

---

## Project structure

```
overlay-media-controller/
├── CMakeLists.txt          # CMake build definition
├── vcpkg.json              # vcpkg dependency manifest
└── src/
    ├── main.cpp            # Entry point
    ├── database.hpp/.cpp   # SQLite overlay persistence
    ├── media_controller.hpp/.cpp  # FFmpeg media probing & thumbnails
    └── api_server.hpp/.cpp # cpp-httplib HTTP API
```