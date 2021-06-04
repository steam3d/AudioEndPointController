This is a fork of [DanStevens/AudioEndPointController](https://github.com/DanStevens/AudioEndPointController),
which lets you change the default end-point by supplying the [friendly name](https://msdn.microsoft.com/en-us/library/windows/hardware/ff536394(v=vs.85).aspx)
of the device, since the relative "device index" tends to change when devices are attached/detached.

AudioEndPointController
=======================

A Windows command-line program for listing audio end-points and setting the default

	>EndPointController.exe --help
	Lists active audio end-point playback devices or sets default audio end-point
	playback device.

	USAGE
	  EndPointController.exe [-a] [-f format_str]  Lists audio end-point playback
												   devices that are enabled.
	  EndPointController.exe device_index          Sets the default playback device
												   with the given index.
	  EndPointController.exe -n friendly_name	   Sets the default playvack device
												   with the given friendly name.

	OPTIONS
	  -a             Display all devices, rather than just active devices.
	  -f format_str  Outputs the details of each device using the given format
					 string. If this parameter is ommitted the format string
					 defaults to: "Audio Device %d: %ws"

					 Parameters that are passed to the 'printf' function are
					 ordered as follows:
					   - Device index (int)
					   - Device friendly name (wstring)
					   - Device state (int)
					   - Device default? (1 for true 0 for false as int)
					   - Device description (wstring)
					   - Device interface friendly name (wstring)
					   - Device ID (wstring)
					   
   	> EndPointController.exe -f "%d %ws %d %d %ws %ws %ws"
	1 Головной телефон (Mi Pocket Speaker 2 Hands-Free AG Audio) 8 0 Головной телефон Mi Pocket Speaker 2 Hands-Free AG Audio {0.0.0.00000000}.{0016c7f4-0a72-43dd-a83e-aca1a4fb1c67}
	2  4 0 Digital Audio (HDMI)  {0.0.0.00000000}.{02af2d4f-2cd6-421a-9fc8-3ba3cc9cbb76}
	
