![Build](https://github.com/Simpplay/overlay-media-controller/actions/workflows/build.yml/badge.svg)

# Overlay Media Controller

A C++ application that manages always-on-top overlays through an HTTP REST API, using
**WebView2** for advanced web content rendering, **SQLite** for persistent storage, and
**cpp-httplib** for the HTTP server.

---

## Dependencies

| Library | Purpose |
|---|---|
| [WebView2](https://developer.microsoft.com/es-es/microsoft-edge/webview2/) | Advanced web content rendering |
| [SQLite 3](https://www.sqlite.org/) | Persistent storage for media and overlay data |
| [cpp-httplib](https://github.com/yhirose/cpp-httplib) | Embedded HTTP/1.1 server |
| [gtest](https://github.com/google/googletest) | Unit testing framework |

---

## Building

The binary is generated at:

```bash
build/overlay-media-controller
```

### Using vcpkg

The project uses a `vcpkg.json` manifest so dependencies can be installed automatically when using the vcpkg CMake toolchain.

```bash
cmake -B build \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake

cmake --build build --parallel
```

---

## Running

### Default configuration

```bash
./overlay-media-controller
```

Uses:
- Database: `overlay_media_controller.db`
- Port: `8080`

### Optional command-line arguments

| Argument | Description | Default |
| --- | --- | --- |
| `db-path` | Path to the SQLite database file | `overlay_media_controller.db` |
| `port` | Port for the API server | `8080` |

```bash
./overlay-media-controller --db-path another_database.db --port 9090
```

### Using environment variables

```bash
OMC_DB_PATH=another_database.db \
OMC_PORT=9090 \
./overlay-media-controller
```

---

## Running the Soundboard

In order to play audio through the microphone you need to install a virtual audio driver.
I recommend the use of [VB-CABLE Virtual Audio Device](https://vb-audio.com/Cable/).
Then, you have to choose the CABLE input in the settings.

### Ffmpeg

You also need Ffmpeg to read all the audio files. You can download via Powershell running:
```Powershell
winget install Gyan.FFmpeg
```

---

# API Reference

All responses use:

```http
Content-Type: application/json
```

Base URL:

```txt
http://localhost:8080
```

---

# Health Check

Used to verify that the HTTP server is running.

## Request

```http
GET /health
```

## Response

### 200 OK

```json
{
    "status": "ok"
}
```

---

# Media

Media resources represent uploaded multimedia assets such as videos, audio files, images, or GIFs.

## Endpoints Summary

| Method | Path | Description |
|---|---|---|
| `GET` | `/api/media` | List media |
| `POST` | `/api/media` | Upload media |
| `GET` | `/api/media/:id` | Retrieve a single media |
| `DELETE` | `/api/media/:id` | Delete media |
| `PATCH` | `/api/media/:id` | Update media metadata |
| `GET` | `/api/media/:id/thumbnail` | Retrieve media thumbnail |

---

## Upload Media

Uploads a new media file into the library.

### Request

```http
POST /api/media
```

### Content Type

```http
multipart/form-data
```

### Form Fields

| Field | Type | Required | Description |
|---|---|---|---|
| `media` | file | Yes | Media file to upload |
| `title` | string | No | Display title |

### Example

```bash
curl -X POST http://localhost:8080/api/media \
  -F "media=@video.mp4" \
  -F "title=Demo Video"
```

### Response

#### 201 Created

```json
{
    "id": 1,
    "filename": "video.mp4",
    "content_type": "video/mp4",
    "status": "stored"
}
```

---

## List Media

Returns all stored media.

Optional query parameters can be used for filtering.

### Request

```http
GET /api/media
```

### Optional Query Parameters

| Parameter | Description |
|---|---|
| `query` | Filter media by title |
| `category` | Filter media by category |

### Example

```http
GET /api/media?query=demo&category=Videos
```

### Response

#### 200 OK

```json
[
    {
        "id": 1,
        "title": "Demo Video",
        "filename": "video.mp4",
        "content_type": "video/mp4",
        "categories": [
            "Videos"
        ]
    }
]
```

---

## Get Media

Returns a single media resource.

### Request

```http
GET /api/media/:id
```

### Response

#### 200 OK

```json
{
    "id": 1,
    "title": "Demo Video",
    "filename": "video.mp4",
    "content_type": "video/mp4",
    "categories": [
        "Videos"
    ]
}
```

---

## Delete Media

Deletes a media resource.

### Request

```http
DELETE /api/media/:id
```

### Response

#### 204 No Content

---

## Update Media Metadata

Updates media metadata such as title.

## Request
```http
PATCH /api/media/:id
```

## Request Fields

| Field | Description | Type | 
| --- | --- | --- |
| `title` | The new title of the media | `string` |


## Request Example
```json
{
    "title": "Updated Title"
}
```

## Response

#### 200 OK
```json
{
    "id": 1,
    "title": "Updated Title",
    "filename": "video.mp4",
    "content_type": "video/mp4",
    "categories": [
        "Videos"
    ]
}
```

---

## Get Media Thumbnail

Returns the thumbnail associated with a media resource.

### Request

```http
GET /api/media/:id/thumbnail
```

### Response

#### 200 OK

Thumbnail in png format with size of 256x256

---

# Categories

Categories are used to organize media into logical groups.

## Endpoints Summary

| Method | Path | Description |
|---|---|---|
| `GET` | `/api/categories` | List categories |
| `POST` | `/api/categories` | Create category |
| `GET` | `/api/categories/:id` | Retrieve category |
| `DELETE` | `/api/categories/:id` | Delete category |
| `PUT` | `/api/categories/:id/media/:media_id` | Add media to category |
| `DELETE` | `/api/categories/:id/media/:media_id` | Remove media from category |

---

## Create Category

Creates a new category.

### Request

```http
POST /api/categories
```

### Request Body

```json
{
    "name": "Videos"
}
```

### Response

#### 201 Created

```json
{
    "id": 1,
    "name": "Videos"
}
```

---

## List Categories

Returns all categories.

### Request

```http
GET /api/categories
```

### Response

#### 200 OK

```json
[
    {
        "id": 1,
        "name": "Videos"
    }
]
```

---

## Get Category

Returns a category and all associated media.

### Request

```http
GET /api/categories/:id
```

### Response

#### 200 OK

```json
{
    "id": 1,
    "name": "Videos",
    "media": [
        {
            "id": 1,
            "title": "Demo Video",
            "filename": "video.mp4",
            "content_type": "video/mp4"
        }
    ]
}
```

---

## Delete Category

Deletes a category.

### Request

```http
DELETE /api/categories/:id
```

### Response

#### 204 No Content

---

## Add Media to Category

Associates a media resource with a category.

### Request

```http
PUT /api/categories/:id/media/:media_id
```

### Response

#### 200 OK

```json
{
    "status": "added"
}
```

---

## Remove Media from Category

Removes a media resource from a category.

### Request

```http
DELETE /api/categories/:id/media/:media_id
```

### Response

#### 204 No Content

---

# Overlays

Overlays are runtime instances that render media on screen.

A single media resource can be displayed in multiple overlays simultaneously.

## Endpoints Summary

| Method | Path | Description |
|---|---|---|
| `GET` | `/api/overlays` | List active overlays |
| `POST` | `/api/overlays` | Create overlay |
| `PATCH` | `/api/overlays/:id` | Update overlay |
| `DELETE` | `/api/overlays/:id` | Close overlay |

---

## List Active Overlays

Returns all active overlays currently displayed.

### Request

```http
GET /api/overlays
```

### Response

#### 200 OK

```json
[
    {
        "id": 1,
        "media_id": 1,
        "state": "active",
        "fullscreen": false,
        "position": {
            "x": 100,
            "y": 100
        },
        "size": {
            "width": 800,
            "height": 600
        }
    }
]
```

---

## Create Overlay

Creates a new overlay instance and displays media on screen.

### Request

```http
POST /api/overlays
```

### Request Body

```json
{
    "media_id": 1,
    "fullscreen": false,
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

### Response

#### 201 Created

```json
{
    "id": 1,
    "media_id": 1,
    "state": "active"
}
```

---

## Update Overlay

Updates overlay runtime properties such as position, size, or fullscreen mode.

### Request

```http
PATCH /api/overlays/:id
```

### Example Request Body

```json
{
    "fullscreen": true
}
```

or

```json
{
    "position": {
        "x": 300,
        "y": 200
    }
}
```

### Response

#### 200 OK

```json
{
    "id": 1,
    "state": "updated"
}
```

---

## Close Overlay

Closes an active overlay instance.

### Request

```http
DELETE /api/overlays/:id
```

### Response

#### 204 No Content

---

# Soundboard

Soundboards are media that plays through a VB driver

## Play Sound

### Request

```http
POST /api/soundboard
```

### Request Body

```json
{
    "media_id": 1,
    "volume": 100.0,
    "force": false
}
```

### Response

#### 201 Created

```json
{
    "status": "shown"
}
```

---

## Get input devices

### Request

```http
GET /api/soundboard/devices
```

### Response

#### 200 OK

```json
[
    {
        "device_id": 123456789,
        "name": CABLE Input,
        "selected": false
    },
    ...
]
```

---

## Set input device

### Request

```http
PUT /api/soundboard/devices
```

### Request Body

```json
{
    "device_id": 123456789,
}
```

### Response

#### 200 OK

---

# Project Structure

```txt
overlay-media-controller/
├── CMakeLists.txt
├── vcpkg.json
├── tests/
└── src/
    ├── app/
    ├── core/
    └── modules/
        ├── media/
        ├── server/
        └── ui/
            ├── api/
            ├── application/
            ├── domain/
            ├── infrastructure/
            └── runtime/
```