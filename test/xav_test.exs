defmodule XavTest do
  use ExUnit.Case
  doctest Xav

  test "new_reader/1" do
    Xav.new_reader("./sample_h264.h264")
  end

  test "next_frame/1" do
    r =
      "./sample_h264.h264"
      |> Xav.new_reader()

    Xav.next_frame(r)
    Xav.next_frame(r)
    Xav.next_frame(r)
    Xav.next_frame(r)
    Xav.next_frame(r)
    Xav.next_frame(r)
    Xav.next_frame(r)
    Xav.next_frame(r)

    Xav.next_frame(r)
    |> IO.inspect()

    # |> byte_size()
  end

  test "Frame.to_nx/1" do
    r =
      "./sample_h264.h264"
      |> Xav.new_reader()

    Xav.next_frame(r)
    |> Xav.Frame.to_nx()
    |> IO.inspect()
  end

  @tag :debug
  test "mp4" do
    r =
      "./sample_mp4.mp4"
      |> Xav.new_reader()

    Xav.next_frame(r)
    |> Xav.Frame.to_nx()
    |> IO.inspect()
  end
end
