project('cjs', 'cpp', 'c', version: '5.2.0', license: ['MIT', 'LGPL2+'],
    meson_version: '>= 0.49.2',
    default_options: ['cpp_std=c++17', 'c_std=c99', 'warning_level=2'])

api_version = '1.0'
api_name = '@0@-@1@'.format(meson.project_name(), api_version)

gnome = import('gnome')
pkg = import('pkgconfig')

top_include = include_directories('.')

### Check for conflicting build options ########################################

if get_option('systemtap') and not get_option('dtrace')
    error('-Ddtrace=true is required for -Dsystemtap=true')
endif

if get_option('buildtype').startswith('release') and get_option('verbose_logs')
    error('-Dverbose_logs=true is not allowed with --buildtype=release')
endif

### Check for compiler args ####################################################

cxx = meson.get_compiler('cpp')
cc = meson.get_compiler('c')

if cc.get_id() == 'msvc'
    add_project_arguments(cxx.get_supported_arguments([
        '-utf-8',  # Use UTF-8 mode
        '/Zc:externConstexpr',  # Required for 'extern constexpr' on MSVC

        # Ignore spurious compiler warnings for things that GLib and SpiderMonkey
        # header files commonly do
        '-FImsvc_recommended_pragmas.h',
        '-EHsc',
        '-D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS', # Don't worry about the C++17 deprecations
        '-wd4099',
        '-wd4251',
        '-wd4291',
        '-wd4800',
    ]), language: ['cpp', 'c'])
else
    # Ignore spurious compiler warnings for things that GLib and SpiderMonkey
    # header files commonly do	
    add_project_arguments(cxx.get_supported_arguments([
        '-fno-strict-aliasing',
        '-Wno-variadic-macros',  # GLib uses these in header files
        '-Wno-missing-field-initializers',  # SpiderMonkey JSClass, among others
    ]), language: 'cpp')

    add_project_arguments(cc.get_supported_arguments([
        '-Wno-typedef-redefinition',  # GLib does this in header files
    ]), language: 'c')
endif

if cc.get_argument_syntax() == 'msvc'
    add_project_arguments(cxx.get_supported_arguments([
        '-Dssize_t=gssize',  # Windows SDK/MSVC headers do not come with ssize_t
        '-DNOMINMAX',  # We don't want 'min' or 'max' to interfere
        ]), language: ['cpp', 'c'])
else
    if get_option('bsymbolic_functions')
        if not cxx.has_link_argument('-Bsymbolic-functions')
            error('''-Bsymbolic-functions not supported, configure with
-Dbsymbolic_functions=false''')
        endif
        add_project_link_arguments('-Bsymbolic-functions', language: ['cpp', 'c'])
        if cc.has_argument('-fno-semantic-interposition')
            add_project_arguments('-fno-semantic-interposition', language: 'c')
        endif
        if cxx.has_argument('-fno-semantic-interposition')
            add_project_arguments('-fno-semantic-interposition', language: 'cpp')
        endif
    endif
endif

# SpiderMonkey can be compiled with or without runtime type information, we must
# match that option because we need to derive from SpiderMonkey classes
if get_option('spidermonkey_rtti')
    if cxx.has_argument('-GR-')
        add_project_arguments('-GR-', language: 'cpp')  # MSVC/clang-cl option
    endif
else
    if cxx.has_argument('-fno-rtti')
        add_project_arguments('-fno-rtti', language: 'cpp')
        # -fno-rtti is not compatible with the vptr sanitizer (part of ubsan)
        if get_option('b_sanitize') != 'none' and cxx.has_argument('-fno-sanitize=vptr')
            add_project_arguments('-fno-sanitize=vptr', language: 'cpp')
        endif
    endif
endif

if get_option('verbose_logs')
    add_project_arguments([
        '-DGJS_VERBOSE_ENABLE_PROPS=1',
        '-DGJS_VERBOSE_ENABLE_MARSHAL=1',
        '-DGJS_VERBOSE_ENABLE_LIFECYCLE=1',
        '-DGJS_VERBOSE_ENABLE_GI_USAGE=1',
        '-DGJS_VERBOSE_ENABLE_GCLOSURE=1',
        '-DGJS_VERBOSE_ENABLE_GSIGNAL=1',
    ], language: 'cpp')
