# SConstruct for Noya
# @Author Mathieu Virbel <tito@bankiz.org>

# Enumerate sources and libraries
src_files = Glob('src/*.c')
lib_files = [
	# handle OSC message
	# http://liblo.sourceforge.net/
	'liblo',

	# read wave file
	# http://www.mega-nerd.com/libsndfile/
	'sndfile',

	# resample wave
	# http://www.mega-nerd.com/SRC/
	'samplerate',

	# rendering
	# http://clutter-project.org
	'clutter-0.8',

	# audio management
	# http://www.portaudio.com/
	'portaudio-2.0'
]

# Add library
env = Environment()
for lib_file in lib_files:
	env.ParseConfig('pkg-config %s --cflags --libs' % lib_file)

# Debug
env['CFLAGS'].append('-ggdb')
env['CFLAGS'].append('-O0')

# Output
env.Program('noya', src_files)
