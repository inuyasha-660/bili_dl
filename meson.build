project('bili_dl', 'c')

deps = [dependency('libcjson'), dependency('libcurl')]
src = files(
  'main.c',
  'config.c',
  'api/video.c',
  'libs/thpool.c'
  )

executable('bili_dl', src, dependencies: deps)
