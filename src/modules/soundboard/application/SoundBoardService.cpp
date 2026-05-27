#include "SoundBoardService.hpp"

#include <iostream>

namespace omc::soundboard
{
	SoundBoardService::SoundBoardService(std::shared_ptr<ISoundboardPlayer> player, std::shared_ptr<omc::media::IMediaRepository> mediaRepository) :
		player(player), mediaRepository(mediaRepository)
	{

	}

	SoundBoardService::~SoundBoardService() = default;

	std::vector<SoundboardDeviceDto> SoundBoardService::getAudioDevices() const
	{
		auto audioDevices = player->getAllAudioDevices();
		int selectedDevice = player->getSelectedAudioDevice();
		auto returnValue = std::vector<SoundboardDeviceDto>();

		for (auto& device : audioDevices) {
			returnValue.push_back(SoundboardDeviceDto{
				.device_id = device.media_id,
				.name = device.name,
				.selected = device.media_id == selectedDevice
			});
		}

		return returnValue;
	}

	bool SoundBoardService::setAudioDevice(SetInputDeviceDto& deviceId)
	{
		return player->selectAudioDevice(deviceId.device_id);
	}

	bool SoundBoardService::playMedia(int mediaId)
	{
		auto media = mediaRepository->getMediaById(mediaId);
		if (!media) {
			std::cerr << "Media with ID " << mediaId << " not found." << std::endl;
			return false;
		}

		std::string filepath = media->filepath;

		return player->playSound(filepath);;
	}
}