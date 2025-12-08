# https://stackoverflow.com/questions/16780797/incompatible-qt-library-between-app-and-plugin-compiled-same-way/16801698#16801698
add_project_arguments('-DQT_NO_DEBUG', language : 'cpp')

# Use QT_NO_KEYWORDS to avoid clashes with words like 'slots' in other libraries
add_project_arguments('-DQT_NO_KEYWORDS', language : 'cpp')

# Don't show deprecation warnings added after Qt 5.6
add_project_arguments('-DQT_DEPRECATED_WARNINGS_SINCE=QT_VERSION_CHECK(5, 6, 0)', language : 'cpp')

# NODE_TEXT and NODE_DOCUMENT clash with definitions in windows kits/8.1/include/um/msxml.h
add_project_arguments('-DCMARK_NO_SHORT_NAMES', language : 'cpp')

if meson.get_compiler('c').get_argument_syntax() != 'msvc'
  # add_project_arguments('-Wcast-align', language : ['c', 'cpp']) # TODO: enable?
  # add_project_arguments('-Wold-style-cast', language : ['cpp']) # TODO: enable?
  # add_project_arguments('-Wsign-promo', language : ['cpp']) # TODO: enable?
  # add_project_arguments('-Wconversion', language : ['c', 'cpp']) # TODO: enable?

  add_project_arguments('-Wmissing-include-dirs', language : ['c', 'cpp'])
  add_project_arguments('-Wundef', language : ['c', 'cpp'])
  add_project_arguments('-Wwrite-strings', language : ['c', 'cpp'])
  add_project_arguments('-Woverloaded-virtual', language : ['cpp'])
  add_project_arguments('-Wsuggest-override', language : 'cpp')
  add_project_arguments('-Wmissing-declarations', language : ['c', 'cpp'])
  # Note cast-qual is only enabled for C++, where const_cast<> is available
  add_project_arguments('-Wcast-qual', language : ['cpp'])
  add_project_arguments('-Wredundant-decls', language : ['c', 'cpp'])

  # On e.g. Ubuntu 16.04 the compiler is too old to support -Wshadow=local and this will cause an error
  if meson.get_compiler('c').get_id() != 'gcc' or meson.get_compiler('c').version().version_compare('>=7') or meson.get_compiler('c').has_argument('-Wshadow=local')
    add_project_arguments('-Wshadow=local', language : ['c', 'cpp'])
  endif

  # Needed to keep clang from complaining about -Wsuggest-override
  add_project_arguments('-Wno-unknown-warning-option', language : ['c', 'cpp'])
