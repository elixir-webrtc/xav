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
    ],
    gop_size: [
      type: :pos_integer,
      doc: """
      Group of pictures length.

      Determines the interval in which I-Frames (or keyframes) are inserted in
      the stream. e.g. a value of 50, means the I-Frame will be inserted at the 1st frame,
      the 51st frame, the 101st frame, and so on.
      """
    ],
    max_b_frames: [
      type: :non_neg_integer,
      doc: """
      Maximum number of consecutive B-Frames to insert between non-B-Frames.

      A value of 0, disable insertion of B-Frames.
      """
    ],
    profile: [
      type: {:in, [:constrained_baseline, :baseline, :main, :high, :main_10, :main_still_picture]},
      type_doc: "`t:atom/0`",
      doc: """
      The encoder's profile.

      A profile defines the capabilities and features an encoder can use to
      target specific applications (e.g. `live video`)

      The following profiles are defined:

      | Codec | Profiles |
      |-------|----------|
      | h264  | constrained_baseline, baseline, main, high |
      | h265/hevc  | main, main_10, main_still_picture |
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
