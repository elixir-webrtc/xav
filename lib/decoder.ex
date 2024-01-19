defmodule Xav.Decoder do
  def new(codec) do
    Xav.NIF.new_decoder(codec)
  end

  def decode(decoder, data, pts, dts) do
    case Xav.NIF.decode(decoder, data, pts, dts) do
      {:ok, {data, format, width, height, pts}} ->
        {:ok, Xav.Frame.new(data, format, width, height, pts)}

      {:ok, {data, format, samples, pts}} ->
        {:ok, Xav.Frame.new(data, format, samples, pts)}
    end
  end
end
