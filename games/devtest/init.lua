local fennel = require("fennel.fennel")

table.insert(package.loaders, fennel.searcher)
debug.traceback = fennel.traceback

fennel.dofile(hypertest.dirname .. "/init.fnl")
