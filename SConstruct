# SConstruct for Noya
# @Author Mathieu Virbel <tito@bankiz.org>

# Enumerate sources
src_files = Glob('src/*.c')
lib_files = ['clutter-0.8', 'liblo']

# Add library
env = Environment()
for lib_file in lib_files:
	env.ParseConfig('pkg-config %s --cflags --libs' % lib_file)

# Output
env.Program('noya', src_files)
