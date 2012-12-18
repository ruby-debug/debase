require "debase_internals"
require "debase/version"
require "debase/context"
require "debase/catchpoint"
require "debase/breakpoint"

module Debase
  class << self
    attr_reader :breakpoints
    attr_accessor :handler

    # possibly deprecated options
    attr_accessor :keep_frame_binding, :tracing

    def started?
     !!@contexts
    end

    def start(options={}, &block)      
      @contexts = {}
      @breakpoints = []
      @catchpoints = {}
      @breakpoints_count = 0
      @locked = []
      Debase.const_set('PROG_SCRIPT', $0) unless defined? Debugger::PROG_SCRIPT
      Debase.const_set('INITIAL_DIR', Dir.pwd) unless defined? Debugger::INITIAL_DIR
    end 

    def stop
      @contexts = nil
      @breakpoints = nil
      @catchpoints = nil
    end

    def debug_load(file, stop = false, increment_start = false)
      begin
        setup_tracepoints
        load file
        return nil
      rescue Exception => e
        return e
      end
    end

    # @param [String] file
    # @param [Fixnum] line
    # @param [String] expr
    def add_breakpoint(file, line, expr)
      @breakpoints_count = @breakpoints_count + 1
      breakpoint = Breakpoint.new(@breakpoints_count, file, line, expr)
      @breakpoints << breakpoint
      breakpoint
    end

    def remove_breakpoint(id)
      Breakpoint.remove @breakpoints, id
    end

    def source_reload; {} end

    def post_mortem?
      false
    end

    def debug
      false
    end

    def contexts
      @contexts.values
    end

    def catchpoints
      check_started
      @catchpoints
    end

    def add_catchpoint(exception)
      @catchpoints[exception] = 0
    end

    private
    def check_started
      raise RuntimeError.new unless started?
    end

    def check_not_started
      raise RuntimeError.new if started?
    end

    def starts_with?(what, prefix)
      prefix = prefix.to_s
      what[0, prefix.length] == prefix
    end
  end
  
  class DebugThread < Thread
    def inherited
      raise RuntimeError.new("Can't inherit Debugger::DebugThread class")
    end  
  end
end

Debugger = Debase
