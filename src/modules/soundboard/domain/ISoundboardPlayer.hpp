#pragma once

#include <vector>

#include "AudioDevice.hpp"

namespace omc::soundboard {

	class ISoundboardPlayer
	{
	public:
		virtual ~ISoundboardPlayer() = default;

		virtual std::vector<AudioDevice> getAllAudioDevices() = 0;
		virtual int getSelectedAudioDevice() = 0;
		virtual bool selectAudioDevice(int device_id) = 0;

		virtual bool playSound(const std::string& filepath) = 0;
	};
} // omc::soundboard