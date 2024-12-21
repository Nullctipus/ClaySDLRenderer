#!/bin/env python

import os
from SCons.Script import *
import scons_compiledb

# Function to recursively find all source files in the src/ directory
def find_source_files(directories):
    source_files = []
    for directory in directories:
        for root, dirs, files in os.walk(directory):
            for file in files:
                if file.endswith(('.c')):
                    source_files.append(os.path.join(root, file))
    return source_files


# Configuration
env = Environment()

scons_compiledb.enable(env)

# Check if use_llvm=yes flag is set
if ARGUMENTS.get('use_llvm') == 'yes':
    env['CC'] = 'clang'  # Set the C compiler to clang
else:
    env['CC'] = 'gcc'  # Default to gcc if use_llvm=yes flag is not set

# Set compiler flags if needed
# env.Append(CPPFLAGS=['-Wall', '-O2'])  # Example flags

env.Append(CPPPATH=['src/include', 'clay', '/usr/include/SDL2'])
env.Append(LIBS=['SDL2','SDL2_ttf','SDL2_image','m'])
env.Append(LIBPATH=['/usr/lib'])
if ARGUMENTS.get('release') == 'yes':
    env.Append(CPPFLAGS=['-O2', '-DNDEBUG'])
else:
    env.Append(CPPFLAGS=['-g'])

# Find all source files in src/ directory
source_files = find_source_files(['src','test'])

# Create Object files
object_files = [env.Object(source) for source in source_files]

# Create the final executable
executable = 'testapp'
env.Program(executable, object_files)

# Generate compile_commands.json
env.CompileDb()

# Alias to generate compile_commands.json and build the project
Alias('build', [executable, 'compile_commands.json'])
