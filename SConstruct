#!/usr/bin/env python
import os
import sys

env = SConscript("godot-cpp/SConstruct")

# For reference:
# - CCFLAGS are compilation flags shared between C and C++
# - CFLAGS are for C-specific compilation flags
# - CXXFLAGS are for C++-specific compilation flags
# - CPPFLAGS are for pre-processor flags
# - CPPDEFINES are for pre-processor defines
# - LINKFLAGS are for linking flags

# tweak this if you want to use different folders, or more folders, to store your source code in.
env.Append(CPPPATH=["src/"])
sources = Glob("src/*.cpp")
destinationproject = "/home/julian/repositories/libopus-gdnative4"

opus_headers = '/nix/store/z3ydm9blbyxpl73j1hjrng8c21lkf6di-libopus-1.3.1-dev/include/opus'
opus_libs = '/nix/store/7lyly0wrvqghz08jcjmshg2bkcngxawn-libopus-1.3.1/lib'
env.Append(CPPPATH=[opus_headers])
env.Append(LIBPATH=[opus_libs])
env.Append(LIBS=['libopus.so'])

# checkout the godot-cpp to the branch matching the version of godot you are using
# downgrading to 4.0 from 4.1 because the mix works there under linux

# see /home/julian/repositories/godot_multiplayer_networking_workbench_G4/addons/opus/opus.gdextension for the libraries

# This creates a link directly to the nix-store, when it should do it as a relative link (maybe patchelf it?)
# also do: cp /nix/store/7lyly0wrvqghz08jcjmshg2bkcngxawn-libopus-1.3.1/lib/libopus.so /home/julian/repositories/godot_multiplayer_networking_workbench_G4/addons/opus/


#env.Append(CPPPATH=['.', godot_headers_path, cpp_bindings_path + 'include/', cpp_bindings_path + 'include/core/', cpp_bindings_path + 'include/gen/', opus_headers])
#env.Append(LIBPATH=[cpp_bindings_path + 'bin/'])
#env.Append(LIBS=[cpp_library])


if env["platform"] == "macos":
    library = env.SharedLibrary(
        "demo/bin/libgdexample.{}.{}.framework/libgdexample.{}.{}".format(
            env["platform"], env["target"], env["platform"], env["target"]
        ),
        source=sources,
    )
else:
    library = env.SharedLibrary(
        "mic_record_demo/addons/opus/libgdexample{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
        source=sources,
    )

Default(library)
