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
    start = System.monotonic_time(:millisecond)
    frame = Xav.NIF.next_frame(reader)

    (System.monotonic_time(:millisecond) - start)
    |> IO.inspect(label: :time_in_us)

    frame
  end
end
