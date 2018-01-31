require 'bundler/gem_tasks'
require 'rake/testtask'

BASE_TEST_FILE_LIST = Dir['test/**/test_*.rb']

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
  cd "ext/attach" do
    if File.exists?("Makefile")
      sh "make clean"
      rm  "Makefile"
    end
    derived_files = Dir.glob(".o") + Dir.glob("*.so") + Dir.glob("*.bundle")
    rm derived_files unless derived_files.empty?
  end
  if File.exists?('pkg')
    cd 'pkg' do
      derived_files = Dir.glob('*.gem')
      rm derived_files unless derived_files.empty?
    end
  end
end

desc "Create the core debase shared library extension"
task :lib => :clean do
  Dir.chdir("ext") do
    system("#{Gem.ruby} extconf.rb && make")
    exit $?.exitstatus if $?.exitstatus != 0
  end
  Dir.chdir("ext/attach") do
    system("#{Gem.ruby} extconf.rb && make")
    exit $?.exitstatus if $?.exitstatus != 0
  end
end

desc "Test debase."
Rake::TestTask.new(:test) do |t|
    t.libs += ['./ext', './lib']
    t.test_files = FileList[BASE_TEST_FILE_LIST]
    t.verbose = true
  end
task :test => :lib

task :default => :test
