project('bili_dl', 'c')

deps = [dependency('libcjson'), dependency('libcurl')]
depsffmpeg = [dependency('libavcodec'), dependency('libavformat'), dependency('libavutil')]

src = files(
  'main.c',
  'config.c',
  'api/video.c',
  'api/merge.c',
  'libs/thpool.c'
  )

executable('bili_dl', src, dependencies: [deps, depsffmpeg])
