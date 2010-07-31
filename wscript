def set_options(opt):
    # the gcc module provides a --debug-level option
    opt.tool_options('compiler_cc')

def configure(conf):
    conf.check_tool('compiler_cc')
    if not conf.env.CC: conf.fatal('c compiler not found')

    conf.check_cfg(package='openssl',
                   mandatory=True,
                   args='--cflags --libs')

    conf.check_cfg(package='libcurl',
                   mandatory=True,
                   args='--cflags --libs')

    #conf.check_cfg(package='libxml-2.0',
    #               mandatory=True,
    #               args='--cflags --libs')

def build(bld):
    cprowl = bld.new_task_gen()
    cprowl.features = ['cc', 'cprogram']
    cprowl.source = "cprowl.c"
    cprowl.name = "cprowl"
    cprowl.target = "cprowl"
    cprowl.install_path = '${PREFIX}/bin'
    cprowl.uselib = 'LIBCURL'
