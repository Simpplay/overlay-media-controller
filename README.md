# overlay-media-controller

A C++ application that manages allways on top overlays through an HTTP REST API, using
**WebView2** for advanced web content rendering, **SQLite** for persistent storage, and
**cpp-httplib** for the HTTP server.

---

## Dependencies

| Library | Purpose |
|---------|---------|
| [WebView2](https://developer.microsoft.com/es-es/microsoft-edge/webview2/) | Advanced web content rendering |
| [SQLite 3](https://www.sqlite.org/) | Persist overlay configurations |
| [cpp-httplib](https://github.com/yhirose/cpp-httplib) | Embedded HTTP/1.1 server |
| [gtest](https://github.com/google/googletest) | Unit testing framework |

---

## Building

The binary is produced at `build/overlay-media-controller`.

### Using vcpkg

The `vcpkg.json` manifest lists all dependencies so that vcpkg can
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

---

### Health check

```http
GET /health
```

Response:

```json
{
    "status": "ok"
}
```

---

## Media sources

| Method | Path | Description |
|---|---|---|
| `GET` | `/api/media` | List all media sources |
| `POST` | `/api/media` | Add a new media source |
| `GET` | `/api/media/:id` | Get a single media source |
| `DELETE` | `/api/media/:id` | Delete a media source |
| `GET` | `/api/media/:id/show` | Show the media source in an overlay |

---

### POST /api/media

Add a new media source.

Request:
- Content-Type: `multipart/form-data`

Response:
- Content-Type: `application/json`

#### Form fields

| Field | Type | Required | Description |
|---|---|---|---|
| `media` | file | Yes | Media file to upload |
| `title` | string | No | Display title |
| `description` | string | No | Media description |

#### Example request

```bash
curl -X POST http://localhost:8080/api/media \
  -F "media=@video.mp4" \
  -F "title=Demo Video" \
  -F "description=Test upload"
```

#### Example response

```json
{
    "id": 1,
    "filename": "video.mp4",
    "content_type": "video/mp4",
    "status": "stored"
}
```

---

### GET /api/media

List all media sources.

Response:

```json
[
    {
        "id": 1,
        "title": "Demo Video",
        "filename": "video.mp4",
        "content_type": "video/mp4"
    }
]
```

---

### GET /api/media/:id

Get a single media source.

#### Example response

```json
{
    "id": 1,
    "title": "Demo Video",
    "filename": "video.mp4",
    "content_type": "video/mp4"
}
```

---

### DELETE /api/media/:id

Delete a media source.

#### Example response

```json
{
    "status": "deleted"
}
```

---

### GET /api/media/:id/show

Show the media source in an overlay.

Request:
- Content-Type: `application/json`

Response:
- Content-Type: `application/json`

#### Request body

```json
{
    "fullscreen": true,
    "position": {
        "x": 100,
        "y": 100
    },
    "size": {
        "width": 800,
        "height": 600
    }
}
```

#### Example response

```json
{
    "status": "shown"
}
```

---

## Project structure

```
overlay-media-controller/
├── CMakeLists.txt          # CMake build definition
├── vcpkg.json              # vcpkg dependency manifest
├── tests/                  # Unit tests
└── src/
    ├── app/                # App entry point and main loop
    ├── core/               # Core logic for event handling and types
    ├── infra/              # Specific implementations for WebView2, SQLite, and cpp-httplib
    ├─- shared/             # Shared utilities and helper functions 
    └── modules/
        ├── input/          # Shortcuts and global hotkeys
        ├── media/          # Media probing and playback logic
        ├── server/         # HTTP server and API handlers
        ├── storage/        # Persistent storage management
        └── ui/             # Overlay management logic
```