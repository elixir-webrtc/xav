defmodule Xav.VideoConverter do
  @moduledoc """
  Video samples converter.

  Currently it only supports pixel format conversion.
  """

  alias Xav.Frame
  alias Xav.VideoConverter.NIF

  @type t :: reference()

  @doc """
  Creates a new video converter.

  All the fields are required. The converter re-initializes itself in case the resolution
  changes. However `in_format` and `out_format` must remain the same.
  """
  @spec new(non_neg_integer(), non_neg_integer(), Frame.video_format(), Frame.video_format()) ::
          {:ok, reference()} | {:error, any()}
  def new(in_width, in_height, in_pix_fmt, out_pix_fmt) do
    NIF.new(in_width, in_height, in_pix_fmt, out_pix_fmt)
  end

  @doc """
  Same as `new/4` but raises an exception in case of an error.
  """
  @spec new!(non_neg_integer(), non_neg_integer(), Frame.video_format(), Frame.video_format()) ::
          reference()
  def new!(in_width, in_height, in_pix_fmt, out_pix_fmt) do
    case new(in_width, in_height, in_pix_fmt, out_pix_fmt) do
      {:ok, ref} -> ref
      {:error, reason} -> raise "Couldn't create a video converter. Reason: #{inspect(reason)}"
    end
  end

  @spec convert(t(), Frame.t()) :: Frame.t()
  def convert(converter, frame) do
    {data, out_format, width, height, _pts} =
      NIF.convert(converter, frame.data, frame.width, frame.height)

    %Frame{
      type: :video,
      data: data,
      format: out_format,
      width: width,
      height: height,
      pts: frame.pts
    }
  end
end