endif

if get_option('buildtype').startswith('release')
    add_project_arguments('-DG_DISABLE_CAST_CHECKS', language: ['c', 'cpp'])
endif

### Check for required libraries ###############################################

glib_required_version = '>= 2.58.0'
glib = dependency('glib-2.0', version: glib_required_version,
    fallback: ['glib', 'libglib_dep'])
gthread = dependency('gthread-2.0', version: glib_required_version,
    fallback: ['glib', 'libgthread_dep'])
gobject = dependency('gobject-2.0', version: glib_required_version,
    fallback: ['glib', 'libgobject_dep'])
gio = dependency('gio-2.0', version: glib_required_version,
    fallback: ['glib', 'libgio_dep'])
ffi = dependency('libffi', fallback: ['libffi', 'ffi_dep'])
gi = dependency('gobject-introspection-1.0', version: '>= 1.58.3',
    fallback: ['gobject-introspection', 'girepo_dep'])
spidermonkey = dependency('mozjs-78')

# We might need to look for the headers and lib's for Cairo
# manually on MSVC/clang-cl builds...
cairo = dependency('cairo', required: get_option('cairo').enabled() and cxx.get_argument_syntax() != 'msvc')
cairo_gobject = dependency('cairo-gobject', required: cairo.found() and cxx.get_argument_syntax() != 'msvc')
cairo_xlib = dependency('cairo-xlib', required: false)

if cxx.get_argument_syntax() == 'msvc'
    if not cairo.found()
        cairo = cc.find_library('cairo', has_headers: ['cairo.h'], required: get_option('cairo').enabled())
    endif
    if not cairo_gobject.found()
        cairo_gobject = cc.find_library('cairo-gobject', has_headers: ['cairo-gobject.h'], required: cairo.found())
    endif
endif

sysprof_capture = dependency('sysprof-capture-4',
    required: get_option('profiler'),
    fallback: ['sysprof', 'libsysprof_capture_dep'],
    default_options: [
        'enable_examples=false',
        'enable_gtk=false',
        'enable_tests=false',
        'enable_tools=false',
        'libsysprof=false',
        'with_sysprofd=none',
        'help=false',
    ])

readline = cxx.find_library('readline', required: get_option('readline'))
# On some systems we need to link readline to a termcap compatible library
readline_code = '''
#include <cstdio>
#include <readline/readline.h>
int main(void) {
    readline("foo");
    return 0;
}'''
readline_deps = [readline]
if readline.found() and not cxx.links(readline_code, dependencies: readline)
    extra_readline_libs = ['ncursesw', 'ncurses', 'curses', 'termcap']
    found = false
    foreach lib : extra_readline_libs
        termcap = cxx.find_library(lib, required: false)
        if cxx.links(readline_code, dependencies: [readline, termcap])
            found = true
            readline_deps += termcap
            break
        endif
    endforeach
    if not found
        error('''Couldn't figure out how to link
readline library. Configure with -Dreadline=disabled to skip the readline
features.''')
    endif
endif

build_profiler = sysprof_capture.found() and not get_option('profiler').disabled()
profiler_deps = [sysprof_capture]
if build_profiler and not cxx.has_function('timer_settime')
    extra_timer_libs = ['rt', 'posix4']
    found = false
    foreach lib : extra_timer_libs
        timer_lib = cxx.find_library(lib, required: false)
        if cxx.has_function('timer_settime', dependencies: timer_lib)
            found = true
            profiler_deps += timer_lib
            break
        endif
    endforeach
    if not found or not cxx.has_header_symbol('signal.h', 'SIGEV_THREAD_ID')
        if get_option('profiler').enabled()
            error('''The profiler is currently only
supported on Linux. The standard library must support timer_settime() and
SIGEV_THREAD_ID. Configure with -Dprofiler=auto or -Dprofiler=disabled to skip
it on other platforms.''')
        endif
        build_profiler = false
    endif
endif

build_cairo = cairo.found() and not get_option('cairo').disabled()
build_readline = readline.found() and not get_option('readline').disabled()

### Check for library features #################################################

