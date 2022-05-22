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

void EnumEndpoints() {
	CoInitialize(nullptr);
	//^this line caused so much pain (;-;)

	HRESULT hr = S_OK;
	const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
	const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
	IMMDeviceEnumerator* pEnum = NULL;
	IMMDeviceCollection* pCollection = NULL;
	IMMDevice* pDevice = NULL;
	IPropertyStore* pProps = NULL;
	LPWSTR pwszID = NULL;

	//Instantiate an IMMDeviceEnumerator object
	std::cout << "Creating Enumerator.";
	hr = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&pEnum);
	EXIT_ON_ERROR(hr);

		//use enumerator to put the collection of devices into the IMMDeviceCollection store
		std::cout << "\nEnumerating Endpoints.";
	hr = pEnum->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pCollection);
	EXIT_ON_ERROR(hr);

		UINT count;
	//get count of devices
	std::cout << "\nGetting count of devices.";
	hr = pCollection->GetCount(&count);
	EXIT_ON_ERROR(hr);

	//print number of devices found
	std::cout << "\nNumber of devices: " << count;

	//print the names of devices
	for (ULONG i = 0; i < count; i++) {

		//get an item from the collection
		hr = pCollection->Item(i, &pDevice);
		EXIT_ON_ERROR(hr);

		//get device id string
		hr = pDevice->GetId(&pwszID);
		EXIT_ON_ERROR(hr);

		//get the property store for the device
		hr = pDevice->OpenPropertyStore(STGM_READ, &pProps);
		EXIT_ON_ERROR(hr);

		PROPVARIANT varName;
		PropVariantInit(&varName);

		//get the user-friendly name of the device
		hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
		EXIT_ON_ERROR(hr);

		//print the current device to the console
		printf("\nEndpoint device %d: %S", i, varName.pwszVal);
		
		//release interfaces for next iteration
		CoTaskMemFree(pwszID);
		pwszID = NULL;
		PropVariantClear(&varName);
		SAFE_RELEASE(pProps);
		SAFE_RELEASE(pDevice);
	}
	SAFE_RELEASE(pEnum);
	SAFE_RELEASE(pCollection);

Exit:
	CoTaskMemFree(pwszID);
	SAFE_RELEASE(pEnum);
	SAFE_RELEASE(pCollection);
	SAFE_RELEASE(pProps);
	SAFE_RELEASE(pDevice);
	CoUninitialize();
}

int main() {
	EnumEndpoints();
	return 0;
}