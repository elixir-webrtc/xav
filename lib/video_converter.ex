defmodule Xav.VideoConverter do
  @moduledoc """
  Video samples converter.

  It supports pixel format conversion and/or scaling.
  """

  alias Xav.Frame
  alias Xav.VideoConverter.NIF

  @type t :: %__MODULE__{
          converter: reference(),
          out_format: Frame.video_format(),
          out_width: Frame.width(),
          out_height: Frame.height()
        }

  @typedoc """
  Type definition for converter options.

  * `out_format` - video format to convert to (`e.g. :rgb24`).
  * `out_width` - scale the video frame to this width.
  * `out_height` - scale the video frame to this height.

  If `out_width` and `out_height` are both not provided, scaling is not performed. If one of the
  dimensions is `nil`, the other will be calculated based on the input dimensions as
  to keep the aspect ratio.
  """
  @type converter_opts() :: [
          out_format: Frame.video_format(),
          out_width: Frame.width(),
          out_height: Frame.height()
        ]

  defstruct [:converter, :out_format, :out_width, :out_height]

  @doc """
  Creates a new video converter.
  """
  @spec new(converter_opts()) :: t()
  def new(converter_opts) do
    opts = Keyword.validate!(converter_opts, [:out_format, :out_width, :out_height])

    if is_nil(opts[:out_format]) and is_nil(opts[:out_width]) and is_nil(opts[:out_height]) do
      raise "At least one of `out_format`, `out_width` or `out_height` must be provided"
    end

    converter = NIF.new(opts[:out_format], opts[:out_width] || -1, opts[:out_height] || -1)

    %__MODULE__{
      converter: converter,
      out_format: opts[:out_format],
      out_width: opts[:out_width],
      out_height: opts[:out_height]
    }
  end

  @doc """
  Converts a video frame.
  """
  @spec convert(t(), Frame.t()) :: Frame.t()
  def convert(
        %__MODULE__{out_format: format, out_width: nil, out_height: nil},
        %Frame{format: format} = frame
      ),
      do: frame

  def convert(%__MODULE__{converter: converter}, frame) do
    {data, out_format, width, height, _pts} =
      NIF.convert(converter, frame.data, frame.width, frame.height, frame.format)

    %Frame{
      type: frame.type,
      data: data,
      format: out_format,
      width: width,
      height: height,
      pts: frame.pts
    }
  end
end
