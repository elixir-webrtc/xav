# Xav

[![Hex.pm](https://img.shields.io/hexpm/v/xav.svg)](https://hex.pm/packages/xav)
[![API Docs](https://img.shields.io/badge/api-docs-yellow.svg?style=flat)](https://hexdocs.pm/xav)
[![CI](https://img.shields.io/github/actions/workflow/status/elixir-webrtc/xav/ci.yml?logo=github&label=CI)](https://github.com/elixir-webrtc/xav/actions/workflows/ci.yml)
[![codecov](https://codecov.io/gh/elixir-webrtc/xav/graph/badge.svg?token=2AG2acRhOf)](https://codecov.io/gh/elixir-webrtc/xav)

Elixir wrapper over FFmpeg for reading audio and video files.

See an interview with FFmpeg enthusiast:  https://youtu.be/9kaIXkImCAM

## Installation

Make sure you have installed FFMpeg (ver. 4.x - 7.x) development packages on your system
(see [here](INSTALL.md) for installation one-liners) and add Xav to the list of your dependencies:

```elixir
def deps do
  [
    {:xav, "~> 0.4.0"}
  ]
end
```

## Usage

Decode

```elixir
decoder = Xav.Decoder.new(:vp8)
{:ok, %Xav.Frame{} = frame} = Xav.Decoder.decode(decoder, <<"somebinary">>)
```

Read from a file:

```elixir
r = Xav.Reader.new!("./some_mp4_file.mp4")
{:ok, %Xav.Frame{} = frame} = Xav.Reader.next_frame(r)
tensor = Xav.Frame.to_nx(frame)
Kino.Image.new(tensor)
```

Read from a camera:

```elixir
r = Xav.Reader.new!("/dev/video0", device?: true)
{:ok, %Xav.Frame{} = frame} = Xav.Reader.next_frame(r)
tensor = Xav.Frame.to_nx(frame)
Kino.Image.new(tensor)
```

Speech to text:

```elixir
r = Xav.Reader.new!("sample.mp3", read: :audio)

{:ok, whisper} = Bumblebee.load_model({:hf, "openai/whisper-tiny"})
{:ok, featurizer} = Bumblebee.load_featurizer({:hf, "openai/whisper-tiny"})
{:ok, tokenizer} = Bumblebee.load_tokenizer({:hf, "openai/whisper-tiny"})
{:ok, generation_config} = Bumblebee.load_generation_config({:hf, "openai/whisper-tiny"})

serving =
  Bumblebee.Audio.speech_to_text_whisper(whisper, featurizer, tokenizer, generation_config,
    defn_options: [compiler: EXLA]
  )

# read a couple of frames
frames =
  for _i <- 0..200 do
    {:ok, frame} = Xav.Reader.next_frame(r)
    Xav.Frame.to_nx(frame)
  end

batch = Nx.Batch.concatenate(frames)
batch = Nx.Defn.jit_apply(&Function.identity/1, [batch])
Nx.Serving.run(serving, batch) 
```