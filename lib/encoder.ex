defmodule Xav.Encoder do
  @moduledoc """
  Audio/Video encoder.

  Currently, it only supports video encoding:
    * `h264`
    * `h265`/`hevc`
  """

  @type t :: reference()

  @type codec :: :h264 | :h265 | :hevc
  @type encoder_options :: Keyword.t()

  @video_codecs [:h264, :h265, :hevc]

  @video_encoder_schema [
    width: [
      type: :pos_integer,
      required: true,
      doc: "Width of the video samples."
    ],
    height: [
      type: :pos_integer,
      required: true,
      doc: "Height of the video samples."
    ],
    format: [
      type: :atom,
      required: true,
      doc: "Pixel format of the video samples."
    ],
    time_base: [
      type: {:tuple, [:pos_integer, :pos_integer]},
      required: true,
      doc: """
      Time base of the video stream.

      It is a rational represented as a tuple of two postive integers `{numerator, denominator}`.
      It represent the number of ticks `denominator` in `numerator` seconds. e.g. `{1, 90000}` reprensents
      90000 ticks in 1 second.

      it is used for the decoding and presentation timestamps of the video frames. For video frames with constant
      frame rate, choose a timebase of `{1, frame_rate}`.
      """
    ]
  ]

  @doc """
  Create a new encoder.

  It accepts the following options:\n#{NimbleOptions.docs(@video_encoder_schema)}
  """
  @spec new(codec(), Keyword.t()) :: t()
  def new(codec, opts) when codec in @video_codecs do
    opts = NimbleOptions.validate!(opts, @video_encoder_schema)
    {time_base_num, time_base_den} = opts[:time_base]

    nif_options =
      opts
      |> Map.new()
      |> Map.delete(:time_base)
      |> Map.merge(%{time_base_num: time_base_num, time_base_den: time_base_den})

    Xav.Encoder.NIF.new(codec, nif_options)
  end

  @doc """
  Encodes a frame.

  The return value may be an empty list in case the encoder
  needs more frames to produce a packet.
  """
  @spec encode(t(), Xav.Frame.t()) :: [Xav.Packet.t()]
  def encode(encoder, frame) do
    encoder
    |> Xav.Encoder.NIF.encode(frame.data, frame.pts)
    |> to_packets()
  end

  @doc """
  Flush the encoder.
  """
  @spec flush(t()) :: [Xav.Packet.t()]
  def flush(encoder) do
    encoder
    |> Xav.Encoder.NIF.flush()
    |> to_packets()
  end

  defp to_packets(result) do
    Enum.map(result, fn {data, dts, pts, keyframe?} ->
      %Xav.Packet{data: data, dts: dts, pts: pts, keyframe?: keyframe?}
    end)
  end
end
