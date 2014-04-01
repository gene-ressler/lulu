require 'spec_helper'

# Test the Lulu API.
describe Lulu do

  it 'should report version' do
    Lulu::VERSION.should_not be_nil
  end

  it 'should report extension code version' do
    Lulu::EXT_VERSION.should_not be_nil
  end

end
