require "mkmf"
have_header("stdint.h")
have_header("endian.h")
have_header("libkern/OSByteOrder.h")
create_makefile("swf_embed")
