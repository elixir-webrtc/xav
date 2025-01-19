defmodule Xav.Decoder.NIF do
  @moduledoc false

  @on_load :__on_load__

  def __on_load__ do
    path = :filename.join(:code.priv_dir(:xav), ~c"libxavdecoder")
    :ok = :erlang.load_nif(path, 0)
  end

  def new(_codec, _out_format, _out_sample_rate, _out_channels, _out_width, _out_height) do
    :erlang.nif_error(:undef)
  end

  def decode(_decoder, _data, _pts, _dts), do: :erlang.nif_error(:undef)

  def flush(_decoder), do: :erlang.nif_error(:undef)
end
