project('zeppelin', 'c', default_options: ['warning_level=3'])

sources = files(
  'src/main.c',
  'src/dump.c',
  'src/eval.c',
  'src/game.c',
  'src/movegen.c',
  'src/perft.c',
  'src/precomp.c',
  'src/search.c',
  'src/time.c',
  'src/uci.c',
  'src/zobrist.c'
)

if get_option('buildtype') == 'debug'
  add_project_arguments('-ggdb2', language: 'c')
  add_project_arguments('-pg', language: 'c')
  add_project_arguments('-DDEBUG_INTERFACE', language: 'c')
  sources += files('src/debug.c')
elif get_option('buildtype') == 'release'
  add_project_arguments('-O3', language: 'c')
endif

executable('zeppelin', sources)

if get_option('buildtype') == 'debug'
  python = find_program('python3', 'python')

  test_path = meson.project_source_root() + '/test/run_tests.py'
  exe_path = meson.project_build_root() + '/zeppelin'

  test('unittests', python, args: [test_path, exe_path])
endif
