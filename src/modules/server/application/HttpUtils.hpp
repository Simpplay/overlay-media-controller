#pragma once

#include <httplib.h>
#include <span>
#include <string>
#include <string_view>
#include <stdexcept>

// ---------------------------------------------------------------------------
// Helper – extrae el primer capture group de la ruta y lo convierte a int.
// Lanza std::invalid_argument si el match no existe o no es numérico.
// ---------------------------------------------------------------------------
static int extractId(const httplib::Request& req, std::size_t matchIndex = 1)
{
	if (matchIndex >= req.matches.size())
		throw std::invalid_argument("Missing path parameter");

	return std::stoi(req.matches[matchIndex].str());
}

// ---------------------------------------------------------------------------
// Helper – convierte el std::string del body multipart a span<const byte>
// sin copias de memoria innecesarias.
// ---------------------------------------------------------------------------
static std::span<const std::byte> toByteSpan(const std::string& s) noexcept
{
	return { reinterpret_cast<const std::byte*>(s.data()), s.size() };
}

// ---------------------------------------------------------------------------
// Helper – respuesta de error homogénea
// ---------------------------------------------------------------------------
static void setError(httplib::Response& res, int status, std::string_view message)
{
	res.status = status;
	res.set_content(
		std::string("{\"error\":\"") + std::string(message) + "\"}",
		"application/json"
	);
}

// ---------------------------------------------------------------------------
// Helper – añade headers CORS permisivos.
// Necesario cuando el HTML se sirve desde un origen distinto (file://, otro
// puerto, etc.) y el navegador realiza peticiones cross-origin al servidor.
// ---------------------------------------------------------------------------
static void setCorsHeaders(httplib::Response& res)
{
	res.set_header("Access-Control-Allow-Origin", "*");
	res.set_header("Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS");
	res.set_header("Access-Control-Allow-Headers", "Content-Type, Range");
	res.set_header("Access-Control-Expose-Headers",
		"Content-Length, Content-Range, Accept-Ranges");
}

// ---------------------------------------------------------------------------
// Parsea el valor del header "Range: bytes=start-end".
// Soporta las tres formas del RFC 7233:
//   bytes=500-999     → rango cerrado
//   bytes=500-        → desde 500 hasta el final
//   bytes=-500        → últimos 500 bytes
// Devuelve false si el rango es inválido o no satisfacible.
// ---------------------------------------------------------------------------
static bool parseRangeHeader(
	const std::string& value,
	size_t             totalSize,
	size_t& start,
	size_t& end)
{
	if (!value.starts_with("bytes=") || totalSize == 0) {
		return false;
	}

	const auto spec = value.substr(6);
	const auto dash = spec.find('-');
	if (dash == std::string::npos) {
		return false;
	}

	const auto startToken = spec.substr(0, dash);
	const auto endToken = spec.substr(dash + 1);

	try {
		if (startToken.empty()) {
			// suffix-length: "bytes=-N"
			const size_t suffixLen = static_cast<size_t>(std::stoull(endToken));
			if (suffixLen == 0) return false;
			start = (suffixLen >= totalSize) ? 0 : totalSize - suffixLen;
			end = totalSize - 1;
		}
		else {
			start = static_cast<size_t>(std::stoull(startToken));
			end = endToken.empty()
				? (totalSize - 1)
				: static_cast<size_t>(std::stoull(endToken));
		}
	}
	catch (...) {
		return false;
	}

	return start <= end && end < totalSize;
}

// ---------------------------------------------------------------------------
// Helper – devuelve la extensión (con punto, en minúsculas) de un filename,
// o cadena vacía si no tiene extensión.
// ---------------------------------------------------------------------------
static std::string fileExtension(const std::string& filename)
{
	const auto dot = filename.rfind('.');
	if (dot == std::string::npos || dot + 1 == filename.size())
		return {};

	std::string ext = filename.substr(dot); // ".MP4", ".Mov", …
	std::transform(ext.begin(), ext.end(), ext.begin(),
		[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return ext;
}

// ---------------------------------------------------------------------------
// Helper – infiere el MIME type a partir de la extensión del fichero cuando
// el cliente reporta el tipo genérico application/octet-stream (o vacío).
// Solo sobreescribe el tipo declarado si éste es demasiado genérico para ser
// útil; cualquier tipo específico que venga del cliente se respeta tal cual.
// ---------------------------------------------------------------------------
static std::string inferMimeType(const std::string& filename,
	const std::string& declared)
{
	constexpr std::string_view kGeneric = "application/octet-stream";

	if (!declared.empty() && declared != kGeneric)
		return declared; // el cliente ya mandó algo concreto, no lo tocamos

	static const std::unordered_map<std::string, std::string> kMimeMap{
		// Video
		{ ".mp4",  "video/mp4"            },
		{ ".webm", "video/webm"           },
		{ ".mov",  "video/quicktime"      },
		{ ".avi",  "video/x-msvideo"      },
		{ ".mkv",  "video/x-matroska"     },
		{ ".flv",  "video/x-flv"          },
		{ ".wmv",  "video/x-ms-wmv"       },
		// Audio
		{ ".mp3",  "audio/mpeg"           },
		{ ".wav",  "audio/wav"            },
		{ ".ogg",  "audio/ogg"            },
		{ ".aac",  "audio/aac"            },
		{ ".flac", "audio/flac"           },
		{ ".m4a",  "audio/mp4"            },
		// Imagen
		{ ".png",  "image/png"            },
		{ ".jpg",  "image/jpeg"           },
		{ ".jpeg", "image/jpeg"           },
		{ ".gif",  "image/gif"            },
		{ ".webp", "image/webp"           },
		{ ".svg",  "image/svg+xml"        },
		{ ".bmp",  "image/bmp"            },
		// Documento
		{ ".pdf",  "application/pdf"      },
	};

	const std::string ext = fileExtension(filename);
	if (const auto it = kMimeMap.find(ext); it != kMimeMap.end())
		return it->second;

	// Sin mejor información, dejamos el tipo genérico tal como estaba.
	return declared.empty() ? std::string(kGeneric) : declared;
}