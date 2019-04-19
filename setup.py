from setuptools import setup, Extension

# for some more beautiful output:
import os
#os.environ["CC"] = "g++-4.9.2"
#os.environ["CXX"] = "g++-4.9.2"

# retrieve the README
def read(fname):
	f = open(os.path.join(os.path.dirname(__file__), fname))
	cnt = f.read()
	f.close()
	return cnt

module1 = Extension('aqbanking',
	libraries = ['gwenhywfar', 'aqbanking', 'gwengui-cpp'],
	include_dirs = ['/usr/include/gwenhywfar4', '/usr/include/aqbanking5', '/usr/local/include/gwenhywfar4', '/usr/local/include/aqbanking5'],
	# for compiling debug with python debug:
	#extra_compile_args=['-O0', '-g', '-Wunused-variable', '-std=gnu++11', '-DPy_DEBUG', '-Wunused-function', '-DDEBUG', '-DDEBUGSTDERR', '-DFENQUEJOB'],
	# for compiling debug without python debug
	#extra_compile_args=['-O0', '-g', '-Wunused-variable', '-std=gnu++11', '-Wunused-function', '-DDEBUGSTDERR', '-DFENQUEJOB'],
	# RELEASE parameter for compilation:
	extra_compile_args=['-Wunused-variable', '-Wunused-function', '-DFENQUEJOB'],
	sources = ['aqbanking/pyaqhandler.cpp', 'aqbanking/aqbanking.cpp']
)

setup (
	name = 'python-aqbanking',
	version = '0.0.6',
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
