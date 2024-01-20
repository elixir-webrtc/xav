defmodule Xav.Decoder do
  @moduledoc """
  Audio/video decoder.
  """

  @type codec() :: :opus | :vp8

  @type t() :: reference()

  @spec new(codec()) :: t()
  def new(codec) do
    Xav.NIF.new_decoder(codec)
  end

  @spec decode(t(), binary(), integer(), integer()) :: {:ok, Xav.Frame.t()} | {:error, atom()}
  def decode(decoder, data, pts, dts) do
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
