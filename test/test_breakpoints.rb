#!/usr/bin/env ruby
require File.expand_path("helper", File.dirname(__FILE__))

# Some tests of Debugger module in C extension ruby_debug
class TestBreakpoints < Test::Unit::TestCase
  def test_find
    Debugger.start
    Debugger.add_breakpoint("foo.rb", 11, nil)
    assert_not_nil(Debugger::Breakpoint.find(Debugger.breakpoints, "foo.rb", 11))
    assert_nil(Debugger::Breakpoint.find(Debugger.breakpoints, "bar.rb", 11))
    assert_nil(Debugger::Breakpoint.find(Debugger.breakpoints, "foo.rb", 10))
    Debugger.stop
  end
end