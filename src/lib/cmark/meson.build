# This is a hack to compile cmark without needing cmake

srcdir = '../../../lib/cmark/src'

additional_collect_licenses_args += files(
  '../../../lib/cmark.sw.json',
)

cmark_args = []
if meson.get_compiler('c').get_argument_syntax() != 'msvc'
  cmark_args += [ '-Wno-undef', '-Wno-missing-declarations' ]
endif

cmark = static_library(
  'cmark',
  [
    srcdir + '/blocks.c',
    srcdir + '/buffer.c',
    srcdir + '/cmark.c',
    srcdir + '/cmark_ctype.c',
    srcdir + '/commonmark.c',
    srcdir + '/houdini_href_e.c',
    srcdir + '/houdini_html_e.c',
    srcdir + '/houdini_html_u.c',
    srcdir + '/html.c',
    srcdir + '/inlines.c',
    srcdir + '/iterator.c',
    srcdir + '/latex.c',
    srcdir + '/main.c',
    srcdir + '/man.c',
    srcdir + '/node.c',
    srcdir + '/references.c',
    srcdir + '/render.c',
    srcdir + '/scanners.c',
    srcdir + '/utf8.c',
    srcdir + '/xml.c',
  ],
  # implicit_include_directories : false,
  c_args : [ '-DCMARK_STATIC_DEFINE', cmark_args ],
  cpp_args : [ '-DCMARK_STATIC_DEFINE', cmark_args ],
)
