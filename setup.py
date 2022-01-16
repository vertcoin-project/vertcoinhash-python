from setuptools import setup, Extension

vertcoinsources = [
    'scrypt.c',
    'Lyra2RE.c',
    'Sponge.c',
    'Lyra2.c',
    'sha3/blake.c',
    'sha3/groestl.c',
    'sha3/keccak.c',
    'sha3/cubehash.c',
    'sha3/bmw.c',
    'sha3/skein.c',
    'h2.c',
    'tiny_sha3/sha3.c'
]

vertcoinincludes = [
    '.',
    './sha3',
    './tiny_sha3'
]


vtc_scrypt_hash_module = Extension('vtc_scrypt_new',
                                   sources = vertcoinsources + ['scryptmodule.c'],
                                   extra_compile_args=['-O3', '-msse3'],
                                   include_dirs=vertcoinincludes)

lyra2re_hash_module = Extension('lyra2re_hash',
                                sources = vertcoinsources + ['lyra2remodule.c'],
                                include_dirs=vertcoinincludes)

lyra2re2_hash_module = Extension('lyra2re2_hash',
                                 sources = vertcoinsources + ['lyra2re2module.c'],
                                 include_dirs=vertcoinincludes)

lyra2re3_hash_module = Extension('lyra2re3_hash',
                                 sources = vertcoinsources + ['lyra2re3module.c'],
                                 include_dirs=vertcoinincludes)

verthash_module = Extension('verthash',
                            sources = vertcoinsources + ['verthashmodule.c'],
                            extra_compile_args=['-std=c99'],
                            include_dirs=vertcoinincludes)


setup (name = 'vertcoinhash',
       version = '1.0.0',
       author_email = 'vertion@protonmail.com',
       author = 'vertion',
       url = 'https://github.com/vertcoin-project/vertcoinhash-python',
       description = 'Bindings for proof of work used by Vertcoin',
       ext_modules = [verthash_module, lyra2re3_hash_module, lyra2re2_hash_module, lyra2re_hash_module, vtc_scrypt_hash_module])
