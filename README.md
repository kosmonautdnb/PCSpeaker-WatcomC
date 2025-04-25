# PCSpeaker-WatcomC
A versatile Mod/XM/Wav player for PC speakers in DOS (e.g., FreeDos).

It uses https://github.com/Konstanty/libmodplug to play sounds on PC speakers in DOS. Libmodplug is the "main feature" here so to say.

Libmodplug is public domain and just referenced here.

The source works with the public domain PMODE/W, which is included in a ZIP file in the repository.

PMODE/W can handle up to 256 MB of memory (needs pmwsetup.exe on your exe file), while Dos4GW only handles about 32 MB.

The formats: ".amf",".abc",".pat" don't work. Look into stdafx.h to fix these formats.
