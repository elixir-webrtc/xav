defmodule Xav.Reader do
  @moduledoc """
  Audio/video file reader.
  """

  @typedoc """
  Reader options.

  * `read` - determines which stream to read from a file.
  Defaults to `:video`.
  * `device?` - determines whether path points to the camera. Defaults to `false`.
  """
  @type opts :: [
          read: :audio | :video,
          device?: boolean,
          out_format: Xav.Frame.format(),
          out_sample_rate: integer(),
          out_channels: integer()
        ]

  @type t() :: %__MODULE__{
          reader: reference(),
          in_format: atom(),
          out_format: atom(),
          in_sample_rate: integer() | nil,
          out_sample_rate: integer() | nil,
          in_channels: integer() | nil,
          out_channels: integer() | nil,
          bit_rate: integer(),
          duration: integer(),
          codec: atom()
        }

  @enforce_keys [:reader, :in_format, :out_format, :bit_rate, :duration, :codec]
  defstruct @enforce_keys ++ [:in_sample_rate, :out_sample_rate, :in_channels, :out_channels]

  @doc """
  The same as new/1 but raises on error.
  """
  @spec new!(String.t(), opts()) :: t()
  def new!(path, opts \\ []) do
    case new(path, opts) do
      {:ok, reader} -> reader
      {:error, reason} -> raise "Couldn't create a new reader. Reason: #{inspect(reason)}"
    end
  end

  @doc """
  Creates a new audio/video reader.

  Both reading from a file and from a video camera are supported.
  In case of using a video camera, the v4l2 driver is required, and FPS are
  locked to 10.

  Microphone input is not supported.

  `opts` can be used to specify desired output parameters.
  Video frames are always returned in RGB format. This setting cannot be changed.
  Audio samples are always in the packed form.
  See `Xav.Decoder.new/2` for more information.
  """
  @spec new(String.t(), opts()) :: {:ok, t()} | {:error, term()}
  def new(path, opts \\ []) do
    read = opts[:read] || :video
    device? = opts[:device?] || false
    out_format = opts[:out_format]
    out_sample_rate = opts[:out_sample_rate] || 0
    out_channels = opts[:out_channels] || 0

    case Xav.Reader.NIF.new(
           path,
           to_int(device?),
           to_int(read),
           out_format,
           out_sample_rate,
           out_channels
         ) do
      {:ok, reader, in_format, out_format, in_sample_rate, out_sample_rate, in_channels,
       out_channels, bit_rate, duration, codec} ->
        {:ok,
         %__MODULE__{
           reader: reader,
           in_format: in_format,
           out_format: out_format,
           in_sample_rate: in_sample_rate,
           out_sample_rate: out_sample_rate,
           in_channels: in_channels,
           out_channels: out_channels,
           bit_rate: bit_rate,
           duration: duration,
           codec: to_human_readable(codec)
         }}

      {:ok, reader, in_format, out_format, bit_rate, duration, codec} ->
        {:ok,
         %__MODULE__{
           reader: reader,
           in_format: in_format,
           out_format: out_format,
           bit_rate: bit_rate,
           duration: duration,
           codec: to_human_readable(codec)
         }}

      {:error, _reason} = err ->
        err
    end
  end

  @doc """
  Reads and decodes the next frame.
  """
  @spec next_frame(t()) :: {:ok, Xav.Frame.t()} | {:error, :eof}
  def next_frame(%__MODULE__{reader: reader}) do
    case Xav.Reader.NIF.next_frame(reader) do
      {:ok, {data, format, width, height, pts}} ->
        {:ok, Xav.Frame.new(data, format, width, height, pts)}

      {:ok, {data, format, samples, pts}} ->
        {:ok, Xav.Frame.new(data, format, samples, pts)}

      {:error, :eof} = err ->
        err
    end
  end

  defp to_human_readable(:libdav1d), do: :av1
  defp to_human_readable(:mp3float), do: :mp3
  defp to_human_readable(other), do: other

  defp to_int(:video), do: 1
  defp to_int(:audio), do: 0
  defp to_int(true), do: 1
  defp to_int(false), do: 0
end
