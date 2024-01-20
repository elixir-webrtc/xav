defmodule Xav.ReaderTest do
  use ExUnit.Case, async: true

  test "new/1" do
    assert {:ok, %Xav.Reader{}} = Xav.Reader.new("./test/fixtures/sample_h264.mp4")
    assert {:error, _reason} = Xav.Reader.new("non_existing_input")
  end

  test "new!/1" do
    %Xav.Reader{} = Xav.Reader.new!("./test/fixtures/sample_h264.mp4")
    assert_raise RuntimeError, fn -> Xav.Reader.new!("non_existing_input") end
  end

  test "next_frame/1" do
    {:ok, r} = Xav.Reader.new("./test/fixtures/sample_h264.mp4")
    # test reading 5 seconds
    for _i <- 0..(30 * 5), do: assert({:ok, %Xav.Frame{}} = Xav.Reader.next_frame(r))
  end

  # @tag :debug
  # test "next_frame/1 audio" do
  #   {:ok, r} = Xav.Reader.new("./test/fixtures/sample.mp3", read: :audio)

  #   for _i <- 0..5 do
  #     assert({:ok, %Xav.Frame{} = frame} = Xav.Reader.next_frame(r))
  #     IO.inspect({byte_size(frame.data), frame})
  #   end
  # end

  test "to_nx/1" do
    {:ok, r} = Xav.Reader.new("./test/fixtures/sample_h264.mp4")
    {:ok, frame} = Xav.Reader.next_frame(r)
    %Nx.Tensor{} = Xav.Frame.to_nx(frame)
  end

  test "eof" do
    {:ok, r} = Xav.Reader.new("./test/fixtures/one_frame.mp4")
    {:ok, _frame} = Xav.Reader.next_frame(r)
    {:error, :eof} = Xav.Reader.next_frame(r)
  end

  @formats [{"h264", "h264"}, {"h264", "mkv"}, {"vp8", "webm"}, {"vp9", "webm"}, {"av1", "mkv"}]
  Enum.map(@formats, fn {codec, container} ->
    name = "#{codec} #{container}"
    file = "./test/fixtures/sample_#{codec}.#{container}"

    test name do
      {:ok, r} = Xav.Reader.new(unquote(file))
      # test reading 5 seconds
      for _i <- 0..(30 * 5), do: assert({:ok, %Xav.Frame{}} = Xav.Reader.next_frame(r))
    end
  end)
end
