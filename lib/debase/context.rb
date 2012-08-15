require "debase/frame"

module Debase
  class Context
    def initialize(thread)
      @stack = []
      @thread = thread
    end

    def push(file, line, binding, klass)
      @stack << Frame.new(file, line, binding, klass)
    end

    def update(file, line, binding, klass)
      frame = @stack.last
      unless frame
        push(file, line, binding, klass)
        return
      end

      frame.file = file
      frame.line = line
      frame.bind = binding
      frame.klass = klass
    end

    def dead?
      !@thread.alive?
    end

    def pop
      @stack.pop
    end

    def frame_line(frame_no=0)
      raise ArgumentError.new("Invalid frame number #{frame_no}, stack (0..#{@stack.length - 1})") unless frame_no <= @stack.length
      @stack[@stack.length - frame_no - 1].line
    end

    def frame_file(frame_no=0)
      raise ArgumentError.new("Invalid frame number #{frame_no}, stack (0..#{@stack.length - 1})") unless frame_no <= @stack.length
      @stack[@stack.length - frame_no - 1].file
    end

    def frame_class(frame_no=0)
      raise ArgumentError.new("Invalid frame number #{frame_no}, stack (0..#{@stack.length - 1})") unless frame_no <= @stack.length
      @stack[@stack.length - frame_no - 1].klass
    end

    def stack_size
      @stack.length
    end

    def frame_args_info(frame_no=0)
      #return nil unless frame_no <= @stack.length
      #@stack[@stack.length - frame_no - 1].bind
    end

    def at_breakpoint(breakpoint)
      #handler.at_breakpoint(self, breakpoint)
    end

    def at_catchpoint(excpt)
      #handler.at_catchpoint(self, excpt)
    end

    def at_tracing(file, line)
      @tracing_started = true if File.identical?(file, File.join(Debugger::INITIAL_DIR, Debugger::PROG_SCRIPT))
      #handler.at_tracing(self, file, line) if @tracing_started
    end

    def at_line(file, line)
      #handler.at_line(self, file, line)
    end

    def at_return(file, line)
      #handler.at_return(self, file, line)
    end
  end
end