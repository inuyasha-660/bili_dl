project('bili_dl', 'c')

deps = [dependency('libcjson'), dependency('libcurl')]
depsffmpeg = [dependency('libavcodec'), dependency('libavformat'), dependency('libavutil')]

src = files(
  'main.c',
  'utils/configu.c',
  'utils/wbi.c',
  'utils/utils.c',
  'api/video/video.c',
  'api/video/configv.c',
  'api/video/merge.c',
  'api/folder/folder.c',
  'api/folder/configf.c',
  'libs/thpool.c',
  'libs/md5.c'
  )

executable('bili_dl', src, dependencies: [deps, depsffmpeg])
