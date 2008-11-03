# SConstruct for Noya
# @Author Mathieu Virbel <tito@bankiz.org>

# Enumerate sources and libraries
src_files = Glob('src/*.c') + Glob('src/actors/*.c')
mod_files = Glob('modules/*.c');
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
	env.ParseConfig('pkg-config %s --cflags' % lib_file)

# Testing configuration
conf = Configure(env)
if conf.CheckHeader('rtc.h') or conf.CheckHeader('linux/rtc.h'):
	env.AppendUnique(CCFLAGS='-DHAVE_RTC')
conf.Finish()

# Debug
env.AppendUnique(CCFLAGS='-Wall')
env.AppendUnique(CCFLAGS='-ggdb')
env.AppendUnique(CCFLAGS='-O0')
env.AppendUnique(CCFLAGS='-Isrc')
env.AppendUnique(CCFLAGS='-DNA_DEBUG')

# Libs
import os
for mod in mod_files:
	out = str(mod)[:-2] + '.so'
	env.SharedLibrary(target=out, source = mod, SHLIBPREFIX='')

# Links
penv = env.Clone()
for lib_file in lib_files:
	penv.ParseConfig('pkg-config %s --libs' % lib_file)

# Output
penv.Program('noya', src_files)
