require 'mkmf'

# Turn off warnings about declarations mixed with code.
$CFLAGS += ' -std=c99 -Wno-declaration-after-statement'

create_makefile('lulu/lulu')