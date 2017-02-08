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
    attr_reader :unreachable_breakpoints

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
      @breakpoint_add = Breakpoint.new(file, line, expr)
      @track_breakpoint_add.enable
      add_breakpoint_stub
      @track_breakpoint_add.disable
      @breakpoint_add
    end

    def remove_breakpoint(id)
      @bp_id = id
      @track_breakpoint_delete.enable
      remove_breakpoint_stub
      @track_breakpoint_delete.disable
      @breakpoint_remove
    end

    def remove_all_breakpoints
      @bp_id = :all
      @track_breakpoint_delete.enable
      remove_breakpoint_stub
      @track_breakpoint_delete.disable
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

    def add_breakpoint_stub

    end

    def remove_breakpoint_stub

    end

    def init_breakpoints
      @mutex = Mutex.new
      @unreachable_breakpoints = {}
      @breakpoint_add = nil
      @breakpoint_remove = nil
      @bp_id = nil

      # activated on each require/load calls to
      # update list of loaded files
      @track_file_load = TracePoint.new(:c_call) do |tp|
        if [:require, :require_relative, :load].include? tp.method_id
          th = Thread.current
          break if th.instance_of? DebugThread
          # enable tracing only for 1 line after load
          # just to get required file name
          th.set_trace_func proc { |event, file, _, _, _, _|
            if event == 'line'
              @mutex.synchronize do
                file_loaded File.expand_path(file)
              end
              th.set_trace_func nil
            end
          }
        end
      end

      # activated on each add_breakpoint action to
      # actually add new breakpoint
      @track_breakpoint_add = TracePoint.new(:call) do |tp|
        if tp.method_id == :add_breakpoint_stub
          @mutex.synchronize do
            # no need in updating if file was already loaded
            unless file_loaded?(@breakpoint_add.source) || @track_file_load.enabled?
              update_loaded_files
            end
            if file_loaded?(@breakpoint_add.source)
              breakpoints << @breakpoint_add
              enable_trace_points
            else
              @unreachable_breakpoints[@breakpoint_add.source] ||= []
              @unreachable_breakpoints[@breakpoint_add.source] << @breakpoint_add
              @track_file_load.enable
            end
          end
        end
      end

      # activated on each remove_breakpoint action
      # searches for bp in @breakpoints and @unreachable_breakpoints
      @track_breakpoint_delete = TracePoint.new(:call) do |tp|
        if tp.method_id == :remove_breakpoint_stub
          @mutex.synchronize do
            if @bp_id != :all
              if breakpoints.any? {|bp| bp.id == @bp_id}
                @breakpoint_remove = Breakpoint.remove breakpoints, @bp_id
              else
                @unreachable_breakpoints.each_key do |key|
                  @breakpoint_remove ||= Breakpoint.remove @unreachable_breakpoints[key], @bp_id
                end
              end
            else
              breakpoints.clear
              @unreachable_breakpoints.clear
            end
            if @unreachable_breakpoints.size == 0
              @track_file_load.disable
            end
          end
        end
      end

      update_loaded_files
    end

    # This method should be called with @mutex locked
    def update_loaded_files
      @loaded_files = Set.new

      $LOADED_FEATURES.each do |path|
        file_loaded path
      end

      file_loaded File.expand_path(Debugger::PROG_SCRIPT)

      Thread.list.each do |th|
        next if th.instance_of? DebugThread
        th.backtrace_locations.each do |location|
          file_loaded location.absolute_path
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
      if @unreachable_breakpoints.size == 0
        @track_file_load.disable
      end
    end

    # This method should be called with @mutex locked
    def file_loaded?(file_path)
      @loaded_files.include? file_path
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
