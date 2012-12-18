module Debase
  class Context
    def frame_locals(frame_no=0)
      result = {}
      binding = frame_binding(frame_no)
      locals = eval("local_variables", binding)
      locals.each {|local| result[local] = eval(local.to_s, binding)}
      result
    end  

    def frame_class(frame_no=0)
      raise ArgumentError.new("Invalid frame number #{frame_no}, stack (0..#{@stack.length - 1})") unless frame_no <= @stack.length
      @stack[@stack.length - frame_no - 1].klass
    end

    def handler
      Debase.handler or raise "No interface loaded, frame: #{frame_file}:#{frame_line}"
    end

    def at_breakpoint(breakpoint)
      handler.at_breakpoint(self, breakpoint)
    end

    def at_catchpoint(excpt)
      handler.at_catchpoint(self, excpt)
    end

    def at_tracing(file, line)
      @tracing_started = true if File.identical?(file, File.join(Debugger::INITIAL_DIR, Debugger::PROG_SCRIPT))
      handler.at_tracing(self, file, line) if @tracing_started
    end

    def at_line(file, line)
      handler.at_line(self, file, line)
    end

    def at_return(file, line)
      handler.at_return(self, file, line)
    end
  end
end