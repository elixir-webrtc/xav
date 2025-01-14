defmodule Xav.VideoConverterTest do
  use ExUnit.Case, async: true

  test "new/1" do
    assert {:ok, %Xav.VideoConverter{format: :rgb24, converter: converter}} =
             Xav.VideoConverter.new(format: :rgb24)

    assert is_reference(converter)

    assert_raise RuntimeError, fn -> Xav.VideoConverter.new(format: nil) end
  end

  test "new!/1" do
    assert %Xav.VideoConverter{} = Xav.VideoConverter.new!(format: :rgb24)
    assert_raise ErlangError, fn -> Xav.VideoConverter.new!(format: :rgb) end
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
        converter: Xav.VideoConverter.new!(format: :rgb24),
        frame_480p: frame_480p
      }
    end

    test "convert video format", %{converter: converter, frame_480p: frame_480p} do
      # reference frame is generated using ffmeg
      # ffmpeg -f rawvideo -pix_fmt yuv420p -video_size 480x360 -i frame_480x360.yuv -pix_fmt rgb24 ref_frame_480x360.yuv
      ref_data = File.read!("test/fixtures/video_converter/ref_frame_480x360.rgb")

      assert %Xav.Frame{
               type: :video,
               data: ^ref_data,
               format: :rgb24,
               width: 480,
               height: 360,
               pts: 0
             } = Xav.VideoConverter.convert(converter, frame_480p)
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
      converter = Xav.VideoConverter.new!(width: 368)

      assert %Xav.Frame{
               type: :video,
               format: :yuv420p,
               width: 368,
               height: 276
             } = Xav.VideoConverter.convert(converter, frame_480p)
    end

    test "scale and convert video frame", %{frame_480p: frame_480p} do
      converter = Xav.VideoConverter.new!(width: 360, height: 240, format: :rgb24)

      assert %Xav.Frame{
               type: :video,
               format: :rgb24,
               width: 360,
               height: 240
             } = Xav.VideoConverter.convert(converter, frame_480p)
    end
  end
end
