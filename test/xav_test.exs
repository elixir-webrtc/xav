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
end
