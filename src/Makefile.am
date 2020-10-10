#######################################
# The list of libraries we are building seperated by spaces.
# The 'lib_' indicates that these build products will be installed
# in the $(libdir) directory. For example /usr/lib
lib_LTLIBRARIES = libjsonrpcc.la

#######################################
# Build information for each library

# Sources for jsonrpcc
libjsonrpcc_la_SOURCES = cJSON.c jsonrpc-c.c

# Linker options libTestProgram
libjsonrpcc_la_LDFLAGS = -lm

# Compiler options. Here we are adding the include directory
# to be searched for headers included in the source code.
libjsonrpcc_la_CPPFLAGS = $(LIBEV_CFLAGS) -I$(top_srcdir)/include

# libev
libjsonrpcc_la_LIBADD = $(LIBEV_LIBS)

