defmodule Xav.VideoConverter.NIF do
  @moduledoc false

  @on_load :__on_load__

  def __on_load__ do
    path = :filename.join(:code.priv_dir(:xav), ~c"libxavvideoconverter")
    :ok = :erlang.load_nif(path, 0)
  end

  def new(_in_width, _in_height, _in_format, _out_format), do: :erlang.nif_error(:undef)

  def convert(_converter, _frame, _width, _height), do: :erlang.nif_error(:undef)
end
