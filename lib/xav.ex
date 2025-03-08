defmodule Xav do
  @moduledoc File.read!("README.md")

  @type encoder :: %{
          codec: atom(),
          name: atom(),
          long_name: String.t(),
          media_type: atom(),
          profiles: [String.t()],
          sample_formats: [atom()]
        }

  @type decoder :: %{
          codec: atom(),
          name: atom(),
          long_name: String.t(),
          media_type: atom()
        }

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
  """
  @spec list_decoders() :: [decoder()]
  def list_decoders() do
    Xav.Decoder.NIF.list_decoders()
    |> Enum.map(fn {codec, name, long_name, media_type} ->
      %{
        codec: codec,
        name: name,
        long_name: List.to_string(long_name),
        media_type: media_type
      }
    end)
    |> Enum.reverse()
  end

  @doc """
  List all encoders.
  """
  @spec list_encoders() :: [encoder()]
  def list_encoders() do
    Xav.Encoder.NIF.list_encoders()
    |> Enum.map(fn {family_name, name, long_name, media_type, _codec_id, profiles, sample_formats,
                    sample_rates} ->
      %{
        codec: family_name,
        name: name,
        long_name: List.to_string(long_name),
        media_type: media_type,
        profiles: profiles |> Enum.map(&List.to_string/1) |> Enum.reverse(),
        sample_formats: Enum.reverse(sample_formats),
        sample_rates: Enum.reverse(sample_rates)
      }
    end)
    |> Enum.reverse()
  end
end
