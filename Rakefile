require "bundler/gem_tasks"
require 'rake/testtask'

BASE_TEST_FILE_LIST = Dir['test/**/*_test.rb']

desc "Remove built files"
task :clean do
  cd "ext" do
    if File.exists?("Makefile")
      sh "make clean"
      rm  "Makefile"
    end
    derived_files = Dir.glob(".o") + Dir.glob("*.so") + Dir.glob("*.bundle") 
    rm derived_files unless derived_files.empty?
  end
end

desc "Create the core debase shared library extension"
task :lib => :clean do
  Dir.chdir("ext") do
    system("#{Gem.ruby} extconf.rb && make")
    exit $?.to_i if $?.to_i != 0
  end
end

desc "Test debase."
task :test => :lib do 
  Rake::TestTask.new(:test_base) do |t|
    t.libs += ['./ext/debase_internals', './lib']
    t.test_files = FileList[BASE_TEST_FILE_LIST]
    t.verbose = true
  end
end

task :default => :test