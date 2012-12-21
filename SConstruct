import os
from SCons.Script import *

cc='gcc'
cxx='g++'
ccflags='-Wall -Wextra'

conf = {
	'CC': 'clang',
	'CXX': 'clang++',
	'CCFLAGS': '-O0 -g3 -Wall -Wextra -Isrc/third_party/include'
}

if os.environ.has_key('CC'):
	conf['CC'] = os.environ['CC']
if os.environ.has_key('CXX'):
	conf['CXX'] = os.environ['CXX']
if os.environ.has_key('CCFLAGS'):
	conf['CCFLAGS'] = os.environ['CCFLAGS']

# Only clang compiler has color output
if conf['CC'] == 'clang' or conf['CXX'] == 'clang++':
	conf['CCFLAGS'] = conf['CCFLAGS'] + ' -fcolor-diagnostics'

env = Environment(CC = conf['CC'],
                  CXX = conf['CXX'],
				  CCFLAGS = conf['CCFLAGS'],
                  LIBPATH = '#src third_party/gflags third_party/gmock third_party/glog'.split(),
				  platform  = 'posix')
Export('env')
SConscript('src/SConscript')
SConscript('src/third_party/gmock/SConscript')
SConscript('src/third_party/gflags/SConscript')
SConscript('src/third_party/glog/SConscript')
