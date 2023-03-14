defmodule XavTest do
  use ExUnit.Case
  doctest Xav

  test "next_frame/1" do
    r =
      "./out.h264"
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
    # |> byte_size()
    # |> IO.inspect()
  end

  test "Frame.to_nx/1" do
    r =
      "./out.h264"
      |> Xav.new_reader()

    Xav.next_frame(r)
    |> Xav.Frame.to_nx()
    |> IO.inspect()
  end
end