# Check if SpiderMonkey was compiled with --enable-debug. If this is the case,
# you must compile all your sources with -DDEBUG=1
# See https://bugzilla.mozilla.org/show_bug.cgi?id=1261161
debug_arg = []
nondebug_spidermonkey = cxx.compiles('''
#include <js-config.h>
#ifdef JS_DEBUG
#error debug yes, if we did not already error out due to DEBUG not being defined
#endif
''',
    dependencies: spidermonkey,
    name: 'SpiderMonkey is a non-debug build')

if not nondebug_spidermonkey
    debug_arg = ['-DDEBUG']  # for compile tests
endif

if get_option('buildtype').startswith('release') and not nondebug_spidermonkey
    error('''You are trying to make a release
build with a debug-enabled copy of SpiderMonkey. This is probably not what you
want, since it will have bad performance and is not binary-compatible with
release builds of SpiderMonkey. Try configuring SpiderMonkey with
--disable-debug.''')
endif

# Check if a minimal SpiderMonkey program compiles, links, and runs. If not,
# it's most likely the case that SpiderMonkey was configured incorrectly, for
# example by building mozglue as a shared library.
minimal_program = cxx.run('''
#include <js/Initialization.h>
int main(void) {
    if (!JS_Init()) return 1;
    JS_ShutDown();
    return 0;
}
''',
    args: debug_arg, dependencies: spidermonkey,
    name: 'SpiderMonkey sanity check')

if not minimal_program.compiled() or minimal_program.returncode() != 0
    error('''A minimal SpiderMonkey program
could not be compiled, linked, or run. Most likely you should build it with a
different configuration. Check the recommended configuration:
https://github.com/spidermonkey-embedders/spidermonkey-embedding-examples/blob/esr78/docs/Building%20SpiderMonkey.md''')
endif

have_printf_alternative_int = cc.compiles('''
#include <stdio.h>
int main(void) {
    printf("%Id", (int)0);
    return 0;
}
''',
    args: ['-Werror', '-Wformat'],
    name: 'printf() supports %I alternative int syntax')

### Check for external programs ################################################

dtrace = find_program('dtrace', required: get_option('dtrace'))
dbus_run_session = find_program('dbus-run-session',
    required: not get_option('skip_dbus_tests'))
glib_compile_schemas = find_program('glib-compile-schemas')

### Generate config.h ##########################################################

header_conf = configuration_data()

versions = meson.project_version().split('.')
major_version = versions[0].to_int()
minor_version = versions[1].to_int()
micro_version = versions[2].to_int()
int_version = (major_version * 100 + minor_version) * 100 + micro_version
header_conf.set_quoted('VERSION', meson.project_version())
header_conf.set('GJS_VERSION', int_version,
    description: 'The GJS version as an integer')
header_conf.set_quoted('PACKAGE_STRING', '@0@ @1@'.format(meson.project_name(),
    meson.project_version()))

header_conf.set('ENABLE_CAIRO', build_cairo,
    description: 'Build with Cairo support')
header_conf.set('ENABLE_PROFILER', build_profiler,
    description: 'Build the profiler')
# COMPAT: SpiderMonkey headers in some places use DEBUG instead of JS_DEBUG
# https://bugzilla.mozilla.org/show_bug.cgi?id=1261161 */
header_conf.set('DEBUG', not nondebug_spidermonkey,
    description: 'SpiderMonkey was compiled with --enable-debug')
header_conf.set('HAVE_DTRACE', get_option('dtrace'),
    description: 'Using dtrace probes')
header_conf.set('HAVE_PRINTF_ALTERNATIVE_INT', have_printf_alternative_int,
    description: 'printf() accepts "%Id" for alternative integer output')

if build_readline and readline.found()
        header_conf.set('HAVE_READLINE_READLINE_H',
                        cxx.check_header('readline/readline.h', prefix: '#include <cstdio>'))
endif

header_conf.set('HAVE_SYS_SYSCALL_H', cxx.check_header('sys/syscall.h'))
header_conf.set('HAVE_UNISTD_H', cxx.check_header('unistd.h'))

if build_profiler
    header_conf.set('HAVE_SIGNAL_H', cxx.check_header('signal.h'))
endif

