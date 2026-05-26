#pragma once

#include "SoundboardDto.hpp"

namespace omc::soundboard
{
	class ISoundBoardService
	{
	public:
		virtual ~ISoundBoardService() = default;

		virtual std::vector<SoundboardDeviceDto> getAudioDevices() const = 0;
		virtual bool setAudioDevice(SetInputDeviceDto& deviceId) = 0;
	};
}