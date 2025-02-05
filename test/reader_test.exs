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

  describe "seek/2" do
    test "works with video" do
      {:ok, r} = Xav.Reader.new("./test/fixtures/sample_h264.mp4")
      assert :ok = Xav.Reader.seek(r, 5.0)
      assert({:ok, %Xav.Frame{}} = Xav.Reader.next_frame(r))
    end

    test "works with audio" do
      {:ok, r} = Xav.Reader.new("./test/fixtures/stt/harvard.mp3", read: :audio)
      assert :ok = Xav.Reader.seek(r, 5.0)
      assert({:ok, %Xav.Frame{}} = Xav.Reader.next_frame(r))
    end

    test "negative timestamp just seeks to beginning of file" do
      {:ok, r} = Xav.Reader.new("./test/fixtures/sample_h264.mp4")
      assert :ok = Xav.Reader.seek(r, -5.0)
      assert({:ok, %Xav.Frame{}} = Xav.Reader.next_frame(r))
    end

    test "timestamp greater than duration" do
      {:ok, r} = Xav.Reader.new("./test/fixtures/sample_h264.mp4")
      assert :ok = Xav.Reader.seek(r, (r.duration + 1) * 1.0)
      assert({:error, :eof} = Xav.Reader.next_frame(r))
    end

    test "seek back returns same frame" do
      {:ok, r} = Xav.Reader.new("./test/fixtures/sample_h264.mp4")
      assert :ok = Xav.Reader.seek(r, 0.0)
      assert({:ok, %Xav.Frame{} = first} = Xav.Reader.next_frame(r))
      assert :ok = Xav.Reader.seek(r, 5.0)
      assert({:ok, %Xav.Frame{}} = Xav.Reader.next_frame(r))
      assert :ok = Xav.Reader.seek(r, 0.0)
      assert({:ok, %Xav.Frame{} = other_first} = Xav.Reader.next_frame(r))
      assert first == other_first
    end
  end

  test "stream!" do
    Xav.Reader.stream!("./test/fixtures/sample_h264.mp4")
    |> Enum.all?(fn frame -> is_struct(frame, Xav.Frame) end)
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

  # This one might take some time on CI
  @tag timeout: 120_000
  test "speech to text" do
    for {path, expected_output} <- [
          {"./test/fixtures/stt/melnet_sample_0.mp3",
           """
            My thought, I have nobody by a beauty and will as you poured. \
           Mr. Rochester has served and that so don't find a simple and \
           devoted aboud to what might in a\
           """},
          {"./test/fixtures/stt/harvard.wav",
           """
            The stale smell of old beer lingers. It takes heat to bring out the odor. \
           A cold dip restores health in zest. A salt pickle tastes fine with ham. \
           Tacos all pastora are my favorite. A zestful food is the hot cross bun.\
           """},
          # This mp3 file results in an empty buffer after first conversion.
          {"./test/fixtures/stt/harvard.mp3",
           """
            The stale smell of old beer lingers. It takes heat to bring out the odor. \
           A cold dip restores health in zest. A salt pickle tastes fine with ham. \
           Tacos all pastora are my favorite. A zestful food is the hot cross bun.\
           """}
        ] do
      test_speech_to_text(path, expected_output)
    end
  end

  defp test_speech_to_text(path, expected_output) do
    {:ok, whisper} = Bumblebee.load_model({:hf, "openai/whisper-tiny"})
    {:ok, featurizer} = Bumblebee.load_featurizer({:hf, "openai/whisper-tiny"})
    {:ok, tokenizer} = Bumblebee.load_tokenizer({:hf, "openai/whisper-tiny"})
    {:ok, generation_config} = Bumblebee.load_generation_config({:hf, "openai/whisper-tiny"})

    serving =
      Bumblebee.Audio.speech_to_text_whisper(whisper, featurizer, tokenizer, generation_config,
        defn_options: [compiler: EXLA]
      )

    batch =
      Xav.Reader.stream!(path,
        read: :audio,
        out_channels: 1,
        out_format: :flt,
        out_sample_rate: 16_000
      )
      |> Enum.map(&Xav.Frame.to_nx(&1))
      |> Nx.Batch.concatenate()

    batch = Nx.Defn.jit_apply(&Function.identity/1, [batch])
    assert %{chunks: chunks} = Nx.Serving.run(serving, batch)

    assert [%{text: ^expected_output}] = chunks
  end
end
