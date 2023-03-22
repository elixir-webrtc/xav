defmodule Xav.NIF do
  @moduledoc false

  @on_load :__on_load__

  def __on_load__ do
    path = :filename.join(:code.priv_dir(:xav), 'libxav')
    :ok = :erlang.load_nif(path, 0)
  end

  def new_reader(_path, _device), do: :erlang.nif_error(:undef)

  def next_frame(_reader), do: :erlang.nif_error(:undef)
end
