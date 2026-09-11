project('themestylebrowser', 'cpp',
  version: run_command(
        'sh',
        '-c', '''
        v=$(
            command -v zfr >/dev/null 2>&1 && zfr version 2>/dev/null || true
        )
        if [ -z "$v" ] && [ -f VERSION ]; then
            v=$(head -n1 VERSION | tr -d '\r')
            v=${v#v}
        fi
        if [ -z "$v" ]; then
            v="0.0.0" # FIXED TO 0.0.0, DO NOT MODIFY
        fi
        printf '%s' "$v"
        ''',
        check: false,
    ).stdout().strip(),
  license : 'AGPL-3.0-or-later',
  default_options : ['warning_level=0', 'cpp_std=c++17'])
project_author = 'Lenik'
project_email = 'themestylebrowser@bodz.net'
project_year = 2026

asciidoctor = find_program('asciidoctor', required: true)
fs = import('fs')

prefix = get_option('prefix')
bindir = prefix / get_option('bindir')
datadir = prefix / get_option('datadir')
mandir = prefix / get_option('mandir')
localedir = prefix / get_option('localedir')
pkgdatadir = datadir / meson.project_name()
pkgdocdir = datadir / 'doc' / meson.project_name()

config_h = configuration_data()
config_h.set_quoted('PROJECT_VERSION', meson.project_version())
config_h.set_quoted('PROJECT_AUTHOR', project_author)
config_h.set_quoted('PROJECT_EMAIL', project_email)
config_h.set('PROJECT_YEAR', project_year)
config_h.set_quoted('LOCALEDIR', localedir)
configure_file(output: 'config.h', configuration: config_h)

cpp = meson.get_compiler('cpp')

# libbas
bas_c_dep = dependency('bas-c', required: true)
bas_cpp_dep = dependency('bas-cpp', required: true)
bas_ui_dep = dependency('bas-ui', required: true)
bas_args = [
  '-DLOGGER_NAME=' + meson.project_name(),
  '-DTEXT_DOMAIN=' + meson.project_name(),
]

# wxWidgets: one GTK toolkit only (avoid mixing GTK2 + GTK3 in-process).
# Detect ABI from the active wx-config so both 3.0 and 3.2+ build.
wx_dep = dependency('wxwidgets', method: 'config-tool', required: true)
wx_config = find_program('wx-config', required: true)
wx_libs_out = run_command(wx_config, '--libs', check: true, capture: true).stdout()
wx_ver = run_command(wx_config, '--release', check: true, capture: true).stdout().strip()
if wx_libs_out.contains('gtk3')
  wx_gtk_prefix = 'wx_gtk3u'
elif wx_libs_out.contains('gtk2')
  wx_gtk_prefix = 'wx_gtk2u'
else
  error('Unsupported wxWidgets toolkit (need gtk2 or gtk3): ' + wx_libs_out)
endif

# Optional extras (same toolkit/ABI). Not always packaged for older GTK2 builds.
wx_media_lib = cpp.find_library(wx_gtk_prefix + '_media-' + wx_ver, required: false)
wx_richtext_lib = cpp.find_library(wx_gtk_prefix + '_richtext-' + wx_ver, required: false)
wx_stc_lib = cpp.find_library(wx_gtk_prefix + '_stc-' + wx_ver, required: false)
wx_webview_lib = cpp.find_library(wx_gtk_prefix + '_webview-' + wx_ver, required: false)

wx_defines = []
if wx_dep.version().version_compare('< 3.1')
  # wx 3.0: media/webview are separate; headers often ship with wxUSE_*=0.
  wx_defines = ['-DwxUSE_MEDIACTRL=1', '-DwxUSE_WEBVIEW=1']
  add_project_arguments(wx_defines, language: 'cpp')
endif

# Disable all warnings (wx 3.0 and deps are noisy)
cpp_nowarn = cpp.get_supported_arguments(['-w', '/w'])

cpp_sources = files(
  'src/main.cpp',
  'src/tsb/LibraryModel.cpp',
  'src/tsb/TSBFrame.cpp',
  'src/tsb/ImageProperties.cpp',
  'src/tsb/ItemProperties.cpp',
)

