defmodule Xav.MixProject do
  use Mix.Project

  @version "0.5.1"
  @source_url "https://github.com/elixir-webrtc/xav"

  def project do
    [
      app: :xav,
      version: @version,
      elixir: "~> 1.14",
      start_permanent: Mix.env() == :prod,
      description: "Elixir audio/video library built on top of FFmpeg",
      package: package(),
      compilers: [:elixir_make] ++ Mix.compilers(),
      deps: deps(),

      # docs
      docs: docs(),
      source_url: @source_url,

      # dialyzer
      dialyzer: [
        plt_local_path: "_dialyzer",
        plt_core_path: "_dialyzer"
      ],

      # code coverage
      test_coverage: [tool: ExCoveralls],
      preferred_cli_env: [
        coveralls: :test,
        "coveralls.detail": :test,
        "coveralls.post": :test,
        "coveralls.html": :test,
        "coveralls.json": :test
      ]
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
      licenses: ["Apache-2.0"],
      links: %{"GitHub" => "https://github.com/elixir-webrtc/xav"}
    ]
  end

  defp deps do
    [
      {:nx, "~> 0.7.0", optional: true},
      {:elixir_make, "~> 0.7", runtime: false},

      # dev/test
      # bumblebee and exla for testing speech to text
      {:bumblebee, "~> 0.5", only: :test},
      {:exla, ">= 0.0.0", only: :test},
      # other
      {:excoveralls, "~> 0.18.0", only: [:dev, :test], runtime: false},
      {:ex_doc, ">= 0.0.0", runtime: false, only: :dev},
      {:credo, ">= 0.0.0", runtime: false, only: :dev},
      {:dialyxir, ">= 0.0.0", runtime: false, only: :dev}
    ]
  end

  defp docs do
    [
      main: "readme",
      extras: ["README.md", "INSTALL.md"],
      source_ref: "v#{@version}",
      formatters: ["html"]
    ]
  end
end
