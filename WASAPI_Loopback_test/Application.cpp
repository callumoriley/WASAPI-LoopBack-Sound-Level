#include <iostream>
#include <Audioclient.h>
#include <audiopolicy.h>
#include <mmdeviceapi.h>
#include <winerror.h>
//for PKEY_Device_FriendlyName
#include <functiondiscoverykeys.h>

#define EXIT_ON_ERROR(hres) \
	if (FAILED(hres)) { std::cout<<"\nError Exit Triggered."<<hres; goto Exit; }
#define SAFE_RELEASE(punk) \
	if((punk) != NULL) \
		{(punk)->Release(); (punk) = NULL;}

#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioCaptureClient);
const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);

void captureStream() {
    CoInitialize(nullptr);
	HRESULT hr;
	REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC;
	REFERENCE_TIME hnsActualDuration;

    UINT32 bufferFrameCount;
    UINT32 numFramesAvailable;
    
    IMMDeviceEnumerator* pEnumerator = NULL;
    IMMDevice* pDevice = NULL;
    IAudioClient* pAudioClient = NULL;
    IAudioCaptureClient* pCaptureClient = NULL;
    
    WAVEFORMATEX* pwfx = NULL;
    
    UINT32 packetLength = 0;
    
    BOOL bDone = FALSE;
    
    BYTE* pData;
    
    DWORD flags;

    hr = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&pEnumerator);
    EXIT_ON_ERROR(hr);

    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    if (pDevice == nullptr) {
        printf("\nDevice not found.");
    }
    EXIT_ON_ERROR(hr);

    hr = pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pAudioClient);
    EXIT_ON_ERROR(hr);

    hr = pAudioClient->GetMixFormat(&pwfx);
    EXIT_ON_ERROR(hr);

    hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, hnsRequestedDuration, 0, pwfx, NULL);
    EXIT_ON_ERROR(hr);

    // Get the size of the allocated buffer.
    hr = pAudioClient->GetBufferSize(&bufferFrameCount);
    EXIT_ON_ERROR(hr);

    hr = pAudioClient->GetService(IID_IAudioCaptureClient, (void**)&pCaptureClient);
    EXIT_ON_ERROR(hr);
    
    // Notify the audio sink which format to use.

    // Calculate the actual duration of the allocated buffer.
    hnsActualDuration = (double)REFTIMES_PER_SEC *
    bufferFrameCount / pwfx->nSamplesPerSec;

    hr = pAudioClient->Start();  // Start recording.
    EXIT_ON_ERROR(hr);

    // Each loop fills about half of the shared buffer.
    while (bDone == FALSE){
        // Sleep for half the buffer duration.
        Sleep(hnsActualDuration / REFTIMES_PER_MILLISEC / 2);

        hr = pCaptureClient->GetNextPacketSize(&packetLength);
        EXIT_ON_ERROR(hr);

        while (packetLength != 0){
            // Get the available data in the shared buffer.
            hr = pCaptureClient->GetBuffer(&pData, &numFramesAvailable, &flags, NULL, NULL);
            EXIT_ON_ERROR(hr);

            if (flags & AUDCLNT_BUFFERFLAGS_SILENT){
                printf("\nNo Audio");
            }
            else {
                printf("\nAudio Detected.");
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