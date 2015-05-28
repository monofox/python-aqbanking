from distutils.core import setup, Extension
import os
os.environ["CC"] = "g++-4.9.2" 
os.environ["CXX"] = "g++-4.9.2" 

module1 = Extension('aqbanking',
                    libraries = ['gwenhywfar', 'aqbanking', 'gwengui-cpp'],
                    include_dirs = ['/usr/include/gwenhywfar4', '/usr/include/aqbanking5'],
					#extra_compile_args=['-O0', '-g', '-Wunused-variable', '-std=gnu++11', '-DPy_DEBUG', '-Wunused-function'],
					extra_compile_args=['-Wunused-variable', '-std=gnu++11', '-Wunused-function'],
                    sources = ['aqbanking/pyaqhandler.cpp', 'aqbanking/aqbanking.cpp'])

setup (name = 'python-aqbanking',
       version = '1.0',
       description = 'This is a client for AqBanking',
       ext_modules = [module1])
