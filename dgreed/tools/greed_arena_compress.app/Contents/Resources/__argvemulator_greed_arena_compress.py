import argvemulator, os

argvemulator.ArgvCollector().mainloop()
execfile(os.path.join(os.path.split(__file__)[0], "greed_arena_compress.py"))
