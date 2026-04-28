defmodule XavTest do
  use ExUnit.Case, async: false

  describe "set_log_level/1" do
    test "accepts atoms" do
      assert :ok = Xav.set_log_level(:error)
      assert :ok = Xav.set_log_level(:quiet)
      # restore default for other tests
      assert :ok = Xav.set_log_level(:info)
    end

    test "accepts integers" do
      assert :ok = Xav.set_log_level(16)
      assert :ok = Xav.set_log_level(:info)
    end

    test "rejects unknown atoms" do
      assert_raise ArgumentError, ~r/invalid FFmpeg log level/, fn ->
        Xav.set_log_level(:nonsense)
      end
    end
  end
end
