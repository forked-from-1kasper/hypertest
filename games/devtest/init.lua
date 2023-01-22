local fennel = require("Fennel.fennel")

table.insert(package.loaders, fennel.searcher)
debug.traceback = fennel.traceback

fennel.dofile(hypertest.dirname .. "/init.fnl")