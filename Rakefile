require "bundler/gem_tasks"
require "rspec/core/rake_task"
require 'rake/extensiontask'

RSpec::Core::RakeTask.new
Rake::ExtensionTask.new('lulu')

task :default => :spec
task :test => :spec
