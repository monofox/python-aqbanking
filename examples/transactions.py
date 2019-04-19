#!/usr/bin/env python
# -*- coding: utf-8 -*-

import aqbanking
import datetime
import getpass

cachedPasswords = {}

def callback(domain, prio, msg):
    print('[LOG]: %r' % (msg,))

# const char *token, const char *pin, GWEN_GUI_PASSWORD_STATUS enumStatus, uint32_t guiid
# 0 = OK
# 1 = Bad
# 2 = Remove
# 4 = Used
# 8 = Unused
# 16 = Unknown
def passwordStatus_cb(token, pin, status):
    print('cb_pw_status: %s / <pin censored> / %d' % (token, status))
    if status == 2 or status == 1:
        try:
            del(cachedPasswords[token])
        except KeyError:
            pass
    elif status == 0:
        cachedPasswords[token] = pin

def password_cb(flags, token, title, text, minLen, maxLen):
    # Ask only, if we didn't asked already for a PIN.
    try:
        return cachedPasswords[token]
    except KeyError:
        plainText = text if '<html>' not in text else text[:text.find('<html>')].replace('\r', '').replace('\n', '')
        pin = getpass.getpass('%s: ' % (plainText,))
        return pin


acc = aqbanking.Account(no='100254687', bank_code='35468754')
acc.set_callbackLog(callback)
acc.set_callbackPassword(password_cb)
acc.set_callbackPasswordStatus(passwordStatus_cb)
now = datetime.datetime.now()
dateFrom = (now - datetime.timedelta(days=30)).strftime('%Y%m%d')
dateTo = now.strftime('%Y%m%d')
ret = acc.transactions(dateFrom, dateTo)
pos = 1
for trans in ret:
    print(
        '[{pos:>4d}] Transaction {localIban} ({localName}) <> {remoteIban} ({remoteName}) about {value:.2f} {curr:s} for {purpose} on {dt}!'.format(
            pos=pos,
            value=trans.value,
            curr=trans.currency,
            remoteIban=trans.remoteIban,
            remoteName='n.A.' if trans.remoteName is None else trans.remoteName,
            localIban=trans.localIban,
            localName='n.A.' if trans.localName is None else trans.localName,
            purpose=trans.purpose,
            dt=trans.date
        )
    )
    pos += 1
