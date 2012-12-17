$LOAD_PATH << File.dirname(__FILE__) + "/../ext"
require "debase_internals"
require "debase/version"
require "debase/context"
require "debase/catchpoint"
require "debase/breakpoint"

module Debase
  class << self
    IGNORED_FOLDER = File.expand_path(".", File.dirname(__FILE__))
    attr_reader :breakpoints

    def started?
     !!@contexts
    end

    def start_
      @contexts = {}
      @breakpoints = []
      @catchpoints = {}
      @breakpoints_count = 0
      Debase.const_set('PROG_SCRIPT', $0) unless defined? Debugger::PROG_SCRIPT
      Debase.const_set('INITIAL_DIR', Dir.pwd) unless defined? Debugger::INITIAL_DIR
      @contexts[Thread.current] = Context.new(Thread.current)
      setup_tracepoints
    end

    def setup_tracepoints
      trace = TracePoint.trace(:line) do |tp|
        current_context.update(tp)
        #b = Breakpoint.find(@breakpoints, file, line)
        #current_context.at_breakpoint b if b
        #current_context.at_line file, line
      end

      trace = TracePoint.trace(:call, :c_call, :b_call) do |tp|
        update_contexts
        current_context.push(tp)
      end


      trace = TracePoint.trace(:return, :c_return, :b_return) do |tp|
        update_contexts
        #current_context.at_return file, line
        current_context.pop
      end

    end  

    def stop
      @contexts = nil
      @breakpoints = nil
      @catchpoints = nil
    end

    def debug_load(file, stop = false, increment_start = false)
      begin
        start_
        load file
        return nil
      rescue Exception => e
        return e
      end
    end

    # @param [String] file
    # @param [Fixnum] line
    def add_breakpoint(file, line)
      @breakpoints_count = @breakpoints_count + 1
      breakpoint = Breakpoint.new(@breakpoints_count, file, line)
      @breakpoints << breakpoint
      breakpoint
    end

    def remove_breakpoint(id)
      Breakpoint.remove @breakpoints, id
    end

    def current_context
      @contexts[Thread.current]
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
    def ignored?(file)
      file == nil || starts_with?(file, IGNORED_FOLDER)
    end

    def check_started
      raise RuntimeError.new unless started?
    end

    def starts_with?(what, prefix)
      prefix = prefix.to_s
      what[0, prefix.length] == prefix
    end
  end
end

Debugger = Debase
