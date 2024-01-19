defmodule Xav.Decoder do
  def new(codec) do
    Xav.NIF.new_decoder(codec)
  end

  def decode(decoder, data, pts, dts) do
    {:ok, {data, format, width, height, pts}} = Xav.NIF.decode(decoder, data, pts, dts)
    {:ok, Xav.Frame.new(data, format, width, height, pts)}
  end
end
