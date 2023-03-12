defmodule XavTest do
  use ExUnit.Case
  doctest Xav

  test "greets the world" do
    assert Xav.hello() == :world
  end
end
