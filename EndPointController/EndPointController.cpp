// EndPointController.cpp : Defines the entry point for the console application.
//
#include <stdio.h>
#include <wchar.h>
#include <tchar.h>
#include <string>
#include <iostream>
#include <locale>
#include "windows.h"
#include "Mmdeviceapi.h"
#include "PolicyConfig.h"
#include "Propidl.h"
#include "Functiondiscoverykeys_devpkey.h"

// Format default string for outputing a device entry. The following parameters will be used in the following order:
// Index, Device Friendly Name
#define DEVICE_OUTPUT_FORMAT "Audio Device %d: %ws"

enum OPTION
{
	OPTION_LIST_DEVICES,
	OPTION_SET_BY_INDEX,
	OPTION_SET_BY_FRIENDLY_NAME,
	OPTION_SET_BY_DEVICE_ID
};

typedef struct TGlobalState
{
	HRESULT hr;
	int option;
	int indexFilter;
	std::wstring nameFilter;
	std::wstring idFilter;
	IMMDeviceEnumerator *pEnum;
	IMMDeviceCollection *pDevices;
	LPWSTR strDefaultDeviceID;
	IMMDevice *pCurrentDevice;
	LPCWSTR pDeviceFormatStr;
	int deviceStateFilter;
} TGlobalState;

void createDeviceEnumerator(TGlobalState* state);
void prepareDeviceEnumerator(TGlobalState* state);
void enumerateOutputDevices(TGlobalState* state);
HRESULT printDeviceInfo(IMMDevice* pDevice, int index, LPCWSTR outFormat, LPWSTR strDefaultDeviceID);
std::wstring getDeviceProperty(IPropertyStore* pStore, const PROPERTYKEY key);
HRESULT SetDefaultAudioPlaybackDevice(LPCWSTR devID);
void invalidParameterHandler(const wchar_t* expression, const wchar_t* function, const wchar_t* file, 
	unsigned int line, uintptr_t pReserved);

// EndPointController.exe [NewDefaultDeviceID]
int _tmain(int argc, LPCWSTR argv[])
{
	TGlobalState state;

	// Set the locale to system's locale
	if (setlocale(LC_CTYPE, "") == NULL) {
		wprintf_s(_T("Cannot set the locale."));
		exit(1);
	}

	// Process command line arguments
	state.option = OPTION_LIST_DEVICES;
	state.strDefaultDeviceID = '\0';
	state.pDeviceFormatStr = _T(DEVICE_OUTPUT_FORMAT);
	state.deviceStateFilter = DEVICE_STATE_ACTIVE;

	for (int i = 1; i < argc; i++) 
	{
		LPCWSTR arg = argv[i];
		if (wcscmp(argv[i], _T("--help")) == 0)
		{
			wprintf_s(_T("Lists active audio end-point playback devices or sets default audio end-point\n"));
			wprintf_s(_T("playback device.\n\n"));
			wprintf_s(_T("USAGE\n"));
			wprintf_s(_T("  EndPointController.exe [-a] [-f format_str]  Lists audio end-point playback\n"));
			wprintf_s(_T("                                               devices that are enabled.\n"));
			wprintf_s(_T("  EndPointController.exe device_index          Sets the default playvack device\n"));
			wprintf_s(_T("                                               with the given index.\n"));
			wprintf_s(_T("  EndPointController.exe -n friendly_name      Sets the default playback device\n"));
			wprintf_s(_T("                                               with the given friendly name.\n"));
			wprintf_s(_T("  EndPointController.exe -id device_id         Sets the default playback device\n"));
			wprintf_s(_T("                                               with the given device id.\n"));
			wprintf_s(_T("\n"));
			wprintf_s(_T("OPTIONS\n"));
			wprintf_s(_T("  -a             Display all devices, rather than just active devices.\n"));
			wprintf_s(_T("  -f format_str  Outputs the details of each device using the given format\n"));
			wprintf_s(_T("                 string. If this parameter is ommitted the format string\n"));
			wprintf_s(_T("                 defaults to: \"%s\"\n\n"), _T(DEVICE_OUTPUT_FORMAT));
			wprintf_s(_T("                 Parameters that are passed to the 'printf' function are\n"));
			wprintf_s(_T("                 ordered as follows:\n"));
			wprintf_s(_T("                   - Device index (int)\n"));
			wprintf_s(_T("                   - Device friendly name (wstring)\n"));
			wprintf_s(_T("                   - Device state (int)\n"));
			wprintf_s(_T("                   - Device default? (1 for true 0 for false as int)\n"));
			wprintf_s(_T("                   - Device description (wstring)\n"));
			wprintf_s(_T("                   - Device interface friendly name (wstring)\n"));
			wprintf_s(_T("                   - Device ID (wstring)\n"));
			exit(0);
		}
		else if (wcscmp(argv[i], _T("-a")) == 0)
		{
			state.deviceStateFilter = DEVICE_STATEMASK_ALL;
			continue;
		}
		else if (wcscmp(argv[i], _T("-f")) == 0)
		{
			if ((argc - i) >= 2) {
				state.pDeviceFormatStr = argv[++i];
				
				// If printf is called with an invalid format string, jump to the invalidParameterHandler function.
				_set_invalid_parameter_handler(invalidParameterHandler);
				_CrtSetReportMode(_CRT_ASSERT, 0);
				continue;
			}
			else
			{
				wprintf_s(_T("Missing format string"));
				exit(1);
			}
		}
		else if (wcscmp(argv[i], _T("-n")) == 0)
		{
			if ((argc - i) >= 2) {
				state.option = OPTION_SET_BY_FRIENDLY_NAME;
				state.nameFilter = argv[++i];
				while ((argc - i) >= 2)
				{
					state.nameFilter += L" " + std::wstring(argv[++i]);
				}
				if (state.nameFilter[0] == L'"')
				{
					state.nameFilter.erase(0);
				}
				if (state.nameFilter[state.nameFilter.length() - 1] == L'"')
				{
					state.nameFilter.erase(state.nameFilter.length() - 1);
				}

				break;
			} else
			{
				wprintf_s(_T("Missing friendly_name string"));
				exit(1);
			}
		}
		else if (wcscmp(argv[i], _T("-id")) == 0)
		{
			if ((argc - i) >= 2) {
				//printf("argc %d\n", argc);
				state.option = OPTION_SET_BY_DEVICE_ID;
				state.idFilter = argv[++i];
				if (state.idFilter[0] == L'"')
				{
					state.idFilter.erase(0);
				}
				if (state.idFilter[state.idFilter.length() - 1] == L'"')
				{
					state.idFilter.erase(state.nameFilter.length() - 1);
				}
				break;
			}
			else
			{
				wprintf_s(_T("Missing device_id string"));
				exit(1);
			}
		}
	}
	
	if (argc == 2)
	{
		state.option = OPTION_SET_BY_INDEX;
		state.indexFilter = _wtoi(argv[1]);
	}

	state.hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	if (SUCCEEDED(state.hr))
	{
		createDeviceEnumerator(&state);
	}
	return state.hr;
}

