/* 
* 
* Used this program as a working example to learn more about WASAPI and the process of getting audio data from a loopback
* 
*/

#include <iostream>
#include <Audioclient.h>
#include <audiopolicy.h>
#include <mmdeviceapi.h>
#include <winerror.h>
#include <math.h>
//for PKEY_Device_FriendlyName
#include <functiondiscoverykeys.h>

#define EXIT_ON_ERROR(hres) \
	if (FAILED(hres)) { std::cout<<"\nError Exit Triggered. 0x"<< std::hex << hres; goto Exit; }
#define SAFE_RELEASE(punk) \
	if((punk) != NULL) \
		{(punk)->Release(); (punk) = NULL;}

#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);

void captureStream() {
    CoInitialize(nullptr);
	HRESULT hr; // gives results of stuff (errors mostly)
	REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC;
	REFERENCE_TIME hnsActualDuration;

    UINT32 bufferFrameCount;
    UINT32 numFramesAvailable;
    
    IMMDeviceEnumerator* pEnumerator = NULL;
    IMMDevice* pDevice = NULL; // IMMDevice interface for rendering endpoint device
    IAudioClient* pAudioClient = NULL; // these guys work together to handle the audio stream
    IAudioCaptureClient* pCaptureClient = NULL; // this is the stream object I think, where is it configured as loopback
    
    WAVEFORMATEX* pwfx = NULL; // contains format information (wFormatTag says that you need to get the rest of the information from the extensible struct)
    WAVEFORMATEXTENSIBLE* pwfx2 = NULL; // contains more format information than the above
    
    UINT32 packetLength = 0;
    
    BOOL bDone = FALSE;
    
    BYTE* pData;
    
    DWORD flags;

    hr = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&pEnumerator);
    EXIT_ON_ERROR(hr);
    printf("\nInstance created.");

    // eRender configures as loopback
    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice); // this is where it should be configured as loopback
    if (pDevice == nullptr) {
        printf("\nDevice not found.");
    }
    EXIT_ON_ERROR(hr);
    printf("\nDevice found.");

    // get audioclient
    hr = pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pAudioClient);
    EXIT_ON_ERROR(hr);
    printf("\nAudio client activated.");

    // get format
    hr = pAudioClient->GetMixFormat(&pwfx);
    EXIT_ON_ERROR(hr);
    printf("\nMix format acquired.");

    hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,AUDCLNT_STREAMFLAGS_LOOPBACK,hnsRequestedDuration,0,pwfx,NULL);
    EXIT_ON_ERROR(hr)

    // Get the size of the allocated buffer.
    hr = pAudioClient->GetBufferSize(&bufferFrameCount);
    EXIT_ON_ERROR(hr);
    printf("\nBuffer size acquired.");

    // Calling the IAudioClient::GetService method to get an IAudioCaptureClient
    hr = pAudioClient->GetService(IID_IAudioCaptureClient, (void**)&pCaptureClient);
    EXIT_ON_ERROR(hr);
    printf("\nIAudioCaptureClient acquired.");

    // Calculate the actual duration of the allocated buffer.
    hnsActualDuration = (double)REFTIMES_PER_SEC * bufferFrameCount / pwfx->nSamplesPerSec;

    hr = pAudioClient->Start();  // Start recording.
    EXIT_ON_ERROR(hr);
    printf("\nRecording started.");

    // Each loop fills about half of the shared buffer.

    // Use the struct contained in pwfx to get information on the stream (https://docs.microsoft.com/en-us/windows/win32/api/mmeapi/ns-mmeapi-waveformatex)
    printf("\nWave format: %d", pwfx->wFormatTag);
    printf("\nNum channels: %d", pwfx->nChannels);
    printf("\nBits per sample: %d", pwfx->wBitsPerSample);
    printf("\nSamples per second: %d", pwfx->nSamplesPerSec);

    // since the WAVEFORMATEX format is the first element of the WAVEFORMATEXTENSIBLE struct, you can just cast a pointer to the WAVEFORMATEX struct to WAVEFORMATEXTENSIBLE to get the pointer to the larger WAVEFORMATEXTENSIBLE struct 
    pwfx2 = (WAVEFORMATEXTENSIBLE*)pwfx;

    printf("\nWave format from extensible: %d", pwfx2->SubFormat);
    printf("\nIs IEEE float PCM? %d", pwfx2->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT);
    printf("\nBits per sample from extensible: %d", pwfx2->Samples.wValidBitsPerSample);

    while (bDone == FALSE){
        // Sleep for half the buffer duration.
        Sleep(hnsActualDuration / REFTIMES_PER_MILLISEC / 2);

        hr = pCaptureClient->GetNextPacketSize(&packetLength);
        EXIT_ON_ERROR(hr);

        while (packetLength != 0){
            // Get the available data in the shared buffer.
            // One frame is both channels at a certain point in time. 32 bits per channel per frame, so 64 bits total per frame
            hr = pCaptureClient->GetBuffer(&pData, &numFramesAvailable, &flags, NULL, NULL);
            EXIT_ON_ERROR(hr);
            
            if (flags & AUDCLNT_BUFFERFLAGS_SILENT){
                printf("\nNo Audio.");
            }
            else {
                //printf("\nAudio Detected.");
                //printf("\nFrames: %d", numFramesAvailable);
                //printf("%f\n", *(float*)pData);
                float total = 0; // calculate the RMS amplitude of the current frame
                for (int i = 0; i < numFramesAvailable; i += 8) {
                    total += ((*(float*)(pData+i))*(*(float*)(pData + i))+(*(float*)(pData + i + 4)) * (*(float*)(pData + i + 4)))/2;
                    // this uses both channels, the first channel audio is stored in the first 4 bytes (32 bits) and the second channel audio is stored in the next 4 bytes (8 bytes, 64 bits total)
                    // can cast a pointer to the start of float bytes to a float pointer before dereferencing it to convert the bytes into a float
                }
                total /= numFramesAvailable;
                total = sqrtf(total);
                printf("\nRoot-mean-square: %f", total*100);
            }

            hr = pCaptureClient->ReleaseBuffer(numFramesAvailable);
            EXIT_ON_ERROR(hr);

            hr = pCaptureClient->GetNextPacketSize(&packetLength);
            EXIT_ON_ERROR(hr);
        }
    }

    hr = pAudioClient->Stop();  // Stop recording.
    EXIT_ON_ERROR(hr);

    CoUninitialize();

    Exit:
        CoTaskMemFree(pwfx);
        SAFE_RELEASE(pEnumerator);
        SAFE_RELEASE(pDevice);
        SAFE_RELEASE(pAudioClient);
        SAFE_RELEASE(pCaptureClient);
        CoUninitialize();
}

int main() {
	captureStream();
	return 0;
}