else
  # MSVC
  add_project_arguments('/wd4127', language : ['c', 'cpp']) # Disable "conditional expression is constant"

  # MSVC does not seem to enable /Zc:lambda on older versions when /permissive- is set, even though the documentation <https://learn.microsoft.com/en-us/cpp/build/reference/zc-lambda?view=msvc-140> says it should be: <https://godbolt.org/z/h5K3qq4a4>. Enable /Zc:lambda manually. See e.g. <https://developercommunity.microsoft.com/t/Captured-parameter-not-visible-in-nested/10634637> for more information whye /Zc:lambda is needed.
  add_project_arguments('/Zc:lambda', language : ['cpp'])
  # Full link: <https://godbolt.org/#z:OYLghAFBqd5QCxAYwPYBMCmBRdBLAF1QCcAaPECAMzwBtMA7AQwFtMQByARg9KtQYEAysib0QXACx8BBAKoBnTAAUAHpwAMvAFYTStJg1DIApACYAQuYukl9ZATwDKjdAGFUtAK4sGe1wAyeAyYAHI%2BAEaYxBIapAAOqAqETgwe3r56icmOAkEh4SxRMVxxdpgOqUIETMQE6T5%2BXLaY9rkM1bUE%2BWGR0bG2NXUNmc0KQ93BvUX9pQCUtqhexMjsHABuqHjoANRUXBBzOzsmAOxWGgCCx8doDOOYqvHEO0xeRDvIAPrrYl6YJwAzAARHZcEyAi7XG5vD5ULwMZBA0EmACsVlRwMOJ3OJiuNwJO2ImAIywYnx%2Bf0wEKhBLOwJpeOhx3hiMOjKu9KZTOCBB2LCYwWxZyspwZVw4C1onFRvD8HC0pFQnDc1msOwUSxWAPMgJ4pAImklCwA1rE4tKOJJeCwQKi4vLFcqOLwFCA4oaFZLSHBYDBEChUCx4nRouRKGhg6GYsAFMx4goEKgCHw6ARom6IBEjaQIsFagBPTj6vPMYgFgDyEW0FU9%2BsjbEEFYYtCLXtIWAiXmAbjEtDd3F4WAFRnE7fwxMq60wA8Vjwq7zW%2Bt5rRztDwEWIhY8WGLvAIxDwtsHpGnxAiSUwwMwI%2BA66MRoWVAMsYAanhMAB3CvxRh7mSCCIYjsFIAHyEoag5roZj6IYxhqpY%2Bgbm6kALKg8TtAOAC04zoBCwKmJY1hcKcvCoGeh5YChhwtG0qQuAw7ieI0/iMT0hTFFkSQpAIIxNAk3HtOxfQlLRtZVBMfF6OUlQCJ0dTCTMonjF0UljBMimcVwCyassqwSFKMpyjmzo7KoAAcABsWGWZIOzAMgSIQAeCImkcEC4IQJAnGYepzPuj4LAgmBMFgMQ0WaXAAJwAHSooCGgaGYlmlFFlmWRoqKnKi%2BicNapCOmRnCuu6BqPj6/q%2BlAlVIJGIb0GQFAQHV0YgOsyDxPEPzRV8ZhRV8r5CFwllfKUXyqLZqa0OmxCZtm7aloW/6LeWVY1g4/4NowBDNq2Oadt2va0P2/7DnBY6KhO4nTrOvDzsgi7/iulqKuum7bhgS77oex76meF5KNet73qAXpPi%2BCjvl%2BP5/ie/CAaI4igfD4EqOo7a6ICsEPoRViIW91FoRhqRugA9L%2BxAsHgCjJNOWFKhR2wzvAOmtOJzgQK4amkIEUwcbMAk5Kk3PZDxDCaQLMntPJ9TMaMYmyR0Gl8yJ0mSXL/EqQpKtKQZix6SBhkcLKBUmZwZm2fyCjtTs6zRTFfU7INw0xaUOwefgRAvLq2kBWDQUhWFlCmiACUxZZgKAmYZiZZIkhcICpynJNlr5ba9qm%2B2zolR65VVRAtVBvVYZNS1DVtR1XV2/1UcDUNpy9eNk00NNGaUPNiorW2Jb5qt1a1ptQaNjtLZtpdmBdj2fYDvqZ2jmsl14JOjg3Tm92PSez1rhuW7ljuX0Gj9/7/ZeQPncEoNaODTBvh%2B36/vK%2Boo0BSPSCjiho1BegGDjCE2ATLMlTEwEGTCmVMaZ4DpgzaIlFmaoQVu0BiTEMj8V5gUVWzRRbtBFoJVIEtlJs0VjLbmUsJJdHwWrVSGtKHa3QbrbS%2BttR60tCbQqSpzYTTsiwa2SJq4xSjk7euDt3aeS9j5Pyfsr4B1Cv0CKIA%2BpxXMlZVEkhUQJ0sqicy2Usap2MlnYqthSqeivhVeABdAxRgauGZqRdWrtU6t1fqfU67DVGhoJub80xtyzDmLuy1e6Vn7htE8W0myj32hPQ609To3nOgvIcS9rrM3bOvdMT1BCrnbG9XeBZ97/gPEeY%2B0QAZXliaOC%2B5Vnw30hnfGGj9eDP0RiBN%2BsgP6QQxiAaQP94JEXxshQB6FMKcHJtEcBtNMBYR2KTAAWsgEABgijoCYGRRmVFAGkI5lzahzQ0HTC0oLMWOChZ5B1vsjZSsqEoOkoQ6Wys6FnPVlc9S5DTmzB0lqfSDCWF6KdBwy23CbZ8Mds7SyrsNAiM9t5H2/kyr%2B1IMFGR4UQ6AlikncyXAzCSCipIaOGhARYtylaH5RUXSGNznCy0ZgbR2gdGbUlxjvT5xqiAJYBB4jvGsZgSFlE9CNOAhIFpgg2no0VJjUgn4tzxD3EbVhdKKzvHZXyVAVALZcJ4bbe2wKhqgrdhADwljojiMBDChlcxpFBzkWHQEllTjmTMOZVEqJLJRVOAnZoujSDp1pfo0lbpyUmPzkgVlirrFl2iKEVgaxOFW0BfbPUgiSKuzuty7YvLZAv2aWBYVX9mgSqYFKwcMriXsI4PKtl7wdjKtVTG3hcazAJtOMIvVtiGriN9rCqR8LA6yJDgo21EdnVJwSklcyaVCX5TYdnMlHbGXMuDRy0uLbw2Rs4NGgFtbYpapdqUZNXkeXND5a/LNEERU6E6eKyV0rCWyp9WWxVlaVVrvVUCqKCadXgubQa72vlJAmsCl2xFwdSBmhRTFcyqjzIaGGqcRKZhNHjuLVOv1M6zWEqpZ6mlmdfn0v/WaSDMVJDOpdYCUdeKUWSB0ZwQEiGDGmqNuhr1WGSWSONIS6jTGS0sdQ2eZIzhJBAA%3D%3D%3D>