// Create a multimedia device enumerator.
void createDeviceEnumerator(TGlobalState* state)
{
	state->pEnum = NULL;
	state->hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
		(void**)&state->pEnum);
	if (SUCCEEDED(state->hr))
	{
		prepareDeviceEnumerator(state);
	}
}

// Prepare the device enumerator
void prepareDeviceEnumerator(TGlobalState* state)
{
	state->hr = state->pEnum->EnumAudioEndpoints(eRender, state->deviceStateFilter, &state->pDevices);
	if SUCCEEDED(state->hr)
	{
		enumerateOutputDevices(state);
	}
	state->pEnum->Release();
}

// Enumerate the output devices
void enumerateOutputDevices(TGlobalState* state)
{
	UINT count;
	state->pDevices->GetCount(&count);

	// If option is less than 1, list devices
	if (state->option == OPTION_LIST_DEVICES)
	{

		// Get default device
		IMMDevice* pDefaultDevice;
		state->hr = state->pEnum->GetDefaultAudioEndpoint(eRender, eMultimedia, &pDefaultDevice);
		if (SUCCEEDED(state->hr))
		{
			state->hr = pDefaultDevice->GetId(&state->strDefaultDeviceID);

			// Iterate all devices
			for (int i = 1; i <= (int)count; i++)
			{
				state->hr = state->pDevices->Item(i - 1, &state->pCurrentDevice);
				if (SUCCEEDED(state->hr))
				{
					state->hr = printDeviceInfo(state->pCurrentDevice, i, state->pDeviceFormatStr,
						state->strDefaultDeviceID);
					state->pCurrentDevice->Release();
				}
			}
		}
	}
	// If option corresponds with the index of an audio device, set it to default
	else if (state->option == OPTION_SET_BY_INDEX)
	{
		if (state->indexFilter <= (int)count)
		{
			state->hr = state->pDevices->Item(state->indexFilter - 1, &state->pCurrentDevice);
			if (SUCCEEDED(state->hr))
			{
				LPWSTR strID = NULL;
				state->hr = state->pCurrentDevice->GetId(&strID);
				if (SUCCEEDED(state->hr))
				{
					state->hr = SetDefaultAudioPlaybackDevice(strID);
				}
				state->pCurrentDevice->Release();
			}
		}
		// Otherwise inform user than option doesn't correspond with a device
		else
		{
			wprintf_s(_T("Error: No audio end-point device with the index '%d'.\n"), state->indexFilter);
		}
	}
	else if (state->option == OPTION_SET_BY_FRIENDLY_NAME)
	{
		// Iterate all devices
		for (int i = 1; i <= (int)count; i++)
		{
			state->hr = state->pDevices->Item(i - 1, &state->pCurrentDevice);
			if (SUCCEEDED(state->hr))
			{
				LPWSTR strID = NULL;
				state->hr = state->pCurrentDevice->GetId(&strID);
				if (SUCCEEDED(state->hr))
				{
					IPropertyStore *pStore;
					state->hr = state->pCurrentDevice->OpenPropertyStore(STGM_READ, &pStore);
					if (SUCCEEDED(state->hr))
					{
						std::wstring friendlyName = getDeviceProperty(pStore, PKEY_Device_FriendlyName);
						if (state->nameFilter == friendlyName)
						{
							state->hr = SetDefaultAudioPlaybackDevice(strID);
							state->pCurrentDevice->Release();
							break;
						}
					}
				}
				state->pCurrentDevice->Release();
			}
		}
	}
	else if (state->option == OPTION_SET_BY_DEVICE_ID)
	{
		// Iterate all devices
		for (int i = 1; i <= (int)count; i++)
		{
			state->hr = state->pDevices->Item(i - 1, &state->pCurrentDevice);
			if (SUCCEEDED(state->hr))
			{
				LPWSTR strID = NULL;
				state->hr = state->pCurrentDevice->GetId(&strID);
				if (SUCCEEDED(state->hr))
				{
					if (state->idFilter.compare(strID) == 0)
					{
						//printf("%ws == %ws\n", state->idFilter.c_str(), strID);
						state->hr = SetDefaultAudioPlaybackDevice(strID);
						state->pCurrentDevice->Release();
						break;
					}
				}
				state->pCurrentDevice->Release();
			}
		}
	}
	
	state->pDevices->Release();
}

