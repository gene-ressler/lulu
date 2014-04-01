require 'spec_helper'

# Test the Lulu API.
describe Lulu::MarkerList do

  TEST_SIZE = 10000

  let(:list) do
    list = Lulu::MarkerList.new
    srand(42)
    TEST_SIZE.times{ |n| list.add(Random.rand(1000), Random.rand(1000), Random.rand(100)) }
    list
  end

  it 'should have length matching number of markers added' do
    list.length.should == TEST_SIZE
  end

  it 'should merge to correct number of markers' do
    list.merge.should == 17362
  end

  it 'should compress to correct number of markers after merge' do
    list.merge
    list.compress.should == 2638
  end

  it 'should have idempotent merge' do
    list.merge
    list.compress == list.merge
  end

  it 'should have correct number of non-merged nodes in parts' do
    n = 0
    list.merge.times{|i| n += 1 if [:root, :single].include? list.parts(i)[0] }
    n.should == list.compress
  end

end