endif

if target_machine.system() == 'windows'
  add_project_arguments('-DNOMINMAX', language : [ 'cpp' ])

  # TODO: this should not be needed (e.g. M_PI should not be used)
  add_project_arguments('-D_USE_MATH_DEFINES', language : [ 'cpp' ])
endif

# Flags to make MSVC linker use a call to main() instead of WinMain() for win_subsystem : 'windows'
gui_link_flags = []
if meson.get_compiler('c').get_argument_syntax() == 'msvc'
  gui_link_flags += ['/ENTRY:mainCRTStartup']
endif

# TODO: Add these options? /FS /MP # /Ox /GL
# https://docs.microsoft.com/en-us/cpp/build/reference/compiler-options-listed-alphabetically?view=msvc-160
#if meson.get_compiler('c').get_argument_syntax() == 'msvc'
#  add_project_arguments('', language : [ 'cpp' ])
#endif

# Hide internal symbols of plugins (and VoxieClient / Voxie)
if meson.get_compiler('c').get_argument_syntax() != 'msvc'
  add_project_arguments('-fvisibility=hidden', language : [ 'c', 'cpp' ])
endif
# To show symbols exported by plugins:
# ls build/*/*.so | grep -v Voxie/ | grep -v VoxieClient/ | while read a; do echo "=== $a ==="; nm -CD "$a" --defined-only; echo; echo; done
# To check for missing symbols:
# LD_BIND_NOW=1 ./voxie.sh

if false # TODO: addresssanitizer
  # TODO: CONFIG += debug
  # TODO: -O1
  add_project_arguments('-fsanitize=address', '-fno-omit-frame-pointer', '-fno-optimize-sibling-calls', language : [ 'c', 'cpp' ])
  add_project_link_arguments('-fsanitize=address', '-g')
endif

# OpenCL
add_project_arguments('-DCL_USE_DEPRECATED_OPENCL_1_1_APIS', language : 'cpp')
# opencl_incdir = include_directories('../lib/opencl-headers-2.1')
opencl_incdir = include_directories('../lib/OpenCL-Headers')

