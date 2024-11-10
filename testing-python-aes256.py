#!/usr/bin/env python3

from Crypto.Cipher import AES

def hexStr(s):
    return ":".join("{:02x}".format(c) for c in s)

# 8b byte
def reversebybyte(string):
    return ''.join(reversed([string[i:i+2] for i in range(0, len(string), 2)]))

# 64b word
def reversebyword(string):
    return ''.join(reversed([string[i:i+16] for i in range(0, len(string), 16)]))

ivstr   = 'ad23511a2d847eedf88452796d8aca9c'
keystr  = '00000000000000000000000000000000'
datastr = '97bb9d3b99067e1718d57c1a51268d00'

combos = []
combos.append(((ivstr), (keystr), (datastr)))

for combo in combos:
    iv   = bytes.fromhex(combo[0])
    key  = bytes.fromhex(combo[1])
    data = bytes.fromhex(combo[2])
    print(f'iv = {hexStr(iv)}')
    print(f'key = {hexStr(key)}')
    print(f'data = {hexStr(data)}')
    cipher = AES.new(key, AES.MODE_CBC, iv)
    ciphertext = cipher.decrypt(data)
    print(f'ciphertext = {hexStr(ciphertext)}')
