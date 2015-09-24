module Debase
  class Context
    def frame_locals(frame_no=0)
      frame_binding(frame_no).eval('local_variables.inject({}){|__h, __v| __h[__v.to_s] = eval(__v.to_s); __h}')
    rescue => e
      {'debase-debug' => "*Evaluation error: '#{e}'" }
    end

    def frame_class(frame_no=0)
      frame_self(frame_no).class
    end

    def frame_args_info(frame_no=0)
      nil
    end

    def handler
      Debase.handler or raise "No interface loaded"
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
