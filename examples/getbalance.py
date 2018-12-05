#!/usr/bin/env python
# -*- coding: utf-8 -*-

import aqbanking
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
        except:
            pass
    elif status == 0:
        cachedPasswords[token] = pin

def password_cb(flags, token, title, text, minLen, maxLen):
    # Ask only, if we didn't asked already for a PIN.
    try:
        return cachedPasswords[token]
    except:
        plainText = text if '<html>' not in text else text[:text.find('<html>')].replace('\r', '').replace('\n', '')
        pin = getpass.getpass('%s: ' % (plainText,))
        return pin

for f in aqbanking.listacc():
    print('Available configured banks: ', f.bank_name)

acc = aqbanking.Account(no='100254687', bank_code='35468754')
acc.set_callbackLog(callback)
acc.set_callbackPassword(password_cb)
acc.set_callbackPasswordStatus(passwordStatus_cb)
ret = acc.balance()
if ret is not None:
    try:
        print(ret[0], ' ', ret[1])
    except TypeError:
        pass
