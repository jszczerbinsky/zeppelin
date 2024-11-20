import sys

from engine import Engine

import fenchecks
import unmakemovechecks

engine = Engine(sys.argv[1])

result = True

print()
print('____________________________________________________________ RUNNING TESTS ____________________________________________________________')
result &= fenchecks.run(engine);
result &= unmakemovechecks.run(engine);
print('_______________________________________________________________________________________________________________________________________')
print()

if result:
    print('All tests succeded')
    print()
else:
    print('Some tests failed')
    print()
    exit(1)
