require "debase_internals"
require "debase/version"
require "debase/context"

module Debase
  class << self
    attr_accessor :handler

    # possibly deprecated options
    attr_accessor :keep_frame_binding, :tracing

    def start(options={}, &block)
      Debugger.const_set('ARGV', ARGV.clone) unless defined? Debugger::ARGV
      Debugger.const_set('PROG_SCRIPT', $0) unless defined? Debugger::PROG_SCRIPT
      Debugger.const_set('INITIAL_DIR', Dir.pwd) unless  defined? Debugger::INITIAL_DIR
      return Debugger.started? ? block && block.call(self) : Debugger.start_(&block) 
    end

    def start_
      setup_tracepoints
    end

    def stop
      remove_tracepoints
    end

    def debug_load(file, stop = false, increment_start = false)
      begin
        start
        prepare_context(file, stop)
        load file
        return nil
      rescue Exception => e
        return e
      end
    end

    # @param [String] file
    # @param [Fixnum] line
    # @param [String] expr
    def add_breakpoint(file, line, expr=nil)
      breakpoint = Breakpoint.new(file, line, expr)
      breakpoints << breakpoint
      breakpoint
    end

    def remove_breakpoint(id)
      Breakpoint.remove breakpoints, id
    end

    def source_reload; {} end

    def post_mortem?
      false
    end

    def debug
      false
    end

    def add_catchpoint(exception)
      catchpoints[exception] = 0
    end
  end
  
  class DebugThread < Thread
    def self.inherited
      raise RuntimeError.new("Can't inherit Debugger::DebugThread class")
    end  
  end
end

Debugger = Debase
