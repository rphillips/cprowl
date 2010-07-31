APPNAME = 'cprowl'
VERSION = '0.5.0'

top = '.'
out = 'build'

def set_options(opt):
    # the gcc module provides a --debug-level option
    opt.tool_options('compiler_cc')

def configure(conf):
    conf.check_tool('compiler_cc')
    if not conf.env.CC: conf.fatal('c compiler not found')

    conf.check_cfg(package='libcurl',
                   mandatory=True,
                   args='--cflags --libs')

    conf.define('CPROWL_VERSION', VERSION)
    conf.define('CPROWL_NAME', APPNAME)
    conf.write_config_header('cprowl_config.h')

def build(bld):
    cprowl = bld.new_task_gen()
    cprowl.features = ['cc', 'cprogram']
    cprowl.source = "cprowl.c"
    cprowl.name = "cprowl"
    cprowl.target = "cprowl"
    cprowl.includes = '.'
    cprowl.install_path = '${PREFIX}/bin'
    cprowl.uselib = 'LIBCURL'
