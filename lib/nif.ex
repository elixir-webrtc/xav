defmodule Xav.NIF do
  @on_load :__on_load__

  def __on_load__ do
    path = :filename.join(:code.priv_dir(:xav), 'libxav')
    :ok = :erlang.load_nif(path, 0)
  end
end
