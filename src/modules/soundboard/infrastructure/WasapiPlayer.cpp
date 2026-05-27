#include "WasapiPlayer.hpp"

#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <functiondiscoverykeys_devpkey.h>
#include <propvarutil.h>
#include <wrl/client.h>
#include <avrt.h>

#include <cstdio>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <iostream>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "avrt.lib")

using Microsoft::WRL::ComPtr;

namespace omc::soundboard
{
    namespace
    {
        // Helper to convert Wide string to UTF-8
        std::string WideToUtf8(const wchar_t* wide)
        {
            if (!wide) return "";
            int len = WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
            if (len <= 1) return "";
            std::string utf8(len - 1, '\0');
            WideCharToMultiByte(CP_UTF8, 0, wide, -1, utf8.data(), len, nullptr, nullptr);
            return utf8;
        }

        // Helper to get the default IMMDevice
        ComPtr<IMMDevice> getDefaultDevice()
        {
            ComPtr<IMMDeviceEnumerator> enumerator;
            HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&enumerator));
            if (FAILED(hr)) return nullptr;

            ComPtr<IMMDevice> device;
            hr = enumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &device);
            return (SUCCEEDED(hr)) ? device : nullptr;
        }

        // Helper to get a specific IMMDevice by index
        ComPtr<IMMDevice> getDeviceByIndex(int index)
        {
            ComPtr<IMMDeviceEnumerator> enumerator;
            HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&enumerator));
            if (FAILED(hr)) return nullptr;

            ComPtr<IMMDeviceCollection> collection;
            hr = enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &collection);
            if (FAILED(hr)) return nullptr;

            ComPtr<IMMDevice> device;
            hr = collection->Item(index, &device);
            return (SUCCEEDED(hr)) ? device : nullptr;
        }
    }

    WasapiPlayer::WasapiPlayer()
        : selectedDeviceId(-1), isPlaying(false), ringBuffer(48000 * 2 * 4 * 2) // 2 seconds of buffer
    {
    }

    WasapiPlayer::~WasapiPlayer()
    {
        isPlaying = false;
        ringBuffer.setDone(true);
    }

    std::vector<AudioDevice> WasapiPlayer::getAllAudioDevices()
    {
        HRESULT hr_com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

        std::vector<AudioDevice> devices;
        ComPtr<IMMDeviceEnumerator> enumerator;

        if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&enumerator))))
        {
            if (SUCCEEDED(hr_com)) CoUninitialize();
            return devices;
        }

        ComPtr<IMMDeviceCollection> collection;
        if (FAILED(enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &collection)))
        {
            if (SUCCEEDED(hr_com)) CoUninitialize();
            return devices;
        }

        UINT count = 0;
        collection->GetCount(&count);

        for (UINT i = 0; i < count; ++i)
        {
            ComPtr<IMMDevice> device;
            if (FAILED(collection->Item(i, &device))) continue;

            ComPtr<IPropertyStore> props;
            if (FAILED(device->OpenPropertyStore(STGM_READ, &props))) continue;

            PROPVARIANT name;
            PropVariantInit(&name);
            if (SUCCEEDED(props->GetValue(PKEY_Device_FriendlyName, &name)))
            {
                devices.push_back(AudioDevice{ static_cast<int>(i), WideToUtf8(name.pwszVal) });
            }
            PropVariantClear(&name);
        }

        if (SUCCEEDED(hr_com)) CoUninitialize();
        return devices;
    }

    int WasapiPlayer::getSelectedAudioDevice()
    {
        return selectedDeviceId;
    }

    bool WasapiPlayer::selectAudioDevice(int device_id)
    {
        selectedDeviceId = device_id;
        return true;
    }

    bool WasapiPlayer::playSound(const std::string& filepath)
    {
        if (selectedDeviceId < 0 || isPlaying)
            return false;

        isPlaying = true;
        ringBuffer.reset();

        // Start producer
        std::thread producer([this, filepath]() { producerTask(filepath); });
        
        std::thread consumer1([this]() { consumerTask(selectedDeviceId, 0); });
        std::thread consumerDefault([this]() { consumerTask(-1, 1); }); // -1 indicates default device

        if (producer.joinable()) producer.join();
        if (consumer1.joinable()) consumer1.join();
        if (consumerDefault.joinable()) consumerDefault.join();

        isPlaying = false;
        return true;
    }

    void WasapiPlayer::producerTask(const std::string& filepath)
    {
        std::string command = "ffmpeg -i \"" + filepath + "\" -f f32le -ar 48000 -ac 2 -loglevel error -";
        FILE* pipe = _popen(command.c_str(), "rb");
        if (!pipe)
        {
            ringBuffer.setDone(true);
            return;
        }

        char buffer[1024];
        while (isPlaying)
        {
            size_t bytesRead = fread(buffer, 1, sizeof(buffer), pipe);
            if (bytesRead <= 0) break;
            ringBuffer.write(buffer, bytesRead);
        }

        _pclose(pipe);
        ringBuffer.setDone(true);
    }

    void WasapiPlayer::consumerTask(int deviceId, int readerId)
    {
        HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (FAILED(hr)) return;

        ComPtr<IMMDevice> device;
        if (deviceId == -1) {
            device = getDefaultDevice();
        } else {
            device = getDeviceByIndex(deviceId);
        }

        if (!device) {
            CoUninitialize();
            return;
        }

        ComPtr<IAudioClient> audioClient;
        if (FAILED(device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, &audioClient))) {
            CoUninitialize();
            return;
        }

        WAVEFORMATEXTENSIBLE wfx = {};
        wfx.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        wfx.Format.nChannels = 2;
        wfx.Format.nSamplesPerSec = 48000;
        wfx.Format.wBitsPerSample = 32;
        wfx.Format.nBlockAlign = (wfx.Format.nChannels * wfx.Format.wBitsPerSample) / 8;
        wfx.Format.nAvgBytesPerSec = wfx.Format.nSamplesPerSec * wfx.Format.nBlockAlign;
        wfx.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
        wfx.Samples.wValidBitsPerSample = 32;
        wfx.dwChannelMask = KSAUDIO_SPEAKER_STEREO;
        wfx.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;

        REFERENCE_TIME hnsRequestedDuration = 500000; // 50ms
        hr = audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 
                                   AUDCLNT_STREAMFLAGS_EVENTCALLBACK, 
                                   hnsRequestedDuration, 0, (WAVEFORMATEX*)&wfx, nullptr);
        if (FAILED(hr)) {
            CoUninitialize();
            return;
        }

        HANDLE hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (!hEvent) {
            CoUninitialize();
            return;
        }

        hr = audioClient->SetEventHandle(hEvent);
        if (FAILED(hr)) {
            CloseHandle(hEvent);
            CoUninitialize();
            return;
        }

        ComPtr<IAudioRenderClient> renderClient;
        if (FAILED(audioClient->GetService(IID_PPV_ARGS(&renderClient)))) {
            CloseHandle(hEvent);
            CoUninitialize();
            return;
        }

        UINT32 bufferFrameCount;
        audioClient->GetBufferSize(&bufferFrameCount);

        hr = audioClient->Start();
        if (FAILED(hr)) {
            CloseHandle(hEvent);
            CoUninitialize();
            return;
        }

        DWORD taskIndex = 0;
        HANDLE hTask = AvSetMmThreadCharacteristicsW(L"Audio", &taskIndex);

        while (isPlaying)
        {
            DWORD waitResult = WaitForSingleObject(hEvent, 1000);
            if (waitResult != WAIT_OBJECT_0) break;

            UINT32 padding;
            if (FAILED(audioClient->GetCurrentPadding(&padding))) break;

            UINT32 framesAvailable = bufferFrameCount - padding;
            if (framesAvailable > 0)
            {
                BYTE* pData;
                if (SUCCEEDED(renderClient->GetBuffer(framesAvailable, &pData)))
                {
                    size_t bytesToRead = framesAvailable * wfx.Format.nBlockAlign;
                    size_t bytesRead = ringBuffer.read(readerId, pData, bytesToRead);

                    if (bytesRead < bytesToRead)
                    {
                        memset(pData + bytesRead, 0, bytesToRead - bytesRead);
                        if (ringBuffer.isDone(readerId) && bytesRead == 0)
                        {
                            renderClient->ReleaseBuffer(framesAvailable, AUDCLNT_BUFFERFLAGS_SILENT);
                            break; 
                        }
                    }

                    renderClient->ReleaseBuffer(framesAvailable, 0);
                }
            }
        }

        audioClient->Stop();
        if (hTask) AvRevertMmThreadCharacteristics(hTask);
        CloseHandle(hEvent);
        CoUninitialize();
    }

} // omc::soundboard
