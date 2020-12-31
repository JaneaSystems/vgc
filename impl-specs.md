# Video GIF Capture

An easy to use tool for screen capture and export into GIF and MP4 formats.

## Pre-implementation specification

### Technologies used
* C++ core language and standard template library, C++20 standard to the degree supported by Visual Studio.
* Direct3D 11 for screen capture, low-level graphics processing and displaying.
* WASAPI for sound recording and playback.
* WinUI 3 for the UI.
* PNG import/export using the COM library.

### Other dependencies
* `ffmpeg` for exporting to mp4. Consider whether we should ship it with PowerToys.
* NeuralQuantizer implementation from ScreenToGif for the best GIF visual quality - needs to be reimplemented in C++.
* Snip and Sketch code for zone/window selection.

### Testing and QA
* Unit tests for all core algorithms, using a Visual Studio C++ unit test project.
* Consider how to automatically test how VGC reacts to hardware changes during recording, since this is a potential source of bugs.
