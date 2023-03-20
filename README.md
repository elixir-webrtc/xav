# Xav

Elixir wrapper over FFmpeg for reading audio and video files.

## Installation

```elixir
def deps do
  [
    {:xav, "~> 0.1.0"}
  ]
end
```

### macOS, Windows and ARM

Currently, Xav ships with precompiled ffmpeg binaries only for x86_64-linux platform.
On other platforms, you have to install ffmpeg on your own and make sure that
paths to ffmpeg header files and ffmpeg shared libraries are searchable by 
C compiler and linker. In case of linux, you have to set properly `C_INCLUDE_PATH`
and `LIBRARY_PATH`.

## Usage

```elixir
r = Xav.new_reader("./some_mp4_file.mp4")
{:ok, %Xav.Frame{} = frame} = Xav.next_frame(r)
tensor = Xav.Frame.to_nx(frame)
Kino.Image.new(tensor)
```

## TODO

* [x] - one pkt can containe multiple frames
* [x] - pkt frame unref
* [x] - nif and ffmpeg code split up
* [x] - resource cleanup
* [ ] - seeking
* [ ] - tests
* [x] - README
* [ ] - streams
* [ ] - camera input 
* [x] - CI for linux
* [ ] - CI for windows and mac
* [x] - C debug logs
* [x] - handle EOF 
* [ ] - handle ffmpeg binaries
* [ ] - audio support
* [ ] - examples

## LICENSE

Copyright 2023, Michał Śledź

Licensed under the [MIT](./LICENSE)