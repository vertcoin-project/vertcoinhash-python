"""Trial hashing of a test header with all five proofs of work; see ci-test/test.py."""
import os
import runpy

runpy.run_path(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'ci-test', 'test.py'), run_name='__main__')
