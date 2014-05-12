require 'lulu'

TEST_SIZE = 10000

def new_marker_list(size = TEST_SIZE)
  list = Lulu::MarkerList.new
  srand(42)
  size.times{ list.add(Random.rand(1000), Random.rand(1000), Random.rand(100)) }
  list
end
