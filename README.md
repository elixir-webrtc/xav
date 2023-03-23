# Xav

Elixir wrapper over FFmpeg for reading audio and video files.

See an interview with FFmpeg enthusiast:  https://youtu.be/9kaIXkImCAM

## Installation

Make sure you have installed FFMpeg development packages on your system
(see [here](INSTALL.md) for installation one-liners) and add Xav to the list of your dependencies:

```elixir
def deps do
  [
    {:xav, "~> 0.1"}
  ]
end
```

## Usage

Read from a file:

```elixir
r = Xav.new_reader("./some_mp4_file.mp4")
{:ok, %Xav.Frame{} = frame} = Xav.next_frame(r)
tensor = Xav.Frame.to_nx(frame)
Kino.Image.new(tensor)
```

Read from your camera:

```elixir
r = Xav.new_reader("/dev/video0", true)
{:ok, %Xav.Frame{} = frame} = Xav.next_frame(r)
tensor = Xav.Frame.to_nx(frame)
Kino.Image.new(tensor)
```

## LICENSE

Copyright 2023, Michał Śledź

Licensed under the [MIT](./LICENSE)
