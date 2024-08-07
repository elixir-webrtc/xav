defmodule Xav.Decoder.NIF do
  @moduledoc false

  @on_load :__on_load__

  def __on_load__ do
    path = :filename.join(:code.priv_dir(:xav), ~c"libxavdecoder")
    :ok = :erlang.load_nif(path, 0)
  end

  def new(_codec, _out_format, _out_sample_rate, _out_channels), do: :erlang.nif_error(:undef)

  def decode(_decoder, _data, _pts, _dts), do: :erlang.nif_error(:undef)
end
