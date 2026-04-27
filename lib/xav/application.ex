defmodule Xav.Application do
  @moduledoc false

  use Application

  @impl true
  def start(_type, _args) do
    maybe_set_log_level()

    Supervisor.start_link([], strategy: :one_for_one, name: Xav.Supervisor)
  end

  defp maybe_set_log_level do
    case Application.get_env(:xav, :ffmpeg_log_level) do
      nil -> :ok
      level -> Xav.set_log_level(level)
    end
  end
end
