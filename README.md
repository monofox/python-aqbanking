AqBanking API for Python
========================

This is a python wrapper for AqBanking - so of course you need the AqBanking and Gwenhywfar dependencies 
installed for a successful compilation. The only supported things at the moment: get balance of an account, 
get the transactions of an account (with limitation to start and end date), list configured accounts in 
AqBanking, check an IBAN.

License
=======

This library is published under the GPLv3 License. See "LICENSE" for details.

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

