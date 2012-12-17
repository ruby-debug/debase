module Debase
  class Context
    attr_reader :thread

    def initialize(thread)
      @thread = thread
    end

    def dead?
      !@thread.alive?
    end

    def thnum
      @thread.t
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