assets_dir = meson.current_source_dir() / 'assets'
assets_zip = custom_target(
  'assets_zip',
  output: 'assets.zip',
  input: [],
  command: [
    'sh', '-c',
    'dst=$(readlink -f "@1@"); cd "@0@" && zip -r "$dst" .'.format(assets_dir, '@OUTPUT@'),
  ],
  build_by_default: true,
  install: false,
)

assets_s = configure_file(
  input: 'app/assets.s',
  output: 'assets.s',
  copy: true,
)

as = find_program('as', required: true)
assets_o = custom_target(
  'assets_o',
  output: 'assets.o',
  input: [assets_s, assets_zip],
  command: [as, '-o', '@OUTPUT@', '@INPUT0@'],
  depends: assets_zip,
)

exe_deps = [
  bas_c_dep,
  bas_cpp_dep,
  bas_ui_dep,
  wx_dep,
]

# wx 3.0 often needs extras linked separately when enabled above.
if wx_dep.version().version_compare('< 3.1')
  foreach lib : [wx_media_lib, wx_richtext_lib, wx_stc_lib, wx_webview_lib]
    if lib.found()
      exe_deps += [lib]
    endif
  endforeach
endif

exe_inc = [
  include_directories('src'),
  include_directories('.'),
]

executable(
  'themestylebrowser',
  cpp_sources,
  'src/module.cpp',
  assets_o,
  dependencies: exe_deps,
  include_directories: exe_inc,
  cpp_args: cpp_nowarn + bas_args + wx_defines,
  install: true,
  install_dir: bindir,
)

run_target(
  'look',
  command: [
    'bash',
    '-euc', '''
            tmpdir=$(mktemp -d)
            DESTDIR=$tmpdir meson install -C "@BUILD_ROOT@"
            tree "$tmpdir"
            rm -fr "$tmpdir"
        ''',
  ],
)

install_data(
  [
    'LICENSE',
    'README.md',
    'README-zh_CN.md',
  ],
  install_dir: pkgdocdir,
)



bash_files = [
    'completions/themestylebrowser.bash',
]
foreach file : bash_files
  name = fs.stem(file)
  install_data(
    file,
    install_dir: datadir / 'bash-completion' / 'completions',
    rename: name,
  )
endforeach

subdir('po')

man_puffs = [
    'themestylebrowser',
]
man_i18n_langs = []
foreach lang : ['fr', 'it', 'de', 'es_MX', 'zh_CN', 'ja', 'ko', 'ar']
  if fs.is_dir('docs' / lang)
    man_i18n_langs += [lang]
  endif
endforeach

_install_man_i18n = find_program('sh')
foreach puff : man_puffs
  custom_target(
    puff + '-man',
    input: 'docs' / (puff + '.adoc'),
    output: puff + '.1',
    command: [
      asciidoctor,
      '-b', 'manpage',
      '-a', 'project-version=' + meson.project_version(),
      '-a', 'project-year=@0@'.format(project_year),
      '-a', 'project-author=' + project_author,
      '-a', 'project-email=' + project_email,
      '-o', '@OUTPUT@',
      '@INPUT@',
    ],
    build_by_default: true,
    install: true,
    install_dir: mandir / 'man1',
  )
  foreach lang : man_i18n_langs
    _man_i18n = custom_target(
      puff + '-' + lang + '-man',
      input: 'docs' / lang / (puff + '.adoc'),
      output: puff + '--' + lang + '.1',
      command: [
        asciidoctor,
        '-b', 'manpage',
        '-a', 'project-version=' + meson.project_version(),
        '-a', 'project-year=@0@'.format(project_year),
        '-a', 'project-author=' + project_author,
        '-a', 'project-email=' + project_email,
        '-o', '@OUTPUT@',
        '@INPUT@',
      ],
      build_by_default: true,
    )
    meson.add_install_script(
      _install_man_i18n,
      '-c',
      'base="${MESON_INSTALL_DESTDIR_PREFIX:-${DESTDIR:-}${MESON_INSTALL_PREFIX:-}}"; install -Dm644 "$0" "$base/$1"',
      _man_i18n,
      get_option('mandir') / lang / 'man1' / (puff + '.1'),
    )
  endforeach
endforeach