# cmark
if get_option('main').enabled()
  if get_option('use_cmark_gfm')
    if get_option('cmark_gfm_path') != ''
      cmark_gfm_path = get_option('cmark_gfm_path')
      cmark_gfm = declare_dependency(
        include_directories : include_directories(cmark_gfm_path + '/include'),
        link_args : [
          # '-L' + join_paths(meson.current_source_dir(), cmark_gfm_path) + '/lib',
          # '-lcmark-gfm', '-lcmark-gfm-extensions',
          join_paths(meson.current_source_dir(), cmark_gfm_path) + '/lib/libcmark-gfm.a',
          join_paths(meson.current_source_dir(), cmark_gfm_path) + '/lib/libcmark-gfm-extensions.a',
        ],
      )
    else
      if get_option('use_system_cmark_gfm')
        cmark_gfm = dependency('libcmark-gfm', static : true, allow_fallback : false)
      else
        cmark_gfm_proj = subproject('cmark-gfm', default_options : 'default_library=static')
        cmark_gfm = cmark_gfm_proj.get_variable('cmark_gfm_dep')
        # cmark_license_fn = fs.parent(cmark_gfm_proj.get_variable('cmark_gfm_lib').path()) + '/COPYING'
        additional_collect_licenses_args += [
          '../subprojects/cmark-gfm.sw.json',
        ]
      endif
    endif
    cmark = declare_dependency(dependencies : cmark_gfm, compile_args : ['-DVX_CMARK_USE_GFM=1'])
  else
    if get_option('use_system_cmark')
      cmark_lib = dependency('libcmark', static : true, allow_fallback : false)
      cmark = declare_dependency(dependencies : cmark_lib, compile_args : ['-DVX_CMARK_USE_GFM=0'])
    else
      error('use_system_cmark is set to false')
    endif
  endif
endif

# valgrind
valgrind_incdir = include_directories(['../lib/valgrind'])

# half precision library
half_dep = declare_dependency(include_directories : include_directories('../lib/half-2.1.0/include')).as_system('system') # Suppress -Wundef warnings in half.hpp

# fmt library
fmt_dep = declare_dependency(include_directories : include_directories('../lib/fmt/include'))
additional_collect_licenses_args += files(
  '../lib/fmt.sw.json',
)

qt5 = import('qt5')
if not is_variable('additional_project_incdirs')
  additional_project_incdirs = []
endif
project_incdir = [include_directories('.')] + additional_project_incdirs

# TODO MacOS: On installation get rid of /usr/local/lib/voxie/lib prefix for libVoxieClient.dylib and libVoxie.dylib (or use a prefix of @executable_path/../ / @executable_path/../../ instead). See e.g. https://github.com/mesonbuild/meson/commit/96b7fdb723e5a8d2d7143c7c7a6abc433e0b3da0 and https://github.com/mesonbuild/meson/issues/3077

# General stuff
thread_dep = dependency('threads')
bin_install_rpath = ''
if target_machine.system() == 'darwin'
  bin_install_rpath = '@executable_path/../Frameworks'
endif

if get_option('lib').enabled()
  subdir('lib')
endif
if get_option('libvoxieclient').enabled()
  subdir('VoxieClient')
endif
if get_option('libvoxiebackend').enabled()
  subdir('VoxieBackend')
endif
if get_option('libvoxie').enabled()
  subdir('Voxie')
endif
if get_option('main').enabled()
  subdir('Main')
endif

if get_option('plugins').enabled()
  # Plugin definitions
  # This is a hack to make sure that the MOC uses the correct include directory (project_incdir), https://github.com/mesonbuild/meson/pull/8139/ should fix this problem
  if not is_variable('additional_project_inc_compile_args')
    additional_project_inc_compile_args = []
  endif
  project_inc_dep = declare_dependency(
    compile_args : [
      # '-I' + project_incdir,
      '-I' + meson.current_source_dir(),
    ] + additional_project_inc_compile_args,
  )
  plugin_dependencies = [
    libvoxie_dep,
    project_inc_dep,
  ]
  plugin_install_dir = voxie_install_dir + '/plugins'
endif