# enable GNU extensions on systems that have them
header_conf.set('_GNU_SOURCE', 1)

configure_file(output: 'config.h', configuration: header_conf)

### Check for environment ######################################################

gjsjsdir = get_option('datadir') / api_name
abs_datadir = get_option('prefix') / get_option('datadir')
pkglibdir = get_option('libdir') / meson.project_name()

### Build dtrace probes ########################################################

if get_option('dtrace')
    probes_header_gen = generator(dtrace, output: '@BASENAME@.h',
        arguments: ['-C', '-h', '-s', '@INPUT@', '-o', '@OUTPUT@'])
    probes_objfile_gen = generator(dtrace, output: '@BASENAME@.o',
        arguments: ['-G', '-s', '@INPUT@', '-o', '@OUTPUT@'])
    probes_header = probes_header_gen.process('gi/gjs_gi_probes.d')
    probes_objfile = probes_objfile_gen.process('gi/gjs_gi_probes.d')
else
    probes_header = []
    probes_objfile = []
endif

tapset_subst = configuration_data({
    'EXPANDED_LIBDIR': get_option('libdir'),
})
tapset = configure_file(input: 'cjs/cjs.stp.in', output: 'cjs.stp',
    configuration: tapset_subst)
if get_option('systemtap')
    install_data(tapset,
        install_dir: get_option('datadir') / 'systemtap/tapset')
endif

### Build library ##############################################################

directory_defines = [
    '-DGJS_JS_DIR="@0@"'.format(get_option('prefix') / gjsjsdir),
    '-DPKGLIBDIR="@0@"'.format(get_option('prefix') / pkglibdir),
]

gjs_public_headers = [
    'cjs/context.h',
    'cjs/coverage.h',
    'cjs/error-types.h',
    'cjs/gjs.h',
    'cjs/macros.h',
    'cjs/mem.h',
    'cjs/profiler.h',
]

# For historical reasons, some files live in gi/
# Some headers in the following list were formerly public

libgjs_sources = [
    'gi/arg.cpp', 'gi/arg.h', 'gi/arg-inl.h',
    'gi/arg-cache.cpp', 'gi/arg-cache.h',
    'gi/boxed.cpp', 'gi/boxed.h',
    'gi/closure.cpp', 'gi/closure.h',
    'gi/enumeration.cpp', 'gi/enumeration.h',
    'gi/foreign.cpp', 'gi/foreign.h',
    'gi/fundamental.cpp', 'gi/fundamental.h',
    'gi/function.cpp', 'gi/function.h',
    'gi/gerror.cpp', 'gi/gerror.h',
    'gi/gjs_gi_trace.h',
    'gi/gobject.cpp', 'gi/gobject.h',
    'gi/gtype.cpp', 'gi/gtype.h',
    'gi/interface.cpp', 'gi/interface.h',
    'gi/ns.cpp', 'gi/ns.h',
    'gi/object.cpp', 'gi/object.h',
    'gi/param.cpp', 'gi/param.h',
    'gi/private.cpp', 'gi/private.h',
    'gi/repo.cpp', 'gi/repo.h',
    'gi/toggle.cpp', 'gi/toggle.h',
    'gi/union.cpp', 'gi/union.h',
    'gi/utils-inl.h',
    'gi/value.cpp', 'gi/value.h',
    'gi/wrapperutils.cpp', 'gi/wrapperutils.h',
    'cjs/atoms.cpp', 'cjs/atoms.h',
    'cjs/byteArray.cpp', 'cjs/byteArray.h',
    'cjs/context.cpp', 'cjs/context-private.h',
    'cjs/coverage.cpp',
    'cjs/debugger.cpp',
    'cjs/deprecation.cpp', 'cjs/deprecation.h',
    'cjs/engine.cpp', 'cjs/engine.h',
    'cjs/error-types.cpp',
    'cjs/global.cpp', 'cjs/global.h',
    'cjs/importer.cpp', 'cjs/importer.h',
    'cjs/mem.cpp', 'cjs/mem-private.h',
    'cjs/module.cpp', 'cjs/module.h',
    'cjs/native.cpp', 'cjs/native.h',
    'cjs/profiler.cpp', 'cjs/profiler-private.h',
    'cjs/stack.cpp',
    'modules/console.cpp', 'modules/console.h',
    'modules/modules.cpp', 'modules/modules.h',
    'modules/print.cpp', 'modules/print.h',
    'modules/system.cpp', 'modules/system.h',
]

