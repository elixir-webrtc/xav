defmodule Xav.Decoder do
  def new() do
    Xav.NIF.new_decoder()
  end

  def decode(decoder, data, pts, dts) do
    {:ok, {data, format, width, height, pts}} = Xav.NIF.decode(decoder, data, pts, dts)
    {:ok, Xav.Frame.new(data, format, width, height, pts)}
  end
end
