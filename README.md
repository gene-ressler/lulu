# Lulu

Lulu - A ruby gem for merging map markers by agglomerative clustering.

## Installation

Add this line to your application's Gemfile:

    gem 'lulu'

And then execute:

    $ bundle

Or install it yourself as:

    $ gem install lulu

## Usage

    require 'lulu'

    # Make an empty marker list
    list = Lulu::MarkerList.new

    # Add some markers.
    10000.times{ |n| list.add(Random.rand(1000), Random.rand(1000), Random.rand(100)) }

    # Get the list length.
    p list.length  # Produces 10000

    # Optionally set merge parameters. Marker calculations can assume circular or
    # square markers, and a scale factor may be set. The scale factor is applied
    # to marker radii so that the marker size exists in a different coordinate
    # space from the inter-marker distances.  This makes sense, for example, if
    # the marker coordinates are pixels on a map. A pixel on the map represents
    # distance, a pixel in a marker something else.
    list.set_info(:circle, 1)

    # Merge the markers.  Pairs that are merged are deleted and replaced by a
    # fresh marker added to the list. If the original list has N, this can
    # result in 2N-1 markers in the result.  Subsequent merges will remove
    # all deleted markers from previous merges.
    list.merge

    # Get the list length again.  If markers were merged, the list grew.
    p list.length  # Produces 10415

    # Get the marker at index 1000.  This is a list [x, y, size].
    # If the index is out of range, nil is returned.
    p list.marker(1000)  # Produces [748.0, 187.0, 80.0]

    # Get whether the marker at index 100 was deleted during a merge.
    p list.marker_deleted(100) # returns true

    # Print the list of all undeleted markers
    p list.markers # Produces [[601.0, 715.0, 38.0]...[647.3314285714285, 179.19619047619048, 1050.0]]

    # Clear the list, returning it to the empty state.
    list.clear

## Ideas

1. It would be good to allow arbitrary data to be associated with markers. The
user might specify callback for merging this data as markers are merged.  For
example, if the markers are a map overlay, roll-over summaries for what each
marker represents could be tracked.
2. The merge algorithm could also turn the marker list into a tree (by giving
each marker two child pointers). Traversing this tree would allow all kinds of
post processing, including the functionality described above.

## Contributing

1. Fork it ( http://github.com/<my-github-username>/lulu/fork )
2. Create your feature branch (`git checkout -b my-new-feature`)
3. Commit your changes (`git commit -am 'Add some feature'`)
4. Push to the branch (`git push origin my-new-feature`)
5. Create new Pull Request
