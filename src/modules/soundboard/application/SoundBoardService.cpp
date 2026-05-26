#include "SoundBoardService.hpp"

namespace omc::soundboard
{
	SoundBoardService::SoundBoardService() {

	}

	SoundBoardService::~SoundBoardService() = default;

	std::vector<SoundboardDeviceDto> SoundBoardService::getAudioDevices() const
	{
		auto testDevice = SoundboardDeviceDto{ .device_id = 1, .name = "Test Device", .selected = true };
		auto testDevice2 = SoundboardDeviceDto{ .device_id = 2, .name = "Test Device 2", .selected = false };

		auto returnValue = std::vector<SoundboardDeviceDto>{ testDevice, testDevice2 };
		return returnValue;
	}

	bool SoundBoardService::setAudioDevice(SetInputDeviceDto& deviceId)
	{
		return true;
	}
}