project('bili_dl', 'c')

meta_data = configuration_data()
meta_data.set('version', '0.2.0')
configure_file(input: 'metadata.h.in', output: 'metadata.h', configuration: meta_data)

deps = [dependency('libcjson'), dependency('libcurl')]
depsffmpeg = [dependency('libavcodec'), dependency('libavformat'), dependency('libavutil')]

src = files(
  'main.c',
  'utils/configu.c',
  'utils/wbi.c',
  'utils/log.c',
  'utils/utils.c',
  'api/video/video.c',
  'api/video/configv.c',
  'api/video/merge.c',
  'api/folder/folder.c',
  'api/folder/configf.c',
  'api/anime/anime.c',
  'api/anime/configa.c',
  'libs/thpool.c',
  'libs/md5.c'
  )

executable('bili_dl', src, dependencies: [deps, depsffmpeg])