if get_option('ext').enabled()
  # Extension definitions
  ext_install_rpath = ''
  if target_machine.system() == 'darwin'
    ext_install_rpath = '@executable_path/../../Frameworks'
  endif
  qt_modules = [
    'Core',
    'DBus',
  ]
  # Note: moc does not like "include_type:'system'" because meson only considers -I and -D for moc, not -isystem: <https://github.com/mesonbuild/meson/blob/398df5629863e913fa603cbf02c525a9f501f8a8/mesonbuild/modules/qt.py#L193>. See also <https://github.com/mesonbuild/meson/pull/8139>. Note that currently voxie should compile even without this workaround, see update-dbus-proxies.py.
  ext_qt5_dep = dependency('qt5', modules : qt_modules, include_type : 'system')
  ext_qt5_dep_moc = dependency('qt5', modules : qt_modules)
  qt_modules = [
    'Core',
    'DBus',
    'Gui',
  ]
  ext_qt5_dep_gui = dependency('qt5', modules : qt_modules, include_type : 'system')
  ext_qt5_dep_gui_moc = dependency('qt5', modules : qt_modules)
  qt_modules = [
    'Core',
    'DBus',
    'Gui',
    'Widgets',
  ]
  ext_qt5_dep_widgets = dependency('qt5', modules : qt_modules, include_type : 'system')
  ext_qt5_dep_widgets_moc = dependency('qt5', modules : qt_modules)
  ext_dependencies = [
    voxieclient_dep,
  ]
  ext_install_dir = voxie_install_dir + '/ext'
  filter_install_dir = voxie_install_dir + '/filters'
  
  # Script definitions
  script_install_dir = voxie_install_dir + '/scripts'
endif

if get_option('plugins').enabled()
  subdir('PluginFilter')
  subdir('PluginFile')
  subdir('PluginVisSlice')
  subdir('PluginVisRaw')
  subdir('PluginVis3D')
  subdir('PluginVisTable')
  subdir('PluginVisLinePlot')
  subdir('PluginFilter2D')
  subdir('PluginExample')
  subdir('PluginSegmentation')
  if get_option('libjpeg').enabled() or get_option('libjpeg12').enabled()
    subdir('PluginJpegLibjpeg')
  endif
endif

if get_option('ext').enabled()
  subdir('ScriptGetAverage')
  
  subdir('ExtDataT3R')
  subdir('ExtFileVgi')
  subdir('ExtFilePly')
  subdir('ExtFilePeridynamics')
  subdir('ExtFilterAddNormalAttribute')
  subdir('ExtFileVxEvl')
  subdir('ExtFilterEventListClustering')
  subdir('ExtFilterExtractKeyPoints')
  subdir('ExtFilterIterativeClosestPoint')
  subdir('ExtFilterEventListProjection')
  subdir('ExtFilterModifySurface')
  subdir('ExtFilterMultiMaterialMarchingCubes')
  subdir('ExtFilterMultiMaterialVolumeSegmentation')
  subdir('ExtFilterLocalThickness')
  subdir('ExtFilterVolumeRegistration')
  subdir('ExtFilterHausdorffDistance')
  subdir('ExtFilterGradientMaximalSurface')
  subdir('ExtFilterMarchingCubes33')
  subdir('ExtFilterMarchingCubes33_by_DVega')
  subdir('ExtFilterRotateSurface')
  subdir('ExtFilterVerifySurface')
  subdir('ExtFilterFloodFill')
  subdir('ExtSegmentationStepWatershed')
  if get_option('hdf5').enabled()
    subdir('ExtFileHdf5')
  endif
endif

if get_option('test').enabled()
  subdir('Test')
endif

if get_option('plugins').enabled()
  if get_option('lapack').enabled()
    subdir('PluginFitting')
  endif
  if get_option('help_browser').enabled()
    subdir('PluginHelpBrowser')
  endif
endif

# Note: extra has to be last so that all additional_collect_licenses_args settings have been made
if get_option('extra').enabled()
  subdir('extra')
endif
