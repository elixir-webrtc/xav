defmodule Xav.Decoder do
  @moduledoc """
  Audio/video decoder.
  """

  @typedoc """
  Supported codecs.
  """
  @type codec() :: :opus | :vp8 | :h264 | :h265

  @type t() :: reference()

  @typedoc """
  Opts that can be passed to `new/2`.
  """
  @type opts :: [
          out_format: Xav.Frame.format(),
          out_sample_rate: integer(),
          out_channels: integer()
        ]

  @doc """
  Creates a new decoder.

  `opts` can be used to specify desired output parameters.

  E.g. if you want to change audio samples format just pass:

  ```elixir
  [out_format: :f32]
  ```

  Video frames are always returned in RGB format.
  This setting cannot be changed.

  Audio samples are always in the packed form -
  samples from different channels are interleaved in the same, single binary:

  ```
  <<c10, c20, c30, c11, c21, c31, c12, c22, c32>>
  ```

  An alternative would be to return a list of binaries, where
  each binary represents different channel:

  ```
  [
    <<c10, c11, c12, c13, c14>>,
    <<c20, c21, c22, c23, c24>>,
    <<c30, c31, c32, c33, c34>>
  ]
  ```
  """
  @spec new(codec(), opts()) :: t()
  def new(codec, opts \\ []) do
    out_format = opts[:out_format]
    out_sample_rate = opts[:out_sample_rate] || 0
    out_channels = opts[:out_channels] || 0
    Xav.Decoder.NIF.new(codec, out_format, out_sample_rate, out_channels)
  end

  @doc """
  Decodes an audio/video frame.

  Some video frames are meant for decoder only and will not
  contain actual video samples.
  Some audio frames might require more data to be converted
  to the desired output format.
  In both cases, `:ok` term is returned and more data needs to be provided.
  """
  @spec decode(t(), binary(), pts: integer(), dts: integer()) ::
          :ok | {:ok, Xav.Frame.t()} | {:error, atom()}
  def decode(decoder, data, opts \\ []) do
    pts = opts[:pts] || 0
    dts = opts[:dts] || 0

    case Xav.Decoder.NIF.decode(decoder, data, pts, dts) do
      :ok ->
        :ok

      {:ok, {data, format, width, height, pts}} ->
        {:ok, Xav.Frame.new(data, format, width, height, pts)}

      # Sometimes, audio converter might not return data immediately.
      {:ok, {"", _format, _samples, _pts}} ->
        :ok

      {:ok, {data, format, samples, pts}} ->
        {:ok, Xav.Frame.new(data, format, samples, pts)}

      {:error, _reason} = error ->
        error
    end
  end

  @doc """
  Flush the decoder.

  Flushing signales end of stream and force the decoder to return
  the buffered frames if there's any.
  """
  @spec flush(t()) :: {:ok, [Xav.Frame.t()]} | {:error, atom()}
  def flush(decoder) do
    with {:ok, frames} <- Xav.Decoder.NIF.flush(decoder) do
      frames =
        Enum.map(frames, fn {data, format, width, height, pts} ->
          Xav.Frame.new(data, format, width, height, pts)
        end)

      {:ok, frames}
    end
  end

  @doc """
  Same as `flush/1` but raises an exception on error.
  """
  def flush!(decoder) do
    case flush(decoder) do
      {:ok, frames} -> frames
      {:error, reason} -> raise "Failed to flush decoder: #{inspect(reason)}"
    end
  end
end
