defmodule Xav do
  @moduledoc """
  Documentation for `Xav`.
  """

  @enforce_keys [:reader]
  defstruct @enforce_keys

  def new_reader(path) do
    reader = Xav.NIF.new_reader(path)

    %__MODULE__{
      reader: reader
    }
  end

  def next_frame(%__MODULE__{reader: reader}) do
    Xav.NIF.next_frame(reader)
  end
end
