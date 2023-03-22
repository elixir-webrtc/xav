# Xav

Elixir wrapper over FFmpeg for reading audio and video files.

See an interview with FFmpeg enthusiast:  https://youtu.be/9kaIXkImCAM

## Installation

Xav requires development packages of FFmpeg to be installed on your system.


### Fedora

```bash
dnf install ffmpeg-devel ffmpeg-libs
export C_INCLUDE_PATH=$C_INCLUDE_PATH:/usr/include/ffmpeg
```

### Ubuntu

```bash
apt install libavcodec-dev libavformat-dev libavutil-dev libswscale-dev libavdevice-dev
```

### Windows

Windows is not supported but PRs are welcomed.

And include Xav in your list of dependencies:

```elixir
def deps do
  [
    {:xav, "~> 0.1.0"}
  ]
end
```

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
* [x] - tests
* [x] - README
* [ ] - streams
* [x] - camera input 
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
