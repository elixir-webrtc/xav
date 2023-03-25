defmodule Xav do
  @moduledoc """
  Main module for interacting with `Xav`.

  `Xav` is an Elixir wrapper over FFmpeg intended for
  reading file and network based media streams.

  It doesn't map FFmpeg functions one to one but rather
  wraps them in bigger building blocks creating a litte
  higher abstraction level.
  """

  defdelegate new_reader!(path, video? \\ true, device? \\ false), to: Xav.Reader, as: :new!

  defdelegate new_reader(path, video? \\ true, device? \\ false), to: Xav.Reader, as: :new

  defdelegate next_frame(reader), to: Xav.Reader

  defdelegate to_nx(frame), to: Xav.Frame
end
