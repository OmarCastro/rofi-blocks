project('rofi-blocks', 'c', version : '0.1.0')

prefix = get_option('prefix')
libdir = get_option('libdir')

libexecdir = join_paths(prefix, get_option('libexecdir'))
sysconfdir = join_paths(prefix, get_option('sysconfdir'))
autostartdir = join_paths(sysconfdir, 'xdg', 'autostart')

rofiDependency = dependency('rofi')
pluginsdir = rofiDependency.get_variable('pluginsdir')


compiler = meson.get_compiler('c')
deps = [
  dependency('glib-2.0'),
  dependency('gmodule-2.0'),
  dependency('json-glib-1.0'),
  dependency('cairo'),
  dependency('rofi'),
]


main_sources = [
  'src/blocks.c',
  'src/blocks_mode_data.c',
  'src/json_glib_extensions.c',
  'src/page_data.c',
  'src/render_state.c',
  'src/string_utils.c',
]


shared_library(
  'blocks', 
  main_sources,
  dependencies: deps,
  install_dir:pluginsdir, 
  name_prefix: '',
  link_args: ['-Wl,--unresolved-symbols=ignore-all']
)