# GjsPrivate introspection sources
libgjs_private_sources = [
    'libgjs-private/gjs-gdbus-wrapper.c', 'libgjs-private/gjs-gdbus-wrapper.h',
    'libgjs-private/gjs-util.c', 'libgjs-private/gjs-util.h',
]

libgjs_jsapi_sources = [
    'cjs/jsapi-class.h',
    'cjs/jsapi-dynamic-class.cpp',
    'cjs/jsapi-util-args.h',
    'cjs/jsapi-util-error.cpp',
    'cjs/jsapi-util-root.h',
    'cjs/jsapi-util-string.cpp',
    'cjs/jsapi-util.cpp', 'cjs/jsapi-util.h',
    'util/log.cpp', 'util/log.h',
    'util/misc.cpp', 'util/misc.h',
]

module_cairo_srcs = [
    'modules/cairo-private.h',
    'modules/cairo-module.h',
    'modules/cairo-region.cpp',
    'modules/cairo-context.cpp',
    'modules/cairo-path.cpp',
    'modules/cairo-surface.cpp',
    'modules/cairo-image-surface.cpp',
    'modules/cairo-ps-surface.cpp',
    'modules/cairo-pdf-surface.cpp',
    'modules/cairo-svg-surface.cpp',
    'modules/cairo-pattern.cpp',
    'modules/cairo-gradient.cpp',
    'modules/cairo-linear-gradient.cpp',
    'modules/cairo-radial-gradient.cpp',
    'modules/cairo-surface-pattern.cpp',
    'modules/cairo-solid-pattern.cpp',
    'modules/cairo.cpp',
]

module_resource_srcs = gnome.compile_resources('js-resources',
    'js.gresource.xml',
    c_name: 'js_resources')

libgjs_dependencies = [glib, gobject, gthread, gio, gi, ffi, spidermonkey,
    readline]
pkg_dependencies = [glib, gobject, gthread, gio, gi, ffi, spidermonkey]
libraries_private = []

if build_cairo
    libgjs_sources += module_cairo_srcs
    libgjs_dependencies += [cairo, cairo_gobject]
    if cairo.type_name() == 'pkgconfig'
        pkg_dependencies += [cairo]
    elif cairo.type_name() == 'library'
        libraries_private += cairo
    endif
    if cairo_gobject.type_name() == 'pkgconfig'
        pkg_dependencies += [cairo_gobject]
    elif cairo_gobject.type_name() == 'library'
        libraries_private += cairo_gobject
    endif
    if cairo_xlib.found()
        libgjs_dependencies += cairo_xlib
        pkg_dependencies += cairo_xlib
    endif
endif

if build_readline
    libgjs_dependencies += readline_deps
endif

if build_profiler
    libgjs_dependencies += profiler_deps
endif

link_args = []
if cxx.has_link_argument('-Wl,-no-undefined')
    link_args += '-Wl,-no-undefined'
else
    # -undefined error is the equivalent of -no-undefined for the macOS linker,
    # but -undefined would also be understood as a valid argument for GNU ld!
    link_args += cxx.get_supported_link_arguments('-Wl,-undefined,error')
endif

libgjs_cpp_args = ['-DGJS_COMPILATION'] + directory_defines

# Check G-I and/or Meson on this one.
libgjs_cpp_args += ['-DG_LOG_DOMAIN="Cjs"']

if host_machine.system() == 'windows'
    # We need these defines to build properly for all Windows builds
    libgjs_cpp_args += ['-DWIN32', '-DXP_WIN']
endif

libgjs_jsapi = static_library(meson.project_name() + '-jsapi',
    libgjs_jsapi_sources, probes_header, probes_objfile,
    cpp_args: libgjs_cpp_args,
    link_args: link_args,
    dependencies: libgjs_dependencies,
    install: false)

