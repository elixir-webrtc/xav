defmodule Xav.Reader do
  @moduledoc """
  Audio/video file reader.
  """

  @reader_options_schema [
    read: [
      type: {:in, [:audio, :video]},
      default: :video,
      doc: "The type of the stream to read from the input, either `video` or `audio`"
    ],
    device?: [
      type: :boolean,
      default: false,
      doc: "Whether the path points to the camera"
    ],
    out_format: [
      type: :atom,
      doc: """
      The output format of the audio samples. For a list of available
      sample formats check `Xav.sample_formats/0`.

      For video samples, it is always `:rgb24`.
      """
    ],
    out_sample_rate: [
      type: :pos_integer,
      doc: "The output sample rate of the audio samples"
    ],
    out_channels: [
      type: :pos_integer,
      doc: "The output number of channels of the audio samples"
    ],
    framerate: [
      type: {:tuple, [:non_neg_integer, :non_neg_integer]},
      default: {0,0},
      doc:
        "the framerate a a 2-element tuple with the first element beign the nominator and the second element the denominator. Will only be used when reading from a device."
    ],
    width: [type: :non_neg_integer, default: 0, doc: "the width of the device resolution. Only used when reading from a device."],
    height: [type: :non_neg_integer,default: 0,  doc: "the height of the device resolution. Only used when reading from a device."]
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
          codec: atom(),
          framerate: {integer(), integer()} | nil
        }

  @enforce_keys [:reader, :in_format, :out_format, :bit_rate, :duration, :codec]
  defstruct @enforce_keys ++
              [:in_sample_rate, :out_sample_rate, :in_channels, :out_channels, :framerate]

  @doc """
  The same as `new/1` but raises on error.
  """
  @spec new!(String.t(), Keyword.t()) :: t()
  def new!(path, opts \\ []) do
    case new(path, opts) do
      {:ok, reader} -> reader
      {:error, reason} -> raise "Couldn't create a new reader. Reason: #{inspect(reason)}"
    end
  end

  @doc """
  Creates a new audio/video reader.

  Both reading from a file and from a video camera are supported.

  Linux:
  In case of using a video camera, the v4l2 driver is required.

  macOS:
  In case of using a video camera, the avfoundation driver is required.
  The name of the device can be found with `ffmpeg -f avfoundation -list_devices true -i ""`

  Windows:
  In case of using a video camera, the dshow driver is required.

  Microphone input is not supported.

  The following options can be provided:\n#{NimbleOptions.docs(@reader_options_schema)}
  """
  @spec new(String.t(), Keyword.t()) :: {:ok, t()} | {:error, term()}
  def new(path, opts \\ []) do
    with {:ok, opts} <- NimbleOptions.validate(opts, @reader_options_schema) do
      do_create_reader(path, opts)
    end
  end

  @doc """
  Reads and decodes the next frame.
  """
  @spec next_frame(t()) :: {:ok, Xav.Frame.t()} | {:error, :eof}
  def next_frame(%__MODULE__{reader: ref} = reader) do
    case Xav.Reader.NIF.next_frame(ref) do
      {:ok, {data, format, width, height, pts}} ->
        format = normalize_format(format)
        {:ok, Xav.Frame.new(data, format, width, height, pts)}

      {:ok, {"", _format, _samples, _pts}} ->
        # Sometimes, audio converter might not return data immediately.
        # Hence, call until we succeed.
        next_frame(reader)

      {:ok, {data, format, samples, pts}} ->
        format = normalize_format(format)
        {:ok, Xav.Frame.new(data, format, samples, pts)}

      {:error, :eof} = err ->
        err
    end
  end

  @doc """
  Seeks the reader to the given time in seconds
  """
  @spec seek(t(), float()) :: :ok | {:error, term()}
  def seek(%__MODULE__{reader: ref}, time_in_seconds) do
    Xav.Reader.NIF.seek(ref, time_in_seconds)
  end

  @typedoc """
  A human-readable FFmpeg log level.

  The mapping to FFmpeg's `AV_LOG_*` constants is:

  | Atom        | FFmpeg constant   | Value |
  |-------------|-------------------|-------|
  | `:quiet`    | `AV_LOG_QUIET`    | -8    |
  | `:panic`    | `AV_LOG_PANIC`    | 0     |
  | `:fatal`    | `AV_LOG_FATAL`    | 8     |
  | `:error`    | `AV_LOG_ERROR`    | 16    |
  | `:warning`  | `AV_LOG_WARNING`  | 24    |
  | `:info`     | `AV_LOG_INFO`     | 32    |
  | `:verbose`  | `AV_LOG_VERBOSE`  | 40    |
  | `:debug`    | `AV_LOG_DEBUG`    | 48    |
  | `:trace`    | `AV_LOG_TRACE`    | 56    |

  FFmpeg's default is `:info`.
  """
  @type log_level ::
          :quiet
          | :panic
          | :fatal
          | :error
          | :warning
          | :info
          | :verbose
          | :debug
          | :trace

  @log_levels %{
    quiet: -8,
    panic: 0,
    fatal: 8,
    error: 16,
    warning: 24,
    info: 32,
    verbose: 40,
    debug: 48,
    trace: 56
  }

  @doc """
  Sets the FFmpeg log level.

  Accepts either one of the level atoms listed in `t:log_level/0`
  or a raw integer level. Returns `:ok`.

  This call wraps FFmpeg's `av_log_set_level/1`, which is
  **process-global**: the level is shared across every `libav*` and
  `libswscale` call in the current OS process, including readers,
  decoders, encoders, and converters created elsewhere in the Elixir
  VM. It is not scoped to a particular `Xav.Reader`.

  Typical use is to silence the informational `[swscaler @ ...]` lines
  that `libswscale` prints when it falls back to a non-SIMD
  colorspace conversion path (which happens for example on
  `yuv420p -> rgb24` on Apple Silicon):

      Xav.Reader.set_log_level(:error)

  To configure the level declaratively at application start, use the
  `:log_level` key in your application env instead:

      # config/runtime.exs
      config :xav, log_level: :error

  `Xav.Application` reads this on boot and calls `set_log_level/1`
  for you.

  ## Examples

      iex> Xav.Reader.set_log_level(:error)
      :ok

      iex> Xav.Reader.set_log_level(:quiet)
      :ok

  """
  @spec set_log_level(log_level() | integer()) :: :ok
  def set_log_level(level) when is_atom(level) do
    case Map.fetch(@log_levels, level) do
      {:ok, int} ->
        Xav.Reader.NIF.set_log_level(int)

      :error ->
        raise ArgumentError,
              "invalid FFmpeg log level #{inspect(level)}. " <>
                "Expected one of #{inspect(Map.keys(@log_levels))} or an integer."
    end
  end

  def set_log_level(level) when is_integer(level) do
    Xav.Reader.NIF.set_log_level(level)
  end

  @doc """
  Creates a new reader stream.

  Check `new/1` for the available options.
  """
  @spec stream!(String.t(), Keyword.t()) :: Enumerable.t()
  def stream!(path, opts \\ []) do
    Stream.resource(
      fn ->
        case new(path, opts) do
          {:ok, reader} ->
            reader

          {:error, reason} ->
            raise "Couldn't create a new Xav.Reader stream. Reason: #{inspect(reason)}"
        end
      end,
      fn reader ->
        case next_frame(reader) do
          {:ok, frame} -> {[frame], reader}
          {:error, :eof} -> {:halt, reader}
        end
      end,
      fn _reader -> :ok end
    )
  end

  defp do_create_reader(path, opts) do
    out_sample_rate = opts[:out_sample_rate] || 0
    out_channels = opts[:out_channels] || 0
    framerate = opts[:framerate]
    width = opts[:width]
    height = opts[:height]

    case Xav.Reader.NIF.new(
           path,
           to_int(opts[:device?]),
           to_int(opts[:read]),
           opts[:out_format],
           out_sample_rate,
           out_channels,
           framerate,
           width,
           height
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

      {:ok, reader, in_format, out_format, bit_rate, duration, codec, framerate} ->
        {:ok,
         %__MODULE__{
           reader: reader,
           in_format: in_format,
           out_format: out_format,
           bit_rate: bit_rate,
           duration: duration,
           codec: to_human_readable(codec),
           framerate: framerate
         }}

      {:error, _reason} = err ->
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

  # Use the same formats as Nx
  defp normalize_format(:flt), do: :f32
  defp normalize_format(:dbl), do: :f64
  defp normalize_format(other), do: other
end
