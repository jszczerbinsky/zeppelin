from engine import Engine

import fenchecks
import unmakemovechecks

engine = Engine('../src/a.out', '../src/')

fenchecks.run(engine);
unmakemovechecks.run(engine);


