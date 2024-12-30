defmodule Xav.Frame do
  @moduledoc """
  Audio/video frame.
  """

  @typedoc """
  Possible audio samples formats.
  """
  @type audio_format() :: :u8 | :s16 | :s32 | :s64 | :f32 | :f64

  @typedoc """
  Possible video frame formats.

  Currently, only RGB is supported.
  """
  @type video_format() :: :rgb

  @type format() :: audio_format() | video_format()

  @type t() :: %__MODULE__{
          type: :audio | :video,
          data: binary(),
          format: format(),
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

  @doc """
  Creates a new audio/video frame.
  """
  @spec new(binary(), format(), non_neg_integer(), non_neg_integer(), integer()) :: t()
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

  @spec new(binary(), format(), integer(), integer()) :: t()
  def new(data, format, samples, pts) do
    %__MODULE__{
      type: :audio,
      data: data,
      format: format,
      samples: samples,
      pts: pts
    }
  end

  if Code.ensure_loaded?(Nx) do
    @doc """
    Converts a frame to an Nx tensor.
    """
    @spec to_nx(t()) :: Nx.Tensor.t()
    def to_nx(%__MODULE__{type: :video} = frame) do
      frame.data
      |> Nx.from_binary(:u8)
      |> Nx.reshape({frame.height, frame.width, 3}, names: [:height, :width, :channels])
    end

    def to_nx(%__MODULE__{type: :audio} = frame) do
      Nx.from_binary(frame.data, frame.format)
    end
  end
end
