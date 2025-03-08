defmodule Xav.Encoder do
  @moduledoc """
  Audio/Video encoder.

  Currently, it only supports video encoding.
  """

  @type t :: reference()

  @type codec :: atom()
  @type encoder_options :: Keyword.t()

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
      type: :string,
      doc: """
      The encoder's profile.

      A profile defines the capabilities and features an encoder can use to
      target specific applications (e.g. `live video`)

      To get the list of available profiles for an encoder, see `Xav.list_encoders/0`
      """
    ]
  ]

  @audio_encoder_schema [
    format: [
      type: :atom,
      required: true,
      doc: "Sample format of the audio samples."
    ],
    sample_rate: [
      type: :pos_integer,
      default: 44100,
      doc: """
      Number of samples per second.

      To get the list of supported sample rates for an encoder, see `Xav.list_encoders/0`
      """
    ],
    profile: [
      type: :string,
      doc: """
      The encoder's profile.

      To get the list of available profiles for an encoder, see `Xav.list_encoders/0`
      """
    ],
    channel_layout: [
      type: :string,
      required: true,
      doc: """
      Channel layout of the audio samples.

      For possible values, check [this](https://ffmpeg.org/ffmpeg-utils.html#Channel-Layout).
      """
    ]
  ]

  @doc """
  Create a new encoder.

  To get the list of available encoders, see `Xav.list_encoders/0`.

  It accepts the following options:\n#{NimbleOptions.docs(@video_encoder_schema)}
  """
  @spec new(codec(), Keyword.t()) :: t()
  def new(codec, opts) do
    {codec, codec_id, media_type} = validate_codec!(codec)

    nif_options =
      case media_type do
        :video ->
          opts = NimbleOptions.validate!(opts, @video_encoder_schema)
          {time_base_num, time_base_den} = opts[:time_base]

          opts
          |> Map.new()
          |> Map.delete(:time_base)
          |> Map.merge(%{time_base_num: time_base_num, time_base_den: time_base_den})

        :audio ->
          opts
          |> NimbleOptions.validate!(@audio_encoder_schema)
          |> Map.new()
      end

    if codec_id do
      Xav.Encoder.NIF.new(nil, Map.put(nif_options, :codec_id, codec_id))
    else
      Xav.Encoder.NIF.new(codec, nif_options)
    end
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

  defp validate_codec!(codec) do
    Xav.Encoder.NIF.list_encoders()
    |> Enum.find_value(fn {codec_family, encoder_name, _, media_type, codec_id, _profiles,
                           _sample_formats, _sample_rates} ->
      cond do
        media_type not in [:video, :audio] -> nil
        encoder_name == codec -> {encoder_name, nil, media_type}
        codec_family == codec -> {codec, codec_id, media_type}
        true -> nil
      end
    end)
    |> case do
      nil -> raise ArgumentError, "Unknown codec: #{inspect(codec)}"
      result -> result
    end
  end
end
