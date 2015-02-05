if defined?(RUBY_ENGINE) && 'rbx' == RUBY_ENGINE
  require 'debase/rbx'
else
  require "debase_internals"
end
require "debase/version"
require "debase/context"

module Debase
  class << self
    attr_accessor :handler

    alias start_ setup_tracepoints
    alias stop remove_tracepoints

    # possibly deprecated options
    attr_accessor :keep_frame_binding, :tracing

    def start(options={}, &block)
      Debugger.const_set('ARGV', ARGV.clone) unless defined? Debugger::ARGV
      Debugger.const_set('PROG_SCRIPT', $0) unless defined? Debugger::PROG_SCRIPT
      Debugger.const_set('INITIAL_DIR', Dir.pwd) unless  defined? Debugger::INITIAL_DIR
      return Debugger.started? ? block && block.call(self) : Debugger.start_(&block) 
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

    #call-seq:
    #   Debase.skip { block } -> obj or nil
    #
    #The code inside of the block is escaped from the debugger.
    def skip
      #it looks like no-op is ok for this method for now
      #no-op
    end

    #call-seq:
    #   Debugger.last_interrupted -> context
    #
    #Returns last debugged context.
    def last_context
      # not sure why we need this so let's return nil for now ;)
      nil
    end
  end
  
  class DebugThread < Thread
    def self.inherited
      raise RuntimeError.new("Can't inherit Debugger::DebugThread class")
    end  
  end
end

Debugger = Debase
