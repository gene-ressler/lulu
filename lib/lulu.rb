require "lulu/version"
require "lulu/lulu"

module Lulu
  class MarkerList

    def markers
      list = []
      length.times {|i| list << marker(i) unless deleted(i) }
      list
    end

  end
end
