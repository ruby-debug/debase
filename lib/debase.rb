if defined?(RUBY_ENGINE) && 'rbx' == RUBY_ENGINE
  require 'debase/rbx'
else
  require "debase_internals"
end
require "debase/version"
require "debase/context"
require 'set'

module Debase
  class << self
    attr_accessor :handler

    alias start_ setup_tracepoints

    # possibly deprecated options
    attr_accessor :keep_frame_binding, :tracing

    def start(options={}, &block)
      Debugger.const_set('ARGV', ARGV.clone) unless defined? Debugger::ARGV
      Debugger.const_set('PROG_SCRIPT', $0) unless defined? Debugger::PROG_SCRIPT
      Debugger.const_set('INITIAL_DIR', Dir.pwd) unless  defined? Debugger::INITIAL_DIR
      init_breakpoints
      Debugger.started? ? block && block.call(self) : Debugger.start_(&block)
    end

    def stop
      remove_all_breakpoints
      remove_tracepoints
    end

    # @param [String] file
    # @param [Fixnum] line
    # @param [String] expr
    def add_breakpoint(file, line, expr=nil)
      breakpoint = Breakpoint.new(file, line, expr)
      do_synchronized do
        if file_loaded?(file)
          breakpoints << breakpoint
          enable_trace_points
        else
          @unreachable_breakpoints[file] ||= []
          @unreachable_breakpoints[file] << breakpoint
        end
      end
      # we need this update in order to
      # prevent race condition between
      # `file_loaded?` check and adding
      # breakpoint to @unreachable_breakpoints
      update_loaded_files
      breakpoint
    end

    def remove_breakpoint(id)
      removed_bp = nil
      do_synchronized do
        if breakpoints.any? {|bp| bp.id == id}
          removed_bp = Breakpoint.remove breakpoints, id
        else
          @unreachable_breakpoints.each_key do |key|
            removed_bp ||= Breakpoint.remove @unreachable_breakpoints[key], id
          end
        end
      end
      removed_bp
    end

    def remove_all_breakpoints
      do_synchronized do
        breakpoints.clear
        @unreachable_breakpoints.clear
      end
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
      enable_trace_points
    end

    def remove_catchpoint(exception)
      catchpoints.delete(exception)
    end

    def clear_catchpoints
      catchpoints.clear
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

    def file_filter
      @file_filter ||= FileFilter.new
    end

    private

    def init_breakpoints
      @mutex = Mutex.new
      @unreachable_breakpoints = {}
      @track_file_load = TracePoint.new(:c_call) do |tp|
        if [:require, :require_relative, :load].include? tp.method_id
          th = Thread.current
          break if th == Debugger.control_thread
          # enable tracing only for 1 line after load
          # just to get required file name
          th.set_trace_func proc { |event, file, _, _, _, _|
            if event == 'line'
              do_synchronized do
                file_loaded File.expand_path(file)
              end
              th.set_trace_func nil
            end
          }
        end
      end
      update_loaded_files
    end

    def update_loaded_files
      do_synchronized do
        @loaded_files = Set.new

        $LOADED_FEATURES.each do |path|
          file_loaded path
        end

        file_loaded File.expand_path(Debugger::PROG_SCRIPT)

        Thread.list.each do |th|
          next if th == Debugger.control_thread
          th.backtrace_locations.each do |location|
            file_loaded location.absolute_path
          end
        end
      end
    end

    # This method should be called with @mutex locked
    def file_loaded(file_path)
      @loaded_files.add file_path
      file_breaks = @unreachable_breakpoints.delete file_path
      if file_breaks && file_breaks.length > 0
        breakpoints.concat(file_breaks)
        enable_trace_points
      end
    end

    # This method should be called with @mutex locked
    def file_loaded?(file_path)
      @loaded_files.include? file_path
    end

    # run given block in synchronized section.
    # this block should not enable @track_file_load
    def do_synchronized
      @track_file_load.disable # without it we can deadlock
      ret = nil
      @mutex.synchronize do
        ret = yield
      end
      update_track_state
      ret
    end

    def update_track_state
      @track_file_load.disable
      @mutex.lock
      if @unreachable_breakpoints.size == 0
        @mutex.unlock
      else
        @mutex.unlock
        @track_file_load.enable
      end
    end

  end

  class FileFilter
    def initialize
      @enabled = false
    end

    def include(file_path)
      included << file_path unless excluded.delete(file_path)
    end

    def exclude(file_path)
      excluded << file_path unless included.delete(file_path)
    end

    def enable
      @enabled = true
      Debase.enable_file_filtering(@enabled);
    end

    def disable
      @enabled = false
      Debase.enable_file_filtering(@enabled);
    end

    def accept?(file_path)
      return true unless @enabled
      return false if file_path.nil?
      included.any? { |path| file_path.start_with?(path) } && excluded.all? { |path| !file_path.start_with?(path)}
    end

    private
    def included
      @included ||= []
    end

    def excluded
      @excluded ||= []
    end
  end

  class DebugThread < Thread
    def self.inherited
      raise RuntimeError.new("Can't inherit Debugger::DebugThread class")
    end  
  end
end

Debugger = Debase
