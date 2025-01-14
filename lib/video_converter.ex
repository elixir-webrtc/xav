defmodule Xav.VideoConverter do
  @moduledoc """
  Video samples converter.

  It supports pixel format conversion and/or scaling.
  """

  alias Xav.Frame
  alias Xav.VideoConverter.NIF

  @type t :: %__MODULE__{
          converter: reference(),
          format: Frame.video_format(),
          width: Frame.width(),
          height: Frame.height()
        }

  @typedoc """
  Type definition for converter options.

  * `format` - video format to convert to (`e.g. :rgb24`).
  * `width` - scale the video frame to this width.
  * `height` - scale the video frame to this height.

  If `width` and `height` are both not provided, scaling is not performed. If one of the
  dimensions is `nil`, the other will be calculated based on the input dimensions as
  to keep the aspect ratio.
  """
  @type converter_opts() :: [
          format: Frame.video_format(),
          width: Frame.width(),
          height: Frame.height()
        ]

  defstruct [:converter, :format, :width, :height]

  @doc """
  Creates a new video converter.
  """
  @spec new(converter_opts()) :: {:ok, t()} | {:error, any()}
  def new(converter_opts) do
    opts = Keyword.validate!(converter_opts, [:format, :width, :height])

    if is_nil(opts[:format]) and is_nil(opts[:width]) and is_nil(opts[:height]) do
      raise "At least one of `format`, `width` or `height` must be provided"
    end

    with {:ok, converter} <- NIF.new(opts[:format], opts[:width] || -1, opts[:height] || -1) do
      {:ok,
       %__MODULE__{
         converter: converter,
         format: opts[:format],
         width: opts[:width],
         height: opts[:height]
       }}
    end
  end

  @doc """
  Same as `new/1` but raises an exception in case of an error.
  """
  @spec new!(converter_opts()) :: t()
  def new!(converter_opts) do
    case new(converter_opts) do
      {:ok, ref} -> ref
      {:error, reason} -> raise "Couldn't create a video converter. Reason: #{inspect(reason)}"
    end
  end

  @doc """
  Converts a video frame.
  """
  @spec convert(t(), Frame.t()) :: Frame.t()
  def convert(
        %__MODULE__{format: format, width: nil, height: nil},
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
