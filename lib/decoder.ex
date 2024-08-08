defmodule Xav.Decoder do
  @moduledoc """
  Audio/video decoder.
  """

  @typedoc """
  Supported codecs.
  """
  @type codec() :: :opus | :vp8

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

  The way in which samples are interleaved is not specified.

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

  Some frames are meant for decoder only and might not
  contain actual video samples.
  In such cases, `:ok` term is returned.
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

      {:ok, {data, format, samples, pts}} ->
        {:ok, Xav.Frame.new(data, format, samples, pts)}

      {:error, _reason} = error ->
        error
    end
  end
end