symbol_map = files('libgjs.map')
symbol_list = files('libgjs.symbols')
link_args += cxx.get_supported_link_arguments([
    '-Wl,--version-script,@0@/@1@'.format(meson.current_source_dir(),
        symbol_map[0]),
    '-Wl,-exported_symbols_list,@0@/@1@'.format(meson.current_source_dir(),
        symbol_list[0]),  # macOS linker
])

libgjs = shared_library(meson.project_name(),
    libgjs_sources, libgjs_private_sources, module_resource_srcs,
    probes_header, probes_objfile,
    cpp_args: libgjs_cpp_args,
    link_args: link_args, link_depends: [symbol_map, symbol_list],
    link_with: libgjs_jsapi,
    dependencies: libgjs_dependencies,
    version: '0.0.0', soversion: '0',
    gnu_symbol_visibility: 'hidden',
    install: true)

install_headers(gjs_public_headers, subdir: api_name / 'cjs')

# Allow using libgjs as a subproject
libgjs_dep = declare_dependency(link_with: [libgjs, libgjs_jsapi],
    dependencies: libgjs_dependencies, include_directories: top_include)

### Build GjsPrivate introspection library #####################################

gjs_private_gir = gnome.generate_gir(libgjs,
    includes: ['GObject-2.0', 'Gio-2.0'], sources: libgjs_private_sources,
    namespace: 'CjsPrivate', nsversion: '1.0', identifier_prefix: 'Gjs',
    symbol_prefix: 'gjs_', extra_args: '--warn-error', install: true,
    install_dir_gir: false, install_dir_typelib: pkglibdir / 'girepository-1.0')
gjs_private_typelib = gjs_private_gir[1]

### Build gjs-console interpreter ##############################################

gjs_console_srcs = ['cjs/console.cpp']

gjs_console = executable('cjs-console', gjs_console_srcs,
    cpp_args: libgjs_cpp_args,
    dependencies: libgjs_dep, install: true)

meson.add_install_script('build/symlink-gjs.py', get_option('bindir'))

### Install data files #########################################################

install_data('installed-tests/extra/gjs.supp',
    install_dir: get_option('datadir') / api_name / 'valgrind')
install_data('installed-tests/extra/lsan.supp',
    install_dir: get_option('datadir') / api_name / 'lsan')

if get_option('installed_tests')
    schemadir = abs_datadir / 'glib-2.0' / 'schemas'
    install_data('installed-tests/js/org.cinnamon.CjsTest.gschema.xml', install_dir: schemadir)
    meson.add_install_script('build/compile-gschemas.py', schemadir)
endif

### Generate pkg-config file ###################################################

pkg.generate(libgjs, name: api_name, description: 'JS bindings for GObjects',
    requires: [glib, gobject, gio], requires_private: pkg_dependencies,
    libraries_private: libraries_private,
    subdirs: api_name,
    variables: [
        'exec_prefix=${prefix}',
        'bindir=${exec_prefix}' / get_option('bindir'),
        'datarootdir=${prefix}' / get_option('datadir'),
        'datadir=${datarootdir}',
        'gjs_console=${bindir}/cjs-console',
    ])

### Test environment ###########################################################

tests_environment = environment()
js_tests_builddir = meson.current_build_dir() / 'installed-tests' / 'js'
# GJS_PATH is empty here since we want to force the use of our own
# resources. G_FILENAME_ENCODING ensures filenames are not UTF-8
tests_environment.set('TOP_BUILDDIR', meson.build_root())
tests_environment.set('GJS_USE_UNINSTALLED_FILES', '1')
tests_environment.set('GJS_PATH', '')
tests_environment.prepend('GI_TYPELIB_PATH', meson.current_build_dir(),
    js_tests_builddir)
tests_environment.prepend('LD_LIBRARY_PATH', meson.current_build_dir(),
    js_tests_builddir)
tests_environment.set('G_FILENAME_ENCODING', 'latin1')
tests_environment.set('LSAN_OPTIONS', 'exitcode=23,suppressions=@0@'.format(
    meson.current_source_dir() / 'installed-tests' / 'extra' / 'lsan.supp'))
