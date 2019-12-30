ACLOCAL_AMFLAGS=-I m4
plugindir=@rofi_PLUGIN_INSTALL_DIR@

plugin_LTLIBRARIES = blocks.la

extended_la_SOURCES=\
		 src/blocks.c

extended_la_CFLAGS= @glib_CFLAGS@ @rofi_CFLAGS@
extended_la_LIBADD= @glib_LIBS@ @rofi_LIBS@
extended_la_LDFLAGS= -module -avoid-version
