require 'debase/rbx/breakpoint'
require 'debase/rbx/context'
require 'debase/rbx/monkey'

module Debase
  class << self
    attr_reader :breakpoints, :current_context

    def setup_tracepoints
      return nil unless @catchpoints.nil?
      @contexts = {}
      @catchpoints = {}
      @breakpoints = []
      Rubinius::CodeLoader.compiled_hook.add proc { |script|
        @breakpoints.each { | b |
          if b.source == script.file_path
            exec, ip = script.compiled_code.locate_line(b.pos)
            if exec
              exec.set_breakpoint ip, b
            end
          end
        }
      }
      spinup_thread
      Thread.current.set_debugger_thread @thread
      self
    end

    def remove_tracepoints
    end

    def prepare_context(thread=Thread.current)
      cleanup_contexts
      @contexts[thread] ||= Context.new(thread)
    end

    def cleanup_contexts
      @contexts.delete_if { |key, _| !key.alive? }
    end

    def contexts
      @contexts.values
    end

    def debug_load(file, stop=false, incremental_start=true)
      setup_tracepoints
      prepare_context
      begin
        load file
        nil
      rescue Exception => e
        e
      end
    end

    def started?
      !@thread.nil?
    end

    def listen
      channel = nil
      while true
        channel << true if channel
        # Wait for someone to stop
        bp, thread, channel, locs = @local_channel.receive

        if bp
          context = prepare_context(thread)
          @current_context = context
          context.fill_frame_info(locs)
          context.at_breakpoint(bp)
          context.at_line(locs.first.file, locs.first.line)
          puts "resuming execution"
          context.clear_frame_info
          @current_context = nil
        end
      end
    end

    def spinup_thread
      return if @thread

      @local_channel = Rubinius::Channel.new

      @thread = DebugThread.new do
        begin
          listen
        rescue Exception => e
          puts e
          e.render("Listening")
        end
      end

      @thread.setup_control!(@local_channel)
    end
  end

  class DebugThread < Thread
    def set_debugger_thread
    end
  end
end
