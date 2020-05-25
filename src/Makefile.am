plugindir=@rofi_PLUGIN_INSTALL_DIR@

plugin_LTLIBRARIES = blocks.la

blocks_la_SOURCES=\
		blocks.c \
		blocks_mode_data.c \
		string_utils.c \
		json_glib_extensions.c \
		page_data.c \
		render_state.c

blocks_la_CFLAGS= @glib_CFLAGS@ @rofi_CFLAGS@
blocks_la_LIBADD= @glib_LIBS@ @rofi_LIBS@
blocks_la_LDFLAGS= -module -avoid-version

