defmodule Xav.MixProject do
  use Mix.Project

  def project do
    [
      app: :xav,
      version: "0.4.0",
      elixir: "~> 1.14",
      start_permanent: Mix.env() == :prod,
      description: "Elixir media library built on top of FFmpeg",
      package: package(),
      compilers: [:elixir_make] ++ Mix.compilers(),
      deps: deps()
    ]
  end

  def application do
    [
      extra_applications: [:logger]
    ]
  end

  defp package do
    [
      files: ~w(lib .formatter.exs mix.exs README* LICENSE* c_src Makefile),
      licenses: ["MIT"],
      links: %{"GitHub" => "https://github.com/mickel8/xav"}
    ]
  end

  defp deps do
    [
      {:nx, "~> 0.7.0"},
      {:elixir_make, "~> 0.7", runtime: false},
      {:ex_doc, ">= 0.0.0", runtime: false, only: :dev},
      {:credo, ">= 0.0.0", runtime: false, only: :dev},
      {:dialyxir, ">= 0.0.0", runtime: false, only: :dev}
    ]
  end
end
