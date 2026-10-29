from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext

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


class BuildExt(build_ext):
    """Add gcc/clang flags, which MSVC does not take.

    -fno-strict-aliasing: the hash code reads byte buffers through uint32_t and
    uint64_t pointers. With strict aliasing, gcc at -O2 and above miscompiles
    scrypt-N, Lyra2REv2 and Lyra2REv3 (wrong, even varying, hashes). MSVC does
    not optimize on aliasing, which is why the Windows wheels were right.
    The code has no SIMD intrinsics, so there is no -msse3 (which breaks arm64).
    """

    def build_extensions(self):
        if self.compiler.compiler_type != 'msvc':
            for ext in self.extensions:
                ext.extra_compile_args = ['-O3', '-fno-strict-aliasing'] + getattr(ext, 'unix_compile_args', [])
        super().build_extensions()


def extension(name, module_sources, unix_compile_args=()):
    ext = Extension(name, sources=vertcoinsources + module_sources, include_dirs=vertcoinincludes)
    ext.unix_compile_args = list(unix_compile_args)
    return ext


with open('README.md', encoding='utf-8') as f:
    long_description = f.read()

setup(name='vertcoinhash',
      version='1.1.0',
      author_email='vertion@protonmail.com',
      author='vertion',
      url='https://github.com/vertcoin-project/vertcoinhash-python',
      description='Bindings for proof of work used by Vertcoin',
      long_description=long_description,
      long_description_content_type='text/markdown',
      license='MIT',
      python_requires='>=3.8',
      classifiers=[
          'Programming Language :: Python :: 3',
          'Programming Language :: C',
          'License :: OSI Approved :: MIT License',
      ],
      cmdclass={'build_ext': BuildExt},
      ext_modules=[
          extension('verthash', ['verthashmodule.c', 'verthash_datfile.c'], ['-std=c99']),
          extension('lyra2re3_hash', ['lyra2re3module.c']),
          extension('lyra2re2_hash', ['lyra2re2module.c']),
          extension('lyra2re_hash', ['lyra2remodule.c']),
          extension('vtc_scrypt_new', ['scryptmodule.c']),
      ])
