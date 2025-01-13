defmodule Xav.VideoConverter.NIF do
  @moduledoc false

  @on_load :__on_load__

  def __on_load__ do
    path = :filename.join(:code.priv_dir(:xav), ~c"libxavvideoconverter")
    :ok = :erlang.load_nif(path, 0)
  end

  def new(_format), do: :erlang.nif_error(:undef)

  def convert(_converter, _frame, _width, _height, _pix_format), do: :erlang.nif_error(:undef)
end
