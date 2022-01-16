import sys

import vtc_scrypt_new
import lyra2re_hash
import lyra2re2_hash
import lyra2re3_hash
# import verthash

teststart = '000000203a297b4b7685170d7644b43e5a6056234cc2414edde454a87580e1967d14c1078c13ea916117b0608732f3f65c2e03b81322efc0a62bcee77d8a9371261970a58a5a715da80e031b02560ad8'

# with open('verthash.dat', 'rb') as f:
#     datfile = f.read()

if sys.version_info[0] < 3:
    testbin = teststart.decode('hex')
else:
    testbin = bytes.fromhex(teststart)


hash_bin_scryptn = vtc_scrypt_new.getPoWHash(testbin)
hash_bin_re = lyra2re_hash.getPoWHash(testbin)
hash_bin_re2 = lyra2re2_hash.getPoWHash(testbin)
hash_bin_re3 = lyra2re3_hash.getPoWHash(testbin)
# hash_bin_verthash = verthash.getPoWHash(testbin, datfile)

if sys.version_info[0] < 3:
    print("scrypt-n", hash_bin_scryptn.encode('hex'))
    print("lyra2re", hash_bin_re.encode('hex'))
    print("lyra2re2", hash_bin_re2.encode('hex'))
    print("lyra2re3", hash_bin_re3.encode('hex'))
    # print("verthash", hash_bin_verthash.encode('hex'))

else:
    print("scrypt-n", hash_bin_scryptn.hex())
    print("lyra2re", hash_bin_re.hex())
    print("lyra2re2", hash_bin_re2.hex())
    print("lyra2re3", hash_bin_re3.hex())
    # print("verthash", hash_bin_verthash.hex())
