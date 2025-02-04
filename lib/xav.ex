defmodule Xav do
  @moduledoc File.read!("README.md")

  @doc """
  Get all available pixel formats.

  The result is a list of 3-element tuples `{name, nb_components, hw_accelerated_format?}`:
    * `name` - The name of the pixel format.
    * `nb_components` - The number of the components in the pixel format.
    * `hw_accelerated_format?` - Whether the pixel format is a hardware accelerated format.
  """
  @spec pixel_formats() :: [{atom(), integer(), boolean()}]
  def pixel_formats(), do: Xav.Decoder.NIF.pixel_formats() |> Enum.reverse()

  @doc """
  Get all available audio sample formats.

  The result is a list of 2-element tuples `{name, nb_bytes}`:
    * `name` - The name of the sample format.
    * `nb_bytes` - The number of bytes per sample.
  """
  @spec sample_formats() :: [{atom(), integer()}]
  def sample_formats(), do: Xav.Decoder.NIF.sample_formats() |> Enum.reverse()

  @doc """
  List all decoders.

  The result is a list of 3-element tuples `{name, long_name, media_type}`:
    * `name` - The short name of the decoder.
    * `long_name` - The long name of the decoder.
    * `media_type` - The media type of the decoder.
  """
  @spec list_decoders() :: [{name :: atom(), long_name :: String.t(), media_type :: atom()}]
  def list_decoders() do
    Xav.Decoder.NIF.list_decoders()
    |> Enum.map(fn {name, long_name, media_type} ->
      {name, List.to_string(long_name), media_type}
    end)
    |> Enum.reverse()
  end
end
