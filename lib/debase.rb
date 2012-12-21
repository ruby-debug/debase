require "debase_internals"
require "debase/version"
require "debase/context"

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
      Debugger.const_set('ARGV', ARGV.clone) unless defined? Debugger::ARGV
      Debugger.const_set('PROG_SCRIPT', $0) unless defined? Debugger::PROG_SCRIPT
      Debugger.const_set('INITIAL_DIR', Dir.pwd) unless  defined? Debugger::INITIAL_DIR
      return Debugger.started? ? block && block.call(self) : Debugger.start_(&block) 
    end

    def start_
      @contexts = {}
      @breakpoints = []
      @catchpoints = {}
      @locked = []
      setup_tracepoints
    end

    def stop
      remove_tracepoints
      @contexts = nil
      @breakpoints = nil
      @catchpoints = nil
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
    def self.inherited
      raise RuntimeError.new("Can't inherit Debugger::DebugThread class")
    end  
  end
end

Debugger = Debase
