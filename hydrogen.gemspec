# coding: utf-8

Gem::Specification.new do |spec|
  spec.name          = "hydrogen"
  spec.version       = "0.1.0"
  spec.authors       = ["Stephen Boyer"]
  spec.email         = ["boyer.steve@gmail.com"]

  spec.summary       = %q{Default theme for Hydrogen}
  spec.homepage      = "https://github.com/mikotoiii/hydrogen.git"
  spec.license       = "MIT"

  spec.files         = `git ls-files -z`.split("\x0").select { |f| f.match(%r{^(_layouts|_includes|_sass|LICENSE|README)/i}) }

  spec.add_development_dependency "jekyll", "~> 3.2"
  spec.add_development_dependency "bundler", "~> 1.12"
  spec.add_development_dependency "rake", "~> 10.0"
end
