#pragma once

#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include <memory>

#include "modules/soundboard/domain/ISoundboardPlayer.hpp"
#include "AudioRingBuffer.hpp"

namespace omc::soundboard
{
    class WasapiPlayer : public ISoundboardPlayer
    {
    public:
        WasapiPlayer();
        ~WasapiPlayer() override;

        std::vector<AudioDevice> getAllAudioDevices() override;
        int getSelectedAudioDevice() override;
        bool selectAudioDevice(int device_id) override;
        bool playSound(const std::string& filepath) override;

    private:
        int selectedDeviceId;
        std::atomic<bool> isPlaying;
        AudioRingBuffer ringBuffer;

        void producerTask(const std::string& filepath);
        void consumerTask(int deviceId, int readerId);
    };

} // omc::soundboard