# Xav

[![Documentation](https://img.shields.io/badge/-Documentation-blueviolet)](https://hexdocs.pm/xav/)

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
r = Xav.new_reader!("./some_mp4_file.mp4")
{:ok, %Xav.Frame{} = frame} = Xav.next_frame(r)
tensor = Xav.Frame.to_nx(frame)
Kino.Image.new(tensor)
```

Read from a camera:

```elixir
r = Xav.new_reader!("/dev/video0", device?: true)
{:ok, %Xav.Frame{} = frame} = Xav.next_frame(r)
tensor = Xav.Frame.to_nx(frame)
Kino.Image.new(tensor)
```

Speech to text:

```elixir
r = Xav.new_reader!("../sample.mp3", read: :audio)

{:ok, whisper} = Bumblebee.load_model({:hf, "openai/whisper-tiny"})
{:ok, featurizer} = Bumblebee.load_featurizer({:hf, "openai/whisper-tiny"})
{:ok, tokenizer} = Bumblebee.load_tokenizer({:hf, "openai/whisper-tiny"})

serving =
  Bumblebee.Audio.speech_to_text(whisper, featurizer, tokenizer,
    max_new_tokens: 100,
    defn_options: [compiler: EXLA]
  )

# read a couple of frames
frames =
  for _i <- 0..200 do
    {:ok, frame} = Xav.next_frame(r)
    Xav.to_nx(frame)
  end

batch = Nx.Batch.concatenate(frames)
batch = Nx.Defn.jit_apply(&Function.identity/1, [batch])
Nx.Serving.run(serving, batch) 
```

## LICENSE

Copyright 2023, Michał Śledź

Licensed under the [MIT](./LICENSE)
