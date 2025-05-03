defmodule Xav.Decoder do
  @moduledoc """
  Audio/video decoder.
  """

  @typedoc """
  Supported codecs.

  To get the list of available decoders see `Xav.list_decoders/0`.
  """
  @type codec() :: atom()

  @type t() :: reference()

  @decoder_options_schema [
    channels: [
      type: :pos_integer,
      doc: """
      The number of channels of the encoded audio.

      Some decoders require this field to be set by the user. (e.g. `G711`)
      """
    ],
    out_format: [
      type: :atom,
      doc: """
      Output format of the samples.

      In case of video, it's the pixel format. In case of audio, it's the sample format.

      To get the list of supported pixel formats use `Xav.pixel_formats/0`,
      and for sample formats `Xav.sample_formats/0`.
      """
    ],
    out_sample_rate: [
      type: :pos_integer,
      doc: """
      Audio sample rate.

      If not specified, the sample rate of the input stream will be used.
      """
    ],
    out_channels: [
      type: :pos_integer,
      doc: """
      Number of audio channels.

      If not specified, the number of channels of the input stream will be used.

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
    ],
    out_width: [
      type: :pos_integer,
      doc: "Scale the output video frame to the provided width."
    ],
    out_height: [
      type: :pos_integer,
      doc: "Scale the output video frame to the provided height."
    ]
  ]

  @doc """
  Creates a new decoder.

  `codec` is any audio/video decoder supported by `FFmpeg`.

  `opts` can be used to specify desired output parameters:\n#{NimbleOptions.docs(@decoder_options_schema)}
  """
  @spec new(codec(), Keyword.t()) :: t()
  def new(codec, opts \\ []) when is_atom(codec) do
    opts = NimbleOptions.validate!(opts, @decoder_options_schema)

    Xav.Decoder.NIF.new(
      codec,
      opts[:channels] || -1,
      opts[:out_format],
      opts[:out_sample_rate] || 0,
      opts[:out_channels] || 0,
      opts[:out_width] || -1,
      opts[:out_height] || -1
    )
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
  Flushes the decoder.

  Flushing signals end of stream and forces the decoder to return
  the buffered frames if there're any.
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
