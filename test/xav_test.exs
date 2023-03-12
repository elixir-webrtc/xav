defmodule XavTest do
  use ExUnit.Case
  doctest Xav

  test "next_frame/1" do
    "./out.h264"
    |> Xav.new_reader()
    |> Xav.next_frame()
    |> IO.inspect(limit: :infinity)
    |> byte_size()
    |> IO.inspect()
  end
end
