#!/usr/bin/env ruby
require File.expand_path("helper", File.dirname(__FILE__))

# Test catchpoint in C ruby_debug extension.
  
class TestRubyDebugCatchpoint < Test::Unit::TestCase
  def test_catchpoints
    assert_equal({}, Debugger.catchpoints)
    Debugger.start_
    assert_equal({}, Debugger.catchpoints)
    Debugger.add_catchpoint('ZeroDivisionError')
    assert_equal({'ZeroDivisionError' => 0}, Debugger.catchpoints)
    Debugger.add_catchpoint('RuntimeError')
    assert_equal(['RuntimeError', 'ZeroDivisionError'], 
                 Debugger.catchpoints.keys.sort)
  ensure
    Debugger.stop
  end
end
