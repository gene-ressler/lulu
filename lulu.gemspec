# coding: utf-8
lib = File.expand_path('../lib', __FILE__)
$LOAD_PATH.unshift(lib) unless $LOAD_PATH.include?(lib)
require 'lulu/version'

Gem::Specification.new do |spec|
  spec.name          = "lulu"
  spec.version       = Lulu::VERSION
  spec.authors       = ["Gene Ressler"]
  spec.email         = ["gene.ressler@gmail.com"]
  spec.summary       = %q{Merge map markers by agglomerative clustering.}
  spec.description   =
%q{Lulu merges the closest overlapping pair of map markers into a one whose
area is the sum of the original two, located at their centroid. It repeats
this until no overlaps remain. Some interesting data structures make this
quite fast for all but pathologically bad data. Markers can be circles or
squares. The user may provide a scale factor allowing the marker radius to
have a different scale than distance between markers.}
  spec.homepage      = "https://github.com/gene-ressler/lulu/wiki"
  spec.licenses      = "GPL-3.0"

  spec.files         = `git ls-files`.split($/)
  spec.extensions  = ['ext/lulu/extconf.rb']
  spec.executables   = spec.files.grep(%r{^bin/}) { |f| File.basename(f) }
  spec.test_files    = spec.files.grep(%r{^(test|spec|features)/})
  spec.require_paths = ["lib"]

  spec.add_development_dependency "bundler", "~> 1.5"
  spec.add_development_dependency "rake", "~> 10.1"
  spec.add_development_dependency "rake-compiler"
  spec.add_development_dependency "rspec", "~> 2.14"
end
