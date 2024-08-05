defmodule Xav.Reader.NIF do
  @moduledoc false

  @on_load :__on_load__

  def __on_load__ do
    path = :filename.join(:code.priv_dir(:xav), ~c"libxavreader")
    :ok = :erlang.load_nif(path, 0)
  end

  def new(_path, _device, _video), do: :erlang.nif_error(:undef)

  def next_frame(_reader), do: :erlang.nif_error(:undef)
end
