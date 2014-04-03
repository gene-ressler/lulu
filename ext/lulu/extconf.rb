require 'mkmf'

# Turn off warnings about declarations mixed with code.
$CFLAGS += ' -std=c99 -Wno-declaration-after-statement'

# Select Ruby gem code
$CFLAGS += ' -DLULU_GEM'

create_makefile('lulu/lulu')