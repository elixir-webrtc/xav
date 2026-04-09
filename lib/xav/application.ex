defmodule Xav.Application do
  @moduledoc false

  use Application

  @impl true
  def start(_type, _args) do
    maybe_set_log_level()

    Supervisor.start_link([], strategy: :one_for_one, name: Xav.Supervisor)
  end

  # Applies the `:xav, :log_level` application env value, if set.
  # Without this, FFmpeg's default log level (`AV_LOG_INFO`)
  # remains unchanged, preserving existing behaviour for users who
  # don't opt in.
  defp maybe_set_log_level do
    case Application.get_env(:xav, :log_level) do
      nil -> :ok
      level -> Xav.Reader.set_log_level(level)
    end
  end
end
