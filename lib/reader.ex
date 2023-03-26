defmodule Xav.Reader do
  @moduledoc """
  File/network reader.
  """

  @type audio_format() :: atom()
  @type video_format() :: :rgb

  @type t() :: %__MODULE__{
          reader: reference(),
          format: audio_format() | video_format(),
          sample_rate: integer() | nil,
          bit_rate: integer(),
          duration: integer(),
          codec: atom()
        }

  @enforce_keys [:reader, :format, :bit_rate, :duration, :codec]
  defstruct @enforce_keys ++ [:sample_rate]

  @doc """
  The same as new/1 but raises on error.
  """
  @spec new!(String.t(), boolean(), boolean()) :: t()
  def new!(path, video? \\ true, device? \\ false) do
    case new(path, video?, device?) do
      {:ok, reader} -> reader
      {:error, reason} -> raise "Couldn't create a new reader. Reason: #{inspect(reason)}"
    end
  end

  @doc """
  Creates a new file reader.
  """
  @spec new(String.t(), boolean(), boolean()) :: {:ok, t()} | {:error, term()}
  def new(path, video? \\ true, device? \\ false) do
    case Xav.NIF.new_reader(path, to_int(device?), to_int(video?)) do
      {:ok, reader, format, sample_rate, bit_rate, duration, codec} ->
        {:ok,
         %__MODULE__{
           reader: reader,
           format: format,
           sample_rate: sample_rate,
           bit_rate: bit_rate,
           duration: duration,
           codec: to_human_readable(codec)
         }}

      {:ok, reader, format, bit_rate, duration, codec} ->
        {:ok,
         %__MODULE__{
           reader: reader,
           format: format,
           bit_rate: bit_rate,
           duration: duration,
           codec: to_human_readable(codec)
         }}

      {:error, _reason} = err ->
        err
    end
  end

  @doc """
  Reads next video frame.

  Frame is always decoded. Pixel format is always RGB.
  See also `Xav.Frame.to_nx/1`.
  """
  @spec next_frame(t()) :: {:ok, Xav.Frame.t()} | {:error, :eof}
  def next_frame(%__MODULE__{reader: reader}) do
    case Xav.NIF.next_frame(reader) do
      {:ok, {data, width, height, pts}} ->
        {:ok, Xav.Frame.new(data, width, height, pts)}

      {:error, :eof} = err ->
        err
    end
  end

  defp to_human_readable(:libdav1d), do: :av1
  defp to_human_readable(:mp3float), do: :mp3
  defp to_human_readable(other), do: other

  defp to_int(true), do: 1
  defp to_int(false), do: 0
end
