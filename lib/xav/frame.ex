defmodule Xav.Frame do
  @moduledoc """
  Audio/video frame.
  """

  @typedoc """
  Possible audio samples formats.

  To get the complete list of sample formats, check `Xav.sample_formats/0`.
  """
  @type audio_format() :: atom()

  @typedoc """
  Possible video frame formats.

  To get the complete list of pixel formats, check `Xav.pixel_formats/0`.

  An example of a pixel format is `:rgb24`.
  """
  @type video_format() :: atom()

  @type format() :: audio_format() | video_format()

  @type width :: non_neg_integer() | nil
  @type height :: non_neg_integer() | nil

  @type t() :: %__MODULE__{
          type: :audio | :video,
          data: binary(),
          format: format(),
          width: width(),
          height: height(),
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

    In case of a video frame, dimension names of the newly created tensor are `[:height, :width, :channels]`.

    For video frames, the only supported pixel formats are:
      * `:rgb24`
      * `:bgr24`
    """
    @spec to_nx(t()) :: Nx.Tensor.t()
    def to_nx(%__MODULE__{type: :video, format: format} = frame)
        when format in [:rgb24, :bgr24] do
      frame.data
      |> Nx.from_binary(:u8)
      |> Nx.reshape({frame.height, frame.width, 3}, names: [:height, :width, :channels])
    end

    def to_nx(%__MODULE__{type: :audio} = frame) do
      Nx.from_binary(frame.data, normalize_format(frame.format))
    end

    defp normalize_format(:flt), do: :f32
    defp normalize_format(:fltp), do: :f32
    defp normalize_format(:dbl), do: :f64
    defp normalize_format(:dblp), do: :f64
    defp normalize_format(:u8p), do: :u8
    defp normalize_format(:s16p), do: :s16
    defp normalize_format(:s32p), do: :s32
    defp normalize_format(:s64p), do: :s64
    defp normalize_format(format), do: format
  end
end
