#pragma once

#include "modules/soundboard/api/ISoundBoardService.hpp"
#include "modules/soundboard/domain/ISoundboardPlayer.hpp"

#include "modules/media/domain/IMediaRepository.hpp"

namespace omc::soundboard
{
	class SoundBoardService : public ISoundBoardService
	{
	public:
		SoundBoardService(ISoundboardPlayer& player, omc::media::IMediaRepository& mediaRepository);
		~SoundBoardService();

		std::vector<SoundboardDeviceDto> getAudioDevices() const override;
		bool setAudioDevice(SetInputDeviceDto& deviceId) override;

		bool playMedia(int mediaId) override;

	private:
		ISoundboardPlayer& player;
		omc::media::IMediaRepository& mediaRepository;
	};
}