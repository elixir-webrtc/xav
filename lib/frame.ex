defmodule Xav.Frame do
  @moduledoc """
  Video or audio frame.
  """

  @type t() :: %__MODULE__{
          type: :audio | :video,
          data: binary(),
          format: atom(),
          width: non_neg_integer() | nil,
          height: non_neg_integer() | nil,
          samples: integer() | nil,
          pts: integer()
        }

  defstruct [
    :type,
    :data,
    :format,
    :width,
    :height,
    :samples,
    :pts
  ]

  @spec new(binary(), atom(), non_neg_integer(), non_neg_integer(), integer()) :: t()
  def new(data, format, width, height, pts) do
    %__MODULE__{
      type: :video,
      data: data,
      format: format,
      width: width,
      height: height,
      pts: pts
    }
  end

  @spec new(binary(), atom(), integer(), integer()) :: t()
  def new(data, format, samples, pts) do
    %__MODULE__{
      type: :audio,
      data: data,
      format: format,
      samples: samples,
      pts: pts
    }
  end

  @doc """
  Converts frame to Nx tensor.
  """
  @spec to_nx(t()) :: Nx.Tensor.t()
  def to_nx(%__MODULE__{type: :video} = frame) do
    frame.data
    |> Nx.from_binary(:u8)
    |> Nx.reshape({frame.height, frame.width, 3})
  end

  def to_nx(%__MODULE__{type: :audio} = frame) do
    Nx.from_binary(frame.data, to_nx_format(frame.format))
  end

  defp to_nx_format(:u8), do: :u8
  defp to_nx_format(:s16), do: :s16
  defp to_nx_format(:s32), do: :s32
  defp to_nx_format(:s64), do: :s64
  defp to_nx_format(:flt), do: :f32
  defp to_nx_format(:dbl), do: :f64
end
