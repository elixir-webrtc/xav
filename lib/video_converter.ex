defmodule Xav.VideoConverter do
  @moduledoc """
  Video samples converter.

  Currently it only supports pixel format conversion.
  """

  alias Xav.Frame
  alias Xav.VideoConverter.NIF

  @type t :: %__MODULE__{format: Frame.video_format(), converter: reference()}

  @typedoc """
  Type definition for converter options.

  * `format` - video format to convert to (`e.g. :rgb24`).
  """
  @type converter_opts() :: [format: Frame.video_format()]

  @enforce_keys [:format]
  defstruct [:format, :converter]

  @doc """
  Creates a new video converter.
  """
  @spec new(converter_opts()) :: {:ok, t()} | {:error, any()}
  def new(converter_opts) do
    with {:ok, converter} <- NIF.new(converter_opts[:format]) do
      {:ok, %__MODULE__{format: converter_opts[:format], converter: converter}}
    end
  end

  @doc """
  Same as `new/1` but raises an exception in case of an error.
  """
  @spec new!(converter_opts()) :: t()
  def new!(converter_opts) do
    case new(converter_opts) do
      {:ok, ref} -> ref
      {:error, reason} -> raise "Couldn't create a video converter. Reason: #{inspect(reason)}"
    end
  end

  @doc """
  Converts a video frame.
  """
  @spec convert(t(), Frame.t()) :: Frame.t()
  def convert(%__MODULE__{format: format}, %Frame{format: format} = frame), do: frame

  def convert(%__MODULE__{converter: converter}, frame) do
    {data, out_format, width, height, _pts} =
      NIF.convert(converter, frame.data, frame.width, frame.height, frame.format)

    %Frame{
      type: frame.type,
      data: data,
      format: out_format,
      width: width,
      height: height,
      pts: frame.pts
    }
  end
end
