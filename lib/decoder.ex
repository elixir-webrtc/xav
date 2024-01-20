defmodule Xav.Decoder do
  @moduledoc """
  Audio/video decoder.
  """

  @typedoc """
  Supported codecs.
  """
  @type codec() :: :opus | :vp8

  @type t() :: reference()

  @doc """
  Creates a new decoder.
  """
  @spec new(codec()) :: t()
  def new(codec) do
    Xav.NIF.new_decoder(codec)
  end

  @doc """
  Decodes an audio or video frame.

  Video frames are always in the RGB format.
  Audio samples are always interleaved.
  """
  @spec decode(t(), binary(), pts: integer(), dts: integer()) ::
          {:ok, Xav.Frame.t()} | {:error, atom()}
  def decode(decoder, data, opts \\ []) do
    pts = opts[:pts] || 0
    dts = opts[:dts] || 0

    case Xav.NIF.decode(decoder, data, pts, dts) do
      {:ok, {data, format, width, height, pts}} ->
        {:ok, Xav.Frame.new(data, format, width, height, pts)}

      {:ok, {data, format, samples, pts}} ->
        {:ok, Xav.Frame.new(data, format, samples, pts)}

      {:error, _reason} = error ->
        error
    end
  end
end
