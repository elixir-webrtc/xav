defmodule Xav.MixProject do
  use Mix.Project

  # https://github.com/mickel8/releases/download/0.1.0/ffmpeg_5.0.1-x86_64-linux.tar.gz

  @precompiled_ffmpeg_platforms []

  def project do
    [
      app: :xav,
      version: "0.2.0",
      elixir: "~> 1.14",
      start_permanent: Mix.env() == :prod,
      description: "Elixir media library built on top of FFmpeg",
      package: package(),
      compilers: [:native] ++ Mix.compilers(),
      make_env: %{"USE_BUNDLED_FFMPEG" => "#{get_platform() in @precompiled_ffmpeg_platforms}"},
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
      {:nx, "~> 0.6.0"},
      {:elixir_make, "~> 0.7", runtime: false},
      {:ex_doc, ">= 0.0.0", runtime: false, only: :dev},
      {:credo, ">= 0.0.0", runtime: false, only: :dev},
      {:dialyxir, ">= 0.0.0", runtime: false, only: :dev}
    ]
  end

  defp get_platform() do
    :erlang.system_info(:system_architecture)
    |> List.to_string()
    |> String.split("-")
    |> case do
      ["x86_64" = arch, _vendor, "linux" = system, _abi] ->
        {arch, system}

      [arch, _vendor, "darwin" <> _ | _] ->
        {arch, "darwin"}

      ["win32"] ->
        {"x86_64", "windows"}
    end
  end

  defp native(_) do
    {arch, system} = platform = get_platform()

    if platform in @precompiled_ffmpeg_platforms do
      download_ffmpeg(arch, system)
    else
      Mix.shell().info("""
      Xav requires FFmpeg development packages to be installed on your system. \
      Make sure paths to ffmpeg header and library files are in C_INCLUDE_PATH, \
      LIBRARY_PATH and LD_LIBRARY_PATH. For more information refer to Xav README \
      under: https://github.com/mickel8/xav#installation \
      """)
    end

    Mix.Tasks.Compile.ElixirMake.run([])
    :ok
  end

  defp download_ffmpeg(arch, system) do
    ffmpeg_version = File.read!("ffmpeg-version") |> String.trim()
    Mix.shell().info("Downloading precompiled ffmpeg for #{arch}-#{system}")
    # TODO
    Mix.shell().info("Unpacking precompiled ffmpeg")

    ffmpeg_archive_path = Path.join(__DIR__, "ffmpeg_#{ffmpeg_version}-#{arch}-#{system}.tar.gz")

    case :erl_tar.extract(ffmpeg_archive_path, [:compressed]) do
      :ok ->
        :ok

      {:error, reason} ->
        Mix.raise("Failed to extract ffmpeg archive, reason: #{inspect(reason)}")
    end

    :ok
  end
end
