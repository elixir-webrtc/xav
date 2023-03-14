defmodule Xav.MixProject do
  use Mix.Project

  def project do
    [
      app: :xav,
      version: "0.1.0",
      elixir: "~> 1.14",
      start_permanent: Mix.env() == :prod,
      compilers: [:elixir_make] ++ Mix.compilers(),
      deps: deps()
    ]
  end

  # Run "mix help compile.app" to learn about applications.
  def application do
    [
      extra_applications: [:logger]
    ]
  end

  # Run "mix help deps" to learn about dependencies.
  defp deps do
    [
      {:nx, "~> 0.5"},
      {:elixir_make, "~> 0.7", runtime: false}
    ]
  end
end
