defmodule Xav.VideoConverterTest do
  use ExUnit.Case, async: true

  describe "new/1" do
    test "new converter" do
      assert %Xav.VideoConverter{out_format: :rgb24, converter: converter} =
               Xav.VideoConverter.new(out_format: :rgb24)

      assert is_reference(converter)
    end

    test "fails when no option is provided" do
      assert_raise RuntimeError, fn -> Xav.VideoConverter.new(out_format: nil) end
    end

    test "fails on invalid options" do
      assert_raise ArgumentError, fn -> Xav.VideoConverter.new(out_width: 0) end
      assert_raise ArgumentError, fn -> Xav.VideoConverter.new(out_height: "15") end
    end
  end

  describe "convert/2" do
    setup do
      frame_480p = %Xav.Frame{
        type: :video,
        data: File.read!("test/fixtures/video_converter/frame_480x360.yuv"),
        format: :yuv420p,
        width: 480,
        height: 360,
        pts: 0
      }

      %{
        converter: Xav.VideoConverter.new(out_format: :rgb24),
        frame_480p: frame_480p
      }
    end

    test "convert video format", %{converter: converter, frame_480p: frame_480p} do
      assert %Xav.Frame{
               type: :video,
               data: data,
               format: :rgb24,
               width: 480,
               height: 360,
               pts: 0
             } = Xav.VideoConverter.convert(converter, frame_480p)

      assert byte_size(data) == 480 * 360 * 3
    end

    test "converter re-init on resolution change", %{converter: converter, frame_480p: frame_480p} do
      frame_360p = %Xav.Frame{
        type: :video,
        data: File.read!("test/fixtures/video_converter/frame_360x240.yuv"),
        format: :yuv420p,
        width: 360,
        height: 240
      }

      assert %Xav.Frame{format: :rgb24, data: ref_frame1} =
               Xav.VideoConverter.convert(converter, frame_480p)

      assert %Xav.Frame{format: :rgb24, data: ref_frame2} =
               Xav.VideoConverter.convert(converter, frame_360p)

      assert byte_size(ref_frame1) == 480 * 360 * 3
      assert byte_size(ref_frame2) == 360 * 240 * 3
    end

    test "scale video frame", %{frame_480p: frame_480p} do
      converter = Xav.VideoConverter.new(out_width: 368)

      assert %Xav.Frame{
               type: :video,
               format: :yuv420p,
               data: data,
               width: 368,
               height: 276
             } = Xav.VideoConverter.convert(converter, frame_480p)

      assert byte_size(data) == 368 * 276 * 3 / 2
    end

    test "scale and convert video frame", %{frame_480p: frame_480p} do
      converter = Xav.VideoConverter.new(out_width: 360, out_height: 240, out_format: :rgb24)

      assert %Xav.Frame{
               type: :video,
               format: :rgb24,
               width: 360,
               height: 240
             } = Xav.VideoConverter.convert(converter, frame_480p)
    end
  end
end
