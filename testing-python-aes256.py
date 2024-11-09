#!/usr/bin/env python3

from Crypto.Cipher import AES

def hexStr(s):
    return ":".join("{:02x}".format(c) for c in s)

iv = bytes.fromhex('00000000000000040000000000000008') # 16B IV
key = bytes.fromhex('0000000000000000000000000000000200000000000000040000000000000006') # 32B key
print(f'key = {hexStr(key)}')
cipher = AES.new(key, AES.MODE_CBC, iv)

#data = bytes.fromhex('0f0e0d0c0b0a09080706050403020100') # 128b/16B block
data = bytes.fromhex('000102030405060708090a0b0c0d0e0f') # 128b/16B block
print(f'data = {hexStr(data)}')

ciphertext = cipher.encrypt(data)
print(f'ciphertext = {hexStr(ciphertext)}')

# ciphertext = cipher.encrypt(data)
# print(f'ciphertext = {hexStr(ciphertext)}')
