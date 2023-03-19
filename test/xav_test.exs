defmodule XavTest do
  use ExUnit.Case
  doctest Xav

  test "new_reader/1" do
    Xav.new_reader("./test/fixtures/sample_h264.h264")
  end

  test "next_frame/1" do
    r =
      "./test/fixtures/sample_h264.h264"
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
      "./test/fixtures/sample_h264.h264"
      |> Xav.new_reader()

    {:ok, frame} = Xav.next_frame(r)

    Xav.Frame.to_nx(frame)
    |> IO.inspect()
  end

  test "mp4" do
    r =
      "./test/fixtures/sample_mp4.mp4"
      |> Xav.new_reader()

    {:ok, frame} = Xav.next_frame(r)

    Xav.Frame.to_nx(frame)
    |> IO.inspect()
  end

  test "eof" do
    r = Xav.new_reader("./test/fixtures/one_frame.mp4")
    {:ok, _frame} = Xav.next_frame(r)
    {:error, :eof} = Xav.next_frame(r)
  end
end
