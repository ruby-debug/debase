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
      set_trace_func(proc do |event, file, line, id, binding, klass|
        process(event, file, line, id, binding, klass)
      end)
    end

    def stop
      set_trace_func nil
      @contexts = nil
      @breakpoints = nil
      @catchpoints = nil
    end

    def process(event, file, line, id, binding, klass)
      update_contexts

      if event == "line" && !ignored?(file)
        current_context.update(file, line, binding, klass)
        b = Breakpoint.find(@breakpoints, file, line)
        current_context.at_breakpoint b if b
        current_context.at_line file, line
      end

      if (event == "call" || event == "c-call") && !ignored?(file)
        current_context.push(file, line, binding, klass)
      end

      if (event == "return" || event == "c-return") && !ignored?(file)
        current_context.at_return file, line
        current_context.pop
      end
    end

    def update_contexts
      list = Thread.list
      list.each do |thread|
        unless @contexts[thread]
          @contexts[thread] << Context.new(thread)
        end
      end

      @contexts.each_key do |key|
        @contexts.delete(key) unless key.alive?
      end
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
