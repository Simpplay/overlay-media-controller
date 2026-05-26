#pragma once

#include "modules/soundboard/api/ISoundBoardService.hpp"

namespace omc::soundboard
{
	class SoundBoardService : public ISoundBoardService
	{
	public:
		SoundBoardService();
		~SoundBoardService();

		std::vector<SoundboardDeviceDto> getAudioDevices() const override;
		bool setAudioDevice(SetInputDeviceDto& deviceId) override;
	};
}