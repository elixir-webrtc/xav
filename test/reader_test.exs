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
    # the file has 30fps, try to read 5 seconds
    for _i <- 0..(30 * 5), do: assert({:ok, %Xav.Frame{}} = Xav.Reader.next_frame(r))
  end

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
      # try to read 100 frames
      for _i <- 0..100, do: assert({:ok, %Xav.Frame{}} = Xav.Reader.next_frame(r))
    end
  end)

  @tag :debug
  test "speech to text" do
    # This file has been downloaded from https://audio-samples.github.io/
    # Section: Samples from the model without biasing or priming.
    reader = Xav.Reader.new!("./test/fixtures/melnet_sample_0.mp3", read: :audio)

    {:ok, whisper} = Bumblebee.load_model({:hf, "openai/whisper-tiny"})
    {:ok, featurizer} = Bumblebee.load_featurizer({:hf, "openai/whisper-tiny"})
    {:ok, tokenizer} = Bumblebee.load_tokenizer({:hf, "openai/whisper-tiny"})
    {:ok, generation_config} = Bumblebee.load_generation_config({:hf, "openai/whisper-tiny"})

    serving =
      Bumblebee.Audio.speech_to_text_whisper(whisper, featurizer, tokenizer, generation_config,
        defn_options: [compiler: EXLA]
      )

    batch =
      read_frames(reader)
      |> Enum.map(&Xav.Frame.to_nx(&1))
      |> Nx.Batch.concatenate()

    batch = Nx.Defn.jit_apply(&Function.identity/1, [batch])
    assert %{chunks: chunks} = Nx.Serving.run(serving, batch)

    assert [
             %{
               text: """
                My thought I have nobody by a beauty and will as you poured. \
               Mr. Rochester has served in that so-done fine-simpless and \
               devoted to bowed, to let might in a\
               """
             }
           ] = chunks
  end

  defp read_frames(reader, acc \\ []) do
    case Xav.Reader.next_frame(reader) do
      {:ok, frame} ->
        read_frames(reader, [frame | acc])

      {:error, :eof} ->
        Enum.reverse(acc)
    end
  end
end
