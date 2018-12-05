AqBanking API for Python
========================

[![Build Status](https://travis-ci.org/monofox/python-aqbanking.svg?branch=master)](https://travis-ci.org/monofox/python-aqbanking)

This is a python wrapper for AqBanking - so of course you need the AqBanking and Gwenhywfar dependencies 
installed for a successful compilation. The only supported things at the moment: get balance of an account, 
get the transactions of an account (with limitation to start and end date), list configured accounts in 
AqBanking, check an IBAN.

License
=======

This library is published under the GPLv3 License. See "LICENSE" for details.

Dependencies
============

The proper development packages are required for:
 - AqBanking >= 5.4.0
 - Python >= 3.1
 - gwenhywfar >= 4.0.0

For the installation, it is necessary to have the proper development packages installed (e.g. `apt-get install libaqbanking-dev libgwenhywfar60-dev`)

Install
=======

To install this library, just execute (append --user if it should not be installed systemwide):
`python setup.py install`

Please remember, that this library only works with Python 3.

Usage
======

To import it, just do the following:
`import aqbanking`

And then you can verify the IBAN number e.g. with:
`aqbanking.chkibn('DE19....')`

And to list all configured accounts (you can not configure it through this library at the moment), you execute this command:
`aqbanking.listacc()`

For all other functions, you need first to create an account:
`acc = aqbanking.Account(no=157458624, bank_code=45021512)`

New is a function in order to get the information, which jobs or features are available:
`acc.availableJobs()`
Implemented is: `nationalTransfer` and `sepaTransfer`.

Furthermore if you're doing some transfer you're partially asked to enter three times the password. Now you can build your PIN cache with help of the `set_callbackPasswordStatus` function. This calls the python callback with parameters `token`, `pin` and `status` whereas the status field can be 9 = reset, 1 = Bad password, 2 = Remove password and 0 = all went fine.

You can find some examples inside of the `examples/` folder.

Known Bugs/Missing features
===========================
Smartcard/Chipcard support meanwhile integrated. But no "text" that user has to enter something on the readers panel is provided. 

The server certificate of the HTTPS connection is not validated at the moment, so do not use it for sensitive data, as man in the middle attack is possible without notice.

