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
  decoders, encoders, and converters created from the Elixir VM.

  Typical use is to silence the informational `[swscaler @ ...]` lines
  that `libswscale` prints when it falls back to a non-SIMD
  colorspace conversion path (which happens for example on
  `yuv420p -> rgb24` on Apple Silicon):

      Xav.set_log_level(:error)

  To configure the level declaratively at application start, use the
  `:ffmpeg_log_level` key in your application env instead:

      # config/runtime.exs
      config :xav, ffmpeg_log_level: :error

  `Xav.Application` reads this on boot and calls `set_log_level/1`
  for you.

  ## Examples

      iex> Xav.set_log_level(:error)
      :ok

      iex> Xav.set_log_level(:quiet)
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
