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
    {:xav, "~> 0.11.0"},
    # Add Nx if you want to have Xav.Frame.to_nx/1
    {:nx, ">= 0.0.0"}
  ]
end
```

## Usage

Decode

```elixir
decoder = Xav.Decoder.new(:vp8, out_format: :rgb24)
{:ok, %Xav.Frame{} = frame} = Xav.Decoder.decode(decoder, <<"somebinary">>)
```

Decode with audio resampling

```elixir
decoder = Xav.Decoder.new(:opus, out_format: :flt, out_sample_rate: 16_000)
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
r = Xav.Reader.new!("/dev/video0", device?: true, out_format: :rgb24)
{:ok, %Xav.Frame{} = frame} = Xav.Reader.next_frame(r)
tensor = Xav.Frame.to_nx(frame)
Kino.Image.new(tensor)
```

Speech to text:

```elixir
{:ok, whisper} = Bumblebee.load_model({:hf, "openai/whisper-tiny"})
{:ok, featurizer} = Bumblebee.load_featurizer({:hf, "openai/whisper-tiny"})
{:ok, tokenizer} = Bumblebee.load_tokenizer({:hf, "openai/whisper-tiny"})
{:ok, generation_config} = Bumblebee.load_generation_config({:hf, "openai/whisper-tiny"})

serving =
  Bumblebee.Audio.speech_to_text_whisper(whisper, featurizer, tokenizer, generation_config,
    defn_options: [compiler: EXLA]
  )

# Read a couple of frames.
# See https://hexdocs.pm/bumblebee/Bumblebee.Audio.WhisperFeaturizer.html for default sampling rate.
frames =
    Xav.Reader.stream!("sample.mp3", read: :audio, out_format: :flt, out_channels: 1, out_sample_rate: 16_000)
    |> Stream.take(200)
    |> Enum.map(fn frame -> Xav.Frame.to_nx(frame) end)

batch = Nx.Batch.concatenate(frames)
batch = Nx.Defn.jit_apply(&Function.identity/1, [batch])
Nx.Serving.run(serving, batch) 
```

## Development

To make `clangd` aware of the header files used in your project, you can create a `compile_commands.json` file. 
`clangd` uses this file to know the compiler flags, include paths, and other compilation options for each source file. 

### Install bear

The easiest way to generate `compile_commands.json` from a Makefile is to use the `bear` tool. `bear` is a tool that records the compiler calls during a build and creates the `compile_commands.json` file.

You can install `bear` with your package manager:

- __macOS__: brew install bear
- __Ubuntu/Debian__: sudo apt install bear
- __Fedora__: sudo dnf install bear

### Generate compile_commands.json

After installing bear, you can run it alongside your make command to capture the necessary information.

```bash
bear -- mix compile
```
