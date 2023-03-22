defmodule XavTest do
  use ExUnit.Case
  doctest Xav

  test "new_reader/1" do
    assert {:ok, %Xav.Reader{}} = Xav.new_reader("./test/fixtures/sample_mp4.mp4")
    assert {:error, _reason} = Xav.new_reader("non_existing_input")
  end

  test "new_reader!/1" do
    {:ok, %Xav.Reader{}} = Xav.new_reader("./test/fixtures/sample_mp4.mp4")
    assert_raise RuntimeError, fn -> Xav.new_reader!("non_existing_input") end
  end

  test "next_frame/1" do
    {:ok, r} = Xav.new_reader("./test/fixtures/sample_mp4.mp4")
    # test reading 5 seconds
    for _i <- 0..(30 * 5), do: assert({:ok, %Xav.Frame{}} = Xav.next_frame(r))
  end

  test "to_nx/1" do
    {:ok, r} = Xav.new_reader("./test/fixtures/sample_mp4.mp4")
    {:ok, frame} = Xav.next_frame(r)
    %Nx.Tensor{} = Xav.Frame.to_nx(frame)
  end

  test "eof" do
    {:ok, r} = Xav.new_reader("./test/fixtures/one_frame.mp4")
    {:ok, _frame} = Xav.next_frame(r)
    {:error, :eof} = Xav.next_frame(r)
  end

  test "h264" do
    {:ok, r} = Xav.new_reader("./test/fixtures/sample_h264.h264")
    # test reading 5 seconds
    for _i <- 0..(30 * 5), do: assert({:ok, %Xav.Frame{}} = Xav.next_frame(r))
  end
end
