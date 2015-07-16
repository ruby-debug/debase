module Debase
  class Context
    def frame_locals(frame_no=0)
      result = {}
      binding = frame_binding(frame_no)
      locals = eval("local_variables", binding)
      if locals.respond_to?(:each)
        locals.each do |local|
          result[local.to_s] = safe_eval(local.to_s, binding)
        end
      else
        result[locals.to_s] = safe_eval(locals.to_s, binding)
      end
      result
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

    private

    def safe_eval(expr, binding)
      begin
        eval(expr, binding)
      rescue => e
        "*Evaluation error: '#{e}'"
      end
    end

  end
end
