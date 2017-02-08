import os
from waflib.Task import Task
from waflib.Build import BuildContext

APPNAME = 'uep'
VERSION = '0.1'

top = '.'
out = 'build'

class TestContext(BuildContext):
    cmd = 'do_test'
    fun = 'do_test'

class run_tests(Task):
    def run(self):
        retcodes = []
        for t in self.inputs:
            #print("Run %s:" % t.bldpath())
            retcodes.append(self.exec_command(["./" + t.bldpath()]))
        if all(map(lambda x: x == 0, retcodes)):
            return 0
        else:
            return 1

def options(ctx):
    ctx.load('compiler_cxx python')
    ctx.add_option("--static", dest="static",
                   help="Statically link the libraries",
                   action="store_true", default=False)

def configure(ctx):
    common_cxxflags = ['-Wall', '-std=c++11', '-ggdb', '-O0']
    boost_defines = []
    if not ctx.options.static:
        boost_defines.append('BOOST_ALL_DYN_LINK')

    system_libs = ['pthread']
    boost_libs = ['boost_unit_test_framework',
                  'boost_log',
                  'boost_log_setup',
                  'boost_system',
                  'boost_thread',
                  'boost_date_time',
                  'boost_filesystem']

    ctx.load('compiler_cxx python')
    ctx.check_python_version()
    ctx.check_cxx(lib=system_libs,
                  cxxflags=common_cxxflags,
                  includes="src",
                  uselib_store='SYSTEM_LIBS')
    if ctx.options.static:
        ctx.check_cxx(stlib=boost_libs,
                      cxxflags=common_cxxflags,
                      includes=["src"],
                      defines=boost_defines,
                      uselib_store='BOOST_LIBS')
    else:
        ctx.check_cxx(lib=boost_libs,
                      cxxflags=common_cxxflags,
                      includes=["src"],
                      defines=boost_defines,
                      uselib_store='BOOST_LIBS')

    ctx.write_config_header('config.h')

def test(ctx):
    os.system('./waf build do_test')

def do_test(ctx):
    rt = run_tests(env=ctx.env)
    rt.inputs = ctx.path.ant_glob(out + '/test_*')
    ctx.add_to_group(rt)

def build(ctx):
    ctx(features='py',
        source=ctx.path.ant_glob('src/*.py'))
    ctx(rule="cp ${SRC} ${TGT}",
        source = ctx.path.ant_glob("src/*.py"),
        target = '.')

    ctx.program(target="test_rng",
                source=["test/test_rng.cpp",
                        "src/rng.cpp"],
                use=['SYSTEM_LIBS', 'BOOST_LIBS'])
    ctx.program(target="test_encoder_decoder",
                source=["test/test_encoder_decoder.cpp",
                        "src/rng.cpp",
                        "src/packets.cpp",
                        "src/decoder.cpp"],
                use=['SYSTEM_LIBS', 'BOOST_LIBS'])
    ctx.program(target="test_udp",
                source=["src/udp_client_server.cpp"],
                use=['SYSTEM_LIBS', 'BOOST_LIBS'])
    ctx.program(target="test_packet_rw",
                source=["test/test_packet_rw.cpp",
                        "src/packets.cpp",
                        "src/packets_rw.cpp"],
                use=['SYSTEM_LIBS', 'BOOST_LIBS'])

        # ctx.program(source="src/rng.cpp src/test_rng.cpp",
        #             target="test_rng",
        #             includes=["src", "/usr/include/python3.4"],
        #             lib=["python3.4m", "boost_python-py34"])

        # ctx.program(source="src/test_fountain.cpp src/rng.cpp src/encoder.cpp src/decoder.cpp src/packets.cpp",
        #             target="test_fountain",
        #             includes=["src", "/usr/include/python3.4"],
        #             lib=["python3.4m", "boost_python-py34"])


        # ctx.shlib(features="pyext",
        #           source="src/rng_python.cpp src/rng.cpp",
        #           target="rng_python",
        #           includes=["src", "/usr/include/python3.4"],
        #           lib=["python3.4m", "boost_python-py34"])


    # if ctx.options.shared:
    #     ctx.shlib(source="src/Hello.cpp", target="hello", includes=["include"],
    #               cxxflags="-g -Wall -O0")
    # elif ctx.options.static:
    #     ctx.stlib(source="src/Hello.cpp", target="hello", includes=["include"],
    #               cxxflags="-g -Wall -O0")
    # else: # by default, build a shared library
    #     ctx.shlib(source="src/Hello.cpp", target="hello", includes=["include"],
    #               cxxflags="-g -Wall -O0")

    # ctx.program(source="src/Main.cpp", target="main", includes=["include"],
    #             use="hello")

# Local Variables:
# mode: python
# End:
