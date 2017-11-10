#!/usr/bin/python
import pexpect
import sys
import os
file_name = sys.argv[1]
passphrase = sys.argv[2]
passphrase.strip()
file_name.strip()
child = pexpect.spawn ('gpg '+file_name)
child.expect ('Enter passphrase:')
child.sendline (passphrase)
child.expect('Enter new filename')
child.sendline(file_name)
child.expect ('Overwrite')
child.sendline ('y')
#os.system('mv temp '+file_name)
