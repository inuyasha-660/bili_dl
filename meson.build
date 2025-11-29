project('bili_dl', 'c')

deps = [dependency('libcjson'), dependency('libcurl')]
src = files(
  'main.c',
  'config.c',
  'api.c',
  'thpool.c'
  )

executable('bili_dl', src, dependencies: deps)