HRESULT printDeviceInfo(IMMDevice* pDevice, int index, LPCWSTR outFormat, LPWSTR strDefaultDeviceID)
{
	// Device ID
	LPWSTR strID = NULL;
	HRESULT hr = pDevice->GetId(&strID);
	if (!SUCCEEDED(hr))
	{
		return hr;
	}

	int deviceDefault = (strDefaultDeviceID != '\0' && (wcscmp(strDefaultDeviceID, strID) == 0));

	// Device state
	DWORD dwState;
	hr = pDevice->GetState(&dwState);
	if (!SUCCEEDED(hr))
	{
		return hr;
	}
		
	IPropertyStore *pStore;
	hr = pDevice->OpenPropertyStore(STGM_READ, &pStore);
	if (SUCCEEDED(hr))
	{
		std::wstring friendlyName = getDeviceProperty(pStore, PKEY_Device_FriendlyName);
		std::wstring Desc = getDeviceProperty(pStore, PKEY_Device_DeviceDesc);
		std::wstring interfaceFriendlyName = getDeviceProperty(pStore, PKEY_DeviceInterface_FriendlyName);
		
		if (SUCCEEDED(hr))
		{
			wprintf_s(outFormat,
				index,
				friendlyName.c_str(),
				dwState,
				deviceDefault,
				Desc.c_str(),
				interfaceFriendlyName.c_str(),
				strID
			);
			wprintf_s(_T("\n"));
			fflush(stdout);
		}

		pStore->Release();
	}
	return hr;
}

std::wstring getDeviceProperty(IPropertyStore* pStore, const PROPERTYKEY key)
{
	PROPVARIANT prop;
	PropVariantInit(&prop);
	HRESULT hr = pStore->GetValue(key, &prop);
	if (SUCCEEDED(hr))
	{
		std::wstring result (prop.pwszVal);
		PropVariantClear(&prop);
		return result;
	}
	else
	{
		return std::wstring (L"");
	}
}

HRESULT SetDefaultAudioPlaybackDevice(LPCWSTR devID)
{	
	IPolicyConfigVista *pPolicyConfig;
	ERole reserved = eConsole;

    HRESULT hr = CoCreateInstance(__uuidof(CPolicyConfigVistaClient), 
		NULL, CLSCTX_ALL, __uuidof(IPolicyConfigVista), (LPVOID *)&pPolicyConfig);
	if (SUCCEEDED(hr))
	{
		hr = pPolicyConfig->SetDefaultEndpoint(devID, reserved);
		pPolicyConfig->Release();
	}
	return hr;
}

void invalidParameterHandler(const wchar_t* expression,
   const wchar_t* function, 
   const wchar_t* file, 
   unsigned int line, 
   uintptr_t pReserved)
{
   wprintf_s(_T("\nError: Invalid format_str.\n"));
   exit(1);
}