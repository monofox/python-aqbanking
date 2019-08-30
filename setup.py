import os
import sys
import pkgconfig
from setuptools import setup, Extension
from distutils.version import StrictVersion
#os.environ["CC"] = "g++-4.9.2"
#os.environ["CXX"] = "g++-4.9.2"

PACKAGE_VERSION = '0.0.8'

# retrieve the README
def read(fname):
	f = open(os.path.join(os.path.dirname(__file__), fname))
	cnt = f.read()
	f.close()
	return cnt

libraries = ['gwenhywfar', 'aqbanking']
depCompilationArgs = ['-Wunused-variable', '-Wunused-function']
depLibraryDirs = []
# check for aqbanking dependency
if not pkgconfig.exists('aqbanking'):
	sys.stderr.write('Need aqbanking development package installed for compilation.' + os.linesep)
	sys.exit(1)
else:
	for library in libraries:
		depCompilationArgs += pkgconfig.cflags(library).split(' ')
		depCompilationArgs += pkgconfig.libs(library).split(' ')
		libPath = pkgconfig.variables(library)['libdir']
		if libPath not in depLibraryDirs:
			depLibraryDirs.append(libPath)

	# furthermore remember the c++ gui!
	if StrictVersion(pkgconfig.modversion('aqbanking').replace('beta', '').replace('alpha', '')) >= StrictVersion('5.8.1'):
		depCompilationArgs.append('-DSUPPORT_APPREGISTRATION')

	depCompilationArgs += ['-DFENQUEJOB']
	#depCompilationArgs += ['-O0', '-g', '-std=gnu++11', '-Wunused-function', '-DDEBUGSTDERR']

module1 = Extension('aqbanking',
	#libraries = ['gwenhywfar', 'aqbanking', 'gwengui-cpp'],
	libraries = libraries + ['gwengui-cpp',],
	#include_dirs = ['/usr/include/gwenhywfar4', '/usr/include/aqbanking5', '/usr/local/include/gwenhywfar4', '/usr/local/include/aqbanking5'],
	# for compiling debug with python debug:
	#extra_compile_args=['-O0', '-g', '-Wunused-variable', '-std=gnu++11', '-DPy_DEBUG', '-Wunused-function', '-DDEBUG', '-DDEBUGSTDERR', '-DFENQUEJOB'],
	# for compiling debug without python debug
	#extra_compile_args=['-O0', '-g', '-Wunused-variable', '-std=gnu++11', '-Wunused-function', '-DDEBUGSTDERR', '-DFENQUEJOB'],
	# RELEASE parameter for compilation:
	extra_compile_args=depCompilationArgs,
	library_dirs=depLibraryDirs,
	sources = ['aqbanking/pyaqhandler.cpp', 'aqbanking/aqbanking.cpp']
)

setup (
	name = 'python-aqbanking',
	version = PACKAGE_VERSION,
	description = 'This is a python wrapper for AqBanking',
	long_description = read('README.md'),
	license = 'GPLv3+',
	keywords = 'aqbanking banking hbci financial',
	author = 'Lukas Schreiner',
	author_email = 'dev@lschreiner.de',
	url = 'https://github.com/monofox/python-aqbanking',
	ext_modules = [module1],
	packages = ['aqbanking'],
	classifiers = [
		'Development Status :: 3 - Alpha',
		'License :: OSI Approved :: GNU General Public License v3 or later (GPLv3+)',
		'Intended Audience :: Developers',
		'Environment :: No Input/Output (Daemon)',
		'Programming Language :: Python :: 3 :: Only'
	]
)
