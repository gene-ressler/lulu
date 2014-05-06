require 'spec_helper'

# Test the Lulu API.
describe Lulu::MarkerList do

  let(:list) { new_marker_list }

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

  it 'should perform fine over multiple runs with unit increases in input length to provoke memory bugs' do
    1000.times { |i| new_marker_list(10000 + i).merge }
  end

end
