#!/usr/bin/env python3

from Crypto.Cipher import AES

def hexStr(s):
    return ":".join("{:02x}".format(c) for c in s)

iv = bytes.fromhex('00000000000000000000000000000000') # 16B IV
key = bytes.fromhex('0000000000000000000000000000000000000000000000000000000000000000') # 32B key
print(f'key = {hexStr(key)}')
cipher = AES.new(key, AES.MODE_CBC, iv)

data = bytes.fromhex('00000000000000000000000000000000') # 128b/16B block
print(f'data = {hexStr(data)}')

ciphertext = cipher.encrypt(data)
print(f'ciphertext = {hexStr(ciphertext)}')

ciphertext = cipher.encrypt(data)
print(f'ciphertext = {hexStr(ciphertext)}')
