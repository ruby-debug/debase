#!/usr/bin/env ruby
require File.expand_path("helper", File.dirname(__FILE__))

class TestReloadBug < Test::Unit::TestCase
  def test_reload_bug
    assert_equal({}, Debugger::source_reload)
  end
end
