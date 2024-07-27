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
      compilers: [:native] ++ Mix.compilers(),
      aliases: [
        "compile.native": &native/1
      ],
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

  defp native(_) do
    Mix.shell().info("""
    Xav requires FFmpeg development packages to be installed on your system. \
    Make sure paths to ffmpeg header and library files are in C_INCLUDE_PATH, \
    LIBRARY_PATH and LD_LIBRARY_PATH. For more information refer to Xav README \
    under: https://github.com/mickel8/xav#installation \
    """)

    Mix.Tasks.Compile.ElixirMake.run([])
    :ok
  end
end