tests_environment.set('NO_AT_BRIDGE', '1')
tests_environment.set('GSETTINGS_SCHEMA_DIR', js_tests_builddir)
tests_environment.set('GSETTINGS_BACKEND', 'memory')
tests_environment.set('G_DEBUG', 'fatal-warnings,fatal-criticals')

tests_locale = 'N/A'
if cxx.get_argument_syntax() != 'msvc'
    result = run_command('build/choose-tests-locale.sh')
    if result.returncode() == 0
        tests_locale = result.stdout().strip()
        tests_environment.set('LC_ALL', tests_locale)
    endif
endif

if not get_option('skip_gtk_tests')
    tests_environment.set('ENABLE_GTK', 'yes')
endif

if get_option('b_coverage')
    tests_environment.set('GJS_UNIT_COVERAGE_OUTPUT', 'lcov')
    tests_environment.set('GJS_UNIT_COVERAGE_PREFIX',
        'resource:///org/gnome/gjs')
endif

### Tests and test setups ######################################################

# Note: The test program in test/ needs to be ported
#       to Windows before we can build it on Windows.
if host_machine.system() != 'windows'
    subdir('test')
endif

if not get_option('skip_gtk_tests')
    have_gtk4 = dependency('gtk4', required: false).found()
endif

subdir('installed-tests')

valgrind_environment = environment()
valgrind_environment.set('G_SLICE', 'always-malloc,debug-blocks')
valgrind_environment.set('G_DEBUG',
    'fatal-warnings,fatal-criticals,gc-friendly')
valgrind_environment.set('VALGRIND', 'valgrind')

glib_suppresssions = (glib.get_pkgconfig_variable('prefix') / 'share' /
    'glib-2.0' / 'valgrind' / 'glib.supp')
gjs_suppressions = (meson.current_source_dir() / 'installed-tests' / 'extra' /
    'cjs.supp')
valgrind_args = [
    '--suppressions=@0@'.format(glib_suppresssions),
    '--suppressions=@0@'.format(gjs_suppressions),
    '--leak-check=full',
    '--num-callers=15',
    '--trace-children=yes',
    '--trace-children-skip=*basename,*cat,*diff,*echo,*grep,*rm,*sed,*stat,*true',
    '--error-exitcode=1'
]

add_test_setup('valgrind', timeout_multiplier: 40, env: valgrind_environment,
    exe_wrapper: ['valgrind'] + valgrind_args)

zeal2_environment = environment()
zeal2_environment.set('JS_GC_ZEAL', '2,10')
add_test_setup('extra_gc', timeout_multiplier: 40, env: zeal2_environment)

zeal4_environment = environment()
zeal4_environment.set('JS_GC_ZEAL', '4')
add_test_setup('pre_verify', timeout_multiplier: 40, env: zeal4_environment)

zeal11_environment = environment()
zeal11_environment.set('JS_GC_ZEAL', '11')
add_test_setup('post_verify', timeout_multiplier: 2, env: zeal11_environment)

### Warn about conditions that may affect runtime ##############################

if gi.version().version_compare('<1.61.2')
    warning('''You do not have a new enough version of
gobject-introspection to run the tests. You can still build GJS, but some
tests will fail.''')
endif

if tests_locale == 'C' or tests_locale == 'N/A'
    warning('''Your libc does not have the C.UTF-8 locale and no other
suitable UTF-8 fallback locale could be found. You can still build GJS, but
some tests will fail.''')
endif

if get_option('buildtype').startswith('debug') and nondebug_spidermonkey
    warning('''Your copy of SpiderMonkey is not debug-enabled, but you are
building a debug or debugoptimized build. This will make development more
difficult. Consider reconfiguring SpiderMonkey with --enable-debug.''')
endif

if not build_cairo
    warning('Building without Cairo support, not all tests will be run.')
endif

if get_option('skip_gtk_tests')
    warning('Not using GTK, not all tests will be run.')
endif

if get_option('skip_dbus_tests')
    warning('Not using DBus, not all tests will be run.')
endif

### Summarize options ##########################################################

message('\n'.join([
    'Optional features',
    '==========================',
    'Build Cairo module: @0@'.format(build_cairo),
    'Use readline for input in interactive shell and debugger: @0@'.format(
        build_readline),
    'Build profiler (Linux only): @0@'.format(build_profiler),
]))
