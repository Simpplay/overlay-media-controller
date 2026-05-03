#pragma once

#include <string>
#include <stdexcept>

/// Basic information about a media stream obtained via FFmpeg.
struct MediaInfo {
    int         width{0};
    int         height{0};
    double      duration{0.0};   ///< Seconds; 0 when unknown.
    std::string codec_name;
    std::string format_name;
};

/// Thin wrapper around the FFmpeg libraries for probing media files and
/// generating thumbnail images.
class MediaController {
public:
    MediaController();
    ~MediaController();

    // Non-copyable
    MediaController(const MediaController&)            = delete;
    MediaController& operator=(const MediaController&) = delete;

    /// Probe @p path and return media information.
    /// Throws std::runtime_error on failure.
    MediaInfo probe(const std::string& path) const;

    /// Decode the frame closest to @p time_offset seconds and write it as a
    /// PNG file to @p output_path.
    /// Returns true on success.
    bool generate_thumbnail(const std::string& source_path,
                            const std::string& output_path,
                            double             time_offset = 0.0) const;
};
