defmodule Xav.VideoConverterTest do
  use ExUnit.Case, async: true

  test "new/4" do
    assert {:ok, converter} = Xav.VideoConverter.new(1920, 1080, :yuv420p, :rgb24)
    assert {:error, :unknown_format} = Xav.VideoConverter.new(1920, 1080, :yuv420p, :rgb)

    assert is_reference(converter)
  end

  test "new!/4" do
    assert converter = Xav.VideoConverter.new!(1920, 1080, :yuv420p, :rgb24)
    assert_raise RuntimeError, fn -> Xav.VideoConverter.new!(1920, 1080, :yuv420p, :rgb) end

    assert is_reference(converter)
  end

  describe "convert/2" do
    setup do
      %{converter: Xav.VideoConverter.new!(480, 360, :yuv420p, :rgb24)}
    end

    test "convert video format", %{converter: converter} do
      assert frame =
               Xav.VideoConverter.convert(converter, %Xav.Frame{
                 type: :video,
                 data: File.read!("test/fixtures/video_converter/frame_480x360.yuv"),
                 format: :yuv420p,
                 width: 480,
                 height: 360,
                 pts: 0
               })

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
             } = frame
    end

    test "converter re-init on resolution change", %{converter: converter} do
      frame1 = %Xav.Frame{
        type: :video,
        data: File.read!("test/fixtures/video_converter/frame_480x360.yuv"),
        format: :yuv420p,
        width: 480,
        height: 360
      }

      frame2 = %Xav.Frame{
        type: :video,
        data: File.read!("test/fixtures/video_converter/frame_360x240.yuv"),
        format: :yuv420p,
        width: 360,
        height: 240
      }

      assert %Xav.Frame{format: :rgb24, data: ref_frame1} =
               Xav.VideoConverter.convert(converter, frame1)

      assert %Xav.Frame{format: :rgb24, data: ref_frame2} =
               Xav.VideoConverter.convert(converter, frame2)

      assert byte_size(ref_frame1) == 480 * 360 * 3
      assert byte_size(ref_frame2) == 360 * 240 * 3
    end
  end
end
