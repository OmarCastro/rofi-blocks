ACLOCAL_AMFLAGS=-I m4
plugindir=@rofi_PLUGIN_INSTALL_DIR@

plugin_LTLIBRARIES = blocks.la

blocks_la_SOURCES=\
		 src/blocks.c \
		 src/string_utils.c \
		 src/json_glib_extensions.c \
		 src/page_data.c \
		 src/render_state.c

blocks_la_CFLAGS= @glib_CFLAGS@ @rofi_CFLAGS@
blocks_la_LIBADD= @glib_LIBS@ @rofi_LIBS@
blocks_la_LDFLAGS= -module -avoid-version
