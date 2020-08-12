project('xprintidle',
  'c',
  version : '0.2.3-dev',
  license : 'GPL-2.0',
  default_options : [
    'c_std=c89',
    'warning_level=3',
    'optimization=3',
  ]
)

src = [
  'xprintidle.c',
]

dep = [
  dependency('xscrnsaver'),
  dependency('x11'),
  dependency('xext'),
]

executable('xprintidle',
  sources: src,
  dependencies: dep,
  install : true,
)
