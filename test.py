import lyra2re_hash
import weakref
import binascii
import StringIO

from binascii import unhexlify, hexlify

teststart = '700000005d385ba114d079971b29a9418fd0549e7d68a95c7f168621a314201000000000578586d149fd07b22f3a8a347c516de7052f034d2b76ff68e0d6ecff9b77a45489e3fd511732011df0731000';
testbin = unhexlify(teststart)
hash_bin = lyra2re_hash.getPoWHash(testbin)

print hexlify(hash_bin)