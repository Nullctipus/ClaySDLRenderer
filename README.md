# Clay SDL
A SDL render implementation for clay

PRs welcome

was made with linux in mind, SConstruct will need to be updated for windows

# Dependancies
- [Clay](https://github.com/nicbarker/clay) Obviously
- [SDL2](https://www.libsdl.org/)
- SDL2_ttf
- SDL2_image

# Building test app
Create a Python venv

`python -m venv venv`

install Scons in the venv

`venv/bin/pip install -r requirements.txt`

Run Scons

`venv/bin/Scons -j $(nproc) use_llvm=yes release=no`

Run the app

`./testapp`

# Attribution
- [Hack](https://github.com/source-foundry/Hack) used to test ttf
