APPNAME = 'uep'
VERSION = '0.1'

top = '.'
out = 'build'

def options(ctx):
    ctx.load('compiler_cxx python')
    # ctx.add_option("--shared", action="store_true", help="build shared library")
    # ctx.add_option("--static", action="store_true", help="build static library")
     
def configure(ctx):
    ctx.load('compiler_cxx python')
    ctx.check_python_version((3,4))
    ctx.check_python_headers()
    ctx.env.append_value('CXXFLAGS', ["-std=c++11", "-ggdb", "-O0"])
    ctx.write_config_header('config.h')
 
def build(ctx):
        ctx.program(source="src/rng.cpp src/test_rng.cpp",
                    target="test_rng",
                    includes=["src", "/usr/include/python3.4"],
                    lib=["python3.4m", "boost_python-py34"])

        ctx.shlib(features="pyext",
                  source="src/rng_python.cpp src/rng.cpp",
                  target="rng_python",
                  includes=["src", "/usr/include/python3.4"],
                  lib=["python3.4m", "boost_python-py34"])

        ctx(features='py', source=ctx.path.ant_glob('src/*.py'), install_from='src')
        
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
