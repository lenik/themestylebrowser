project('themestylebrowser', 'cpp',
  version : '1.0.0',
  license : 'AGPL-3.0-or-later',
  default_options : ['warning_level=0', 'cpp_std=c++17'])

pkgdatadir = get_option('datadir') / meson.project_name()

cpp = meson.get_compiler('cpp')

# libbas
    bas_c_dep = dependency('bas-c', required : true)
    bas_cpp_dep = dependency('bas-cpp', required : true)
    bas_ui_dep = dependency('bas-ui', required : true)

# wxWidgets dependencies (must use a single GTK toolkit to avoid "GTK+ 2.x and GTK+ 3 in the same process" error)
    wx_dep = dependency('wxwidgets', method: 'config-tool', required: true)
    # Detect toolkit from default wx-config so we link only gtk2 or only gtk3
    wx_config = find_program('wx-config', required: true)
    wx_libs_out = run_command(wx_config, '--libs', check: true, capture: true).stdout()
    wx_use_gtk3 = wx_libs_out.contains('gtk3')
    if wx_use_gtk3
        wx_gtk_prefix = 'wx_gtk3u'
        wx_ver = '3.0'
    else
        wx_gtk_prefix = 'wx_gtk2u'
        wx_ver = '3.1'
    endif
    # wxWidgets media (wxMediaCtrl) and extras - use same toolkit as core
    # Media/webview optional on GTK2 (often not built for gtk2)
    wx_media_lib = cpp.find_library(wx_gtk_prefix + '_media-' + wx_ver, required: false)
    wx_richtext_lib = cpp.find_library(wx_gtk_prefix + '_richtext-' + wx_ver, required: true)
    wx_stc_lib = cpp.find_library(wx_gtk_prefix + '_stc-' + wx_ver, required: true)
    wx_webview_lib = cpp.find_library(wx_gtk_prefix + '_webview-' + wx_ver, required: false)

# wx_dep = dependency('wxwidgets', modules : [
#     'core',
#     'media',
#     'richtext',
#     'stc',
#     'webview',
#     ], required : true)

    if wx_dep.version() < '3.1'
        # wx 3.0: media and webview are in separate libs; core setup has wxUSE_*=0, so enable for our build
        wx_defines = ['-DwxUSE_MEDIACTRL=1', '-DwxUSE_WEBVIEW=1']
        add_project_arguments(wx_defines, language: 'cpp')
    endif

# Disable all warnings (wx 3.0 and deps are noisy)
cpp_nowarn = cpp.get_supported_arguments(['-w', '/w'])
cpp_nowarn = cpp_nowarn + wx_defines

# Source files - reorganized structure
cpp_sources = files(
  'src/main.cpp',
  'src/tsb/LibraryModel.cpp',
  'src/tsb/TSBFrame.cpp',
  'src/tsb/ImageProperties.cpp',
  'src/tsb/ItemProperties.cpp',
)

# Assets will be zipped and linked as a section in the executable
all_sources = cpp_sources

# Build assets.zip every time (no file dependency)
assets_dir = meson.current_source_dir() / 'assets'
build_dir = meson.current_build_dir()
assets_zip = custom_target(
    'assets_zip',
    output : 'assets.zip',
    input : [],
    command : [
        'sh', '-c',
        'dst=$(readlink -f "@1@"); cd "@0@" && zip -r "$dst" .'.format(assets_dir, '@OUTPUT@')
    ],
    #build_always : true,
    build_by_default : true,
    install : false,
)

# Compile assembly file that embeds assets.zip using .incbin
# Use configure_file to copy assets.s to build directory
assets_s = configure_file(
    input : 'app/assets.s',
    output : 'assets.s',
    copy : true,
)

# Compile the assembly file (assets.zip will be in the same directory)
as = find_program('as', required : true)
assets_o = custom_target(
    'assets_o',
    output : 'assets.o',
    input : [assets_s, assets_zip],
    command : [
        as,
        '-o', '@OUTPUT@',
        '@INPUT0@'
    ],
    depends : assets_zip,
)

exe_deps = [
    bas_c_dep,
    bas_cpp_dep,
    bas_ui_dep,
    wx_dep,
]

# if wx_dep.version() < '3.1'
#     exe_deps += [
#         wx_media_lib,
#         wx_richtext_lib,
#         wx_stc_lib,
#         wx_webview_lib,
#     ]
# endif

exe_inc = [
    include_directories('src'),
    include_directories('.'),
]

executable('themestylebrowser',
    cpp_sources,
    'src/module.cpp',
    assets_o,
    dependencies : exe_deps,
    include_directories : exe_inc,
    cpp_args : cpp_nowarn + ['-DLOGGER_NAME=tsb'],
    install : true,
)
