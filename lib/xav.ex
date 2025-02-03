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
end
