from distutils.core import setup, Extension

module1 = Extension('aqbanking',
                    libraries = ['gwenhywfar', 'aqbanking'],
                    include_dirs = ['/usr/include/gwenhywfar4', '/usr/include/aqbanking5'],
                    sources = ['aqbanking/aqbanking.c'])

setup (name = 'python-aqbanking',
       version = '1.0',
       description = 'This is a client for AqBanking',
       ext_modules = [module1])
