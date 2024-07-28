project('rofi-blocks', 'c', version : '0.1.0')

prefix = get_option('prefix')
libdir = get_option('libdir')
ui_test_enabled = get_option('ui_test')

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

test_sources = [
  'src/page_data.c',
  'src/json_glib_extensions.c',
  'src/string_utils.c',
]

test_unit_sources = [ 'tests/unit/entrypoint.test.c' ] + test_sources

test_env = [
  'LC_ALL=C',
]

shared_library(
  'blocks', 
  main_sources,
  dependencies: deps,
  install_dir:pluginsdir, 
  name_prefix: '',
  install: true,
  link_args: ['-Wl,--unresolved-symbols=ignore-all']
)

test(
  'test-suite-unit',
  executable('rofi-blocks-unit', test_unit_sources, dependencies: deps),
  env: test_env,
  protocol: 'tap',
  workdir: join_paths(meson.current_source_dir(), 'tests')
)

if ui_test_enabled 
  ui_test = join_paths(meson.source_root(), 'tests/ui/test.sh')
  test(
    'test-suite-ui',
    find_program(ui_test),
    env: test_env,
    protocol: 'tap',
    workdir: join_paths(meson.current_source_dir(), 'tests')
  )
endif