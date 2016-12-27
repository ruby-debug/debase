#!/usr/bin/env ruby
require File.expand_path("helper", File.dirname(__FILE__))

# Some tests of Debugger module in C extension ruby_debug
class TestBreakpoints < Test::Unit::TestCase
  def test_find
    Debugger.start
    Debugger.add_breakpoint("foo.rb", 11, nil)
    assert_not_nil(Debugger::Breakpoint.find(Debugger.breakpoints, "foo.rb", 11, nil))
    assert_nil(Debugger::Breakpoint.find(Debugger.breakpoints, "bar.rb", 11, nil))
    assert_nil(Debugger::Breakpoint.find(Debugger.breakpoints, "foo.rb", 10, nil))
  end

  def test_conditional_true_expression
    Debugger.start
    Debugger.add_breakpoint("foo.rb", 11, "[1, 2, 3].length == 3")
    assert_not_nil(Debugger::Breakpoint.find(Debugger.breakpoints, "foo.rb", 11, nil))
    Debugger.stop
  end

  def test_conditional_false_expression
    Debugger.start
    Debugger.add_breakpoint("foo.rb", 11, "(2 + 2) == 5")
    assert_nil(Debugger::Breakpoint.find(Debugger.breakpoints, "foo.rb", 11, nil))
    Debugger.stop
  end

  def test_conditional_undefined_variable
    Debugger.start
    Debugger.add_breakpoint("foo.rb", 11, "this_variable_does_not_exist")
    assert_nil(Debugger::Breakpoint.find(Debugger.breakpoints, "foo.rb", 11, nil))
    Debugger.stop
  end
end