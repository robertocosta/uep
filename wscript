import os
from waflib import Options
from waflib.Build import BuildContext
from waflib.Task import Task
from waflib.Task import always_run

APPNAME = 'uep'
VERSION = '0.1'

top = '.'
out = 'build'

class TestContext(BuildContext):
    cmd = 'do_test'
    fun = 'do_test'

@always_run
class run_tests(Task):
    def run(self):
        retcodes = []
        for t in self.inputs:
            fpath = "./" + t.bldpath()
            #print("Run %s:" % t.bldpath())
            try:
                retcodes.append(self.exec_command([fpath]))
            except Exception as e:
                if e.msg.find("OSError: [Errno 13] Permission denied") == -1:
                    raise
                else:
                    pass # Ignore not executable files
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
    common_cxxflags = ['-Wall', '-std=c++11', '-ggdb', '-O0', '-pg']
    common_linkflags = ['-pg']
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
    # ffmpeg_libs = ['avformat',
    #                'avcodec',
    #                'avutil',
    #                'swresample']

    ctx.load('compiler_cxx python protoc')
    ctx.check_python_version((2,7))
    # ctx.check_cxx(lib=ffmpeg_libs,
    #               cxxflags=common_cxxflags,
    #               linkflags=common_linkflags,
    #               includes=["src",
    #                         "/home/ricc/local/include"],
    #               libpath="/home/ricc/local/lib",
    #               uselib_store='FFMPEG_LIBS')
    ctx.check_cxx(lib=system_libs,
                  cxxflags=common_cxxflags,
                  linkflags=common_linkflags,
                  includes="src",
                  uselib_store='SYSTEM_LIBS')
    if ctx.options.static:
        ctx.check_cxx(stlib=boost_libs,
                      cxxflags=common_cxxflags,
                      linkflags=common_linkflags,
                      includes=["src"],
                      defines=boost_defines,
                      uselib_store='BOOST_LIBS')
    else:
        ctx.check_cxx(lib=boost_libs,
                      cxxflags=common_cxxflags,
                      linkflags=common_linkflags,
                      includes=["src"],
                      defines=boost_defines,
                      uselib_store='BOOST_LIBS')

    ctx.write_config_header('config.h')

def doxygen(ctx):
    os.system("cd doc && doxygen")

def test(ctx):
    Options.commands = ['build', 'do_test'] + Options.commands

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

    # ctx.program(target="test_ffmpeg",
    #             source=["test/test_ffmpeg.cpp"],
    #             use=['SYSTEM_LIBS',
    #                  'BOOST_LIBS',
    #                  'FFMPEG_LIBS'])

    ctx.program(target="test_data_client_server",
                source=["test/test_data_client_server.cpp",
                        "src/packets.cpp",
                        "src/packets_rw.cpp",
                        "src/rng.cpp",
                        "src/decoder.cpp",
                        "src/block_encoder.cpp",
                        "src/block_decoder.cpp",
                        "src/block_queues.cpp"],
                use=['SYSTEM_LIBS',
                     'BOOST_LIBS'])

    ctx.program(target="test_rng",
                source=["test/test_rng.cpp",
                        "src/rng.cpp"],
                use=['SYSTEM_LIBS', 'BOOST_LIBS'])
    ctx.program(target="test_packets",
                source=["test/test_packets.cpp",
                        "src/packets.cpp"],
                use=['SYSTEM_LIBS', 'BOOST_LIBS'])
    ctx.program(target="test_counters",
                source=["test/test_counters.cpp"],
                use=['SYSTEM_LIBS', 'BOOST_LIBS'])
    ctx.program(target="test_block_decoder",
                source=["test/test_block_decoder.cpp",
                        "src/block_decoder.cpp",
                        "src/packets.cpp",
                        "src/rng.cpp"],
                use=['SYSTEM_LIBS', 'BOOST_LIBS'])
    ctx.program(target="test_block_encoder",
                source=["test/test_block_encoder.cpp",
                        "src/block_encoder.cpp",
                        "src/packets.cpp",
                        "src/rng.cpp"],
                use=['SYSTEM_LIBS', 'BOOST_LIBS'])
    ctx.program(target="test_encoder_decoder",
                source=["test/test_encoder_decoder.cpp",
                        "src/rng.cpp",
                        "src/packets.cpp",
                        "src/decoder.cpp",
                        "src/block_encoder.cpp",
                        "src/block_decoder.cpp",
                        "src/block_queues.cpp"],
                use=['SYSTEM_LIBS', 'BOOST_LIBS'])
    ctx.program(target="test_message_passing",
                source=["test/test_message_passing.cpp",
                        "src/packets.cpp"],
                use=['SYSTEM_LIBS', 'BOOST_LIBS'])
    ctx.program(target="client",
                source=["src/client.cpp"],
                use=['SYSTEM_LIBS', 'BOOST_LIBS'])
    ctx.program(target="test_packet_rw",
                source=["test/test_packet_rw.cpp",
                        "src/packets.cpp",
                        "src/packets_rw.cpp"],
                use=['SYSTEM_LIBS', 'BOOST_LIBS'])

    ctx(features='cxx cxxprogram',
        source='src/server.cpp src/controlMessage.proto',
        use=['SYSTEM_LIBS', 'BOOST_LIBS'],
        target='server'
        )

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
