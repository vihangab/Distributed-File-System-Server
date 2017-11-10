#!/usr/bin/python
import pexpect
import sys
import os
file_name = sys.argv[1]
passphrase = sys.argv[2]
passphrase.strip()
file_name.strip()
child = pexpect.spawn ('gpg -o temp -c '+file_name)
child.expect ('Enter passphrase:')
child.sendline (passphrase)
child.expect ('Repeat passphrase:')
child.sendline (passphrase)
os.system('mv -f temp '+file_name)
