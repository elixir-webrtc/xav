defmodule Xav.EncoderTest do
  use ExUnit.Case, async: true

  alias NimbleOptions.ValidationError

  describe "new/2" do
    test "new encoder" do
      assert encoder =
               Xav.Encoder.new(:h264,
                 width: 180,
                 height: 160,
                 format: :yuv420p,
                 time_base: {1, 90_000}
               )

      assert is_reference(encoder)
    end

    test "raises on invalid encoder" do
      assert_raise ArgumentError, fn -> Xav.Encoder.new(:h264_none, []) end
    end

    test "raises on invalid options" do
      assert_raise ValidationError, fn -> Xav.Encoder.new(:h264, width: 180) end

      assert_raise ValidationError, fn ->
        Xav.Encoder.new(:hevc, width: 360, height: -4, format: :yuv420p, time_base: {1, 90_000})
      end
    end
  end

  describe "encode/1" do
    setup do
      frame = %Xav.Frame{
        type: :video,
        data: File.read!("test/fixtures/video_converter/frame_360x240.yuv"),
        format: :yuv420p,
        width: 360,
        height: 240,
        pts: 0
      }

      %{frame: frame}
    end

    test "encode a frame", %{frame: frame} do
      encoder =
        Xav.Encoder.new(:h264,
          width: 360,
          height: 240,
          format: :yuv420p,
          time_base: {1, 25}
        )

      assert [] = Xav.Encoder.encode(encoder, frame)

      assert [
               %Xav.Packet{
                 data: data,
                 dts: 0,
                 pts: 0,
                 keyframe?: true
               }
             ] = Xav.Encoder.flush(encoder)

      assert byte_size(data) > 0
    end

    test "encode multiple frames", %{frame: frame} do
      encoder =
        Xav.Encoder.new(:h264,
          width: 360,
          height: 240,
          format: :yuv420p,
          time_base: {1, 25},
          gop_size: 1
        )

      packets =
        Xav.Encoder.encode(encoder, frame) ++
          Xav.Encoder.encode(encoder, %{frame | pts: 1}) ++
          Xav.Encoder.encode(encoder, %{frame | pts: 2}) ++ Xav.Encoder.flush(encoder)

      assert length(packets) == 3
      assert Enum.all?(packets, & &1.keyframe?)
    end

    test "no bframes inserted", %{frame: frame} do
      encoder =
        Xav.Encoder.new(:hevc,
          width: 360,
          height: 240,
          format: :yuv420p,
          time_base: {1, 25},
          max_b_frames: 0
        )

      packets =
        Stream.iterate(frame, fn frame -> %{frame | pts: frame.pts + 1} end)
        |> Stream.take(20)
        |> Stream.transform(
          fn -> encoder end,
          fn frame, encoder ->
            {Xav.Encoder.encode(encoder, frame), encoder}
          end,
          fn encoder -> {Xav.Encoder.flush(encoder), encoder} end,
          fn _encoder -> :ok end
        )
        |> Enum.to_list()

      assert length(packets) == 20
      assert Enum.all?(packets, &(&1.dts == &1.pts)), "dts should be equal to pts"
    end
  end
end
