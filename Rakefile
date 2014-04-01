require "bundler/gem_tasks"
require "rspec/core/rake_task"
require 'rake/extensiontask'

RSpec::Core::RakeTask.new

# Rake compiler's standard doesn't match the Ruby Guides
# http://guides.rubygems.org/gems-with-extensions/. So we need this.
Rake::ExtensionTask.new do |ext|
  ext.name = 'lulu'
  ext.ext_dir = 'ext/lulu'
  ext.lib_dir = 'lib/lulu'
end

task :default => :spec
task :test => :spec
