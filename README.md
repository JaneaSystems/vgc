# Video GIF Capture

An easy to use tool for screen capture and export into GIF and MP4 formats.

## Solution architecture

The solution was developed and should be compiled using Microsoft Visual Studio 2019.
The target operating system is Windows 10. The compiled output should be used as a PowerToys module,
or a standalone executable. The solution consists of five projects, described herein.
All projects which use C++ use the latest C++ standard, C++20, to the degree supported by the IDE
and compiler.

## VGC Core library

This is a C++ project and it compiles into a static library to be used in the main application.
It contains functions and classes which wrap existing relevant Windows APIs where they exist,
and implement various low level processing algorithms where they don't, or the APIs are undesirable.
To ensure the best performance on modern hardware, all operations which could benefit from many
CPU cores should be implemented using multithreading.

For screen capture and low-level graphics processing, Direct3D 11 is used. For exporting files
using the GIF format, a custom, extensible library is used and its source code is available as
part of the project. WASAPI is used for audio capture. Loopback capture is supported.

These OS interop functions are a big potential source of bugs. Bugs are most likely to occur
when available system memory is low, the primary storage device is slow, or when captured devices'
settings are changed on-the-fly, for example, when changing the screen resolution or when unplugging
an audio device. Great attention should be paid to these scenarios. Ideally, the recording, up
to the point of failure, should be recoverable, but hard crashes should be absolutely avoided.  

Due to large memory requirements of video capture, captured frames are written to the disk as
lossless PNG images. The frames may optionally be compressed further using an image difference
algorithm. In case RAM is filling up and disk writing speed is limited, the rate of capture (FPS)
is automatically reduced. For shorter snippets, users shouldn't have any problems even when
running on older hardware.

For MP4 export, `ffmpeg` should be present on the machine. You can use an existing installation
or choose to download it from the internet. It's not required for other features to work.

## VGC Main executable

This is also a C++ project and it contains mostly UI code. The UI shall have a native Windows 10
look and feel. There shall be two views.

1. Recording panel view, which contains controls to start/stop recording and for setting up the recording.
2. Editor view, which displays the captured output and allows the user to edit, export or discard it.

Only simple editing operations are supported.

## MSVC Unit test project

This project shall contain unit tests, mostly for the VGC Core library.

## PowerToys DLL project

This project compiles to a dynamically linked library and it implements the PowerToys module interface.
The DLL is responsible for running and managing the main executable.

## PowerToys settings page

The settings page should contain global user preferences, for example, keyboard shortcuts, options
for visual appearance, location of ffmpeg, a button to set it up automatically, additional options for ffmpeg,
default recording options, et cetera.
