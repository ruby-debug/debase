#!/usr/bin/env ruby
require File.expand_path("helper", File.dirname(__FILE__))

# Test of Debugger.debug_load in C extension ruby_debug.so
class TestDebugLoad < Test::Unit::TestCase

  self.test_order = :defined

  class << self
    def at_line(file, line)
      @@at_line = [File.basename(file), line]
    end
  end

  class Debugger::Context
    def at_line(file, line)
      TestDebugLoad::at_line(file, line)
    end
  end

  def test_debug_load
    src_dir = File.dirname(__FILE__)
    prog_script = File.join(src_dir, 'example', 'gcd.rb')

    # Without stopping
    bt = Debugger.debug_load(prog_script, false)
    if bt.is_a?(Exception)
      STDERR.puts bt.backtrace.join("\n")
    end
    assert_equal(nil, bt)
    assert(Debugger.started?)
    Debugger.stop

    # With stopping
    bt = Debugger.debug_load(prog_script, true)
    assert_equal(nil, bt)
    assert_equal(['gcd.rb', 4], @@at_line)
    assert(Debugger.started?)
    Debugger.stop

    # Test that we get a proper backtrace on a script that raises 'abc'
    prog_script = File.join(src_dir, 'example', 'raise.rb')
    bt = Debugger.debug_load(prog_script, false)
    assert_equal('abc', bt.to_s)
    assert(Debugger.started?)
    Debugger.stop
  ensure
    Debugger.stop if Debugger.started?
  end

  module MyBootsnap
    def load_iseq(path)
      iseq = RubyVM::InstructionSequence.compile_file(path)

      Debugger.unset_iseq_flags(iseq)
      iseq
    end
  end

  def test_bootsnap
    @@at_line = nil
    src_dir = File.dirname(__FILE__)
    prog_script = File.join(src_dir, 'example', 'bootsnap', 'bootsnap.rb')

    class << RubyVM::InstructionSequence
      prepend MyBootsnap
    end
    bt = Debugger.debug_load(prog_script, true)
    assert_equal(nil, bt)
    assert_not_nil(@@at_line)
    if RUBY_VERSION >= '2.5' && RUBY_VERSION < '2.6'
      assert_equal(['debase.rb', 101], @@at_line)
    end

    assert(Debugger.started?)
    Debugger.stop

    class << RubyVM::InstructionSequence; self end.class_eval { undef_method :load_iseq }

  end
end
