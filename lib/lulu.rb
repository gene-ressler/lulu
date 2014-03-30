require "lulu/version"
require "lulu/lulu"

module Lulu
  class MarkerList

    def markers
      list = []
      length.times do |i|
        m = marker(i)
        list << m if m
      end
      list
    end

  end
end
