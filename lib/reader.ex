defmodule Xav.Reader do
  @moduledoc """
  File/network reader.
  """

  @type t() :: %__MODULE__{reader: reference()}

  @enforce_keys [:reader]
  defstruct @enforce_keys

  @doc """
  The same as new/1 but raises on error.
  """
  @spec new!(String.t(), boolean(), boolean()) :: t()
  def new!(path, video? \\ true, device? \\ false) do
    case new(path, video?, device?) do
      {:ok, reader} -> reader
      {:error, reason} -> raise "Couldn't create a new reader. Reason: #{inspect(reason)}"
    end
  end

  @doc """
  Creates a new file reader.
  """
  @spec new(String.t(), boolean(), boolean()) :: {:ok, t()} | {:error, term()}
  def new(path, video? \\ true, device? \\ false) do
    case Xav.NIF.new_reader(path, to_int(device?), to_int(video?)) do
      {:ok, reader} -> {:ok, %__MODULE__{reader: reader}}
      {:error, _reason} = err -> err
    end
  end

  @doc """
  Reads next video frame.

  Frame is always decoded. Pixel format is always RGB.
  See also `Xav.Frame.to_nx/1`.
  """
  @spec next_frame(t()) :: {:ok, Xav.Frame.t()} | {:error, :eof}
  def next_frame(%__MODULE__{reader: reader}) do
    case Xav.NIF.next_frame(reader) do
      {:ok, {data, width, height, pts}} ->
        {:ok, Xav.Frame.new(data, width, height, pts)}

      {:error, :eof} = err ->
        err
    end
  end

  defp to_int(true), do: 1
  defp to_int(false), do: 0
end
