TEMPLATE = subdirs
SUBDIRS =   cppCORE \
            cppCORE-TEST \
            cppXML \
            cppXML-TEST \
            cppNGS \
            cppNGS-TEST \
            cppNGSD \
            cppNGSD-TEST \
            cppREST \
            cppREST-TEST \
            cppPlots

cppCORE-TEST.depends = cppCORE

cppXML.depends = cppCORE
cppXML-TEST.depends = cppXML

cppNGS.depends = cppXML
cppNGS-TEST.depends = cppNGS

cppNGSD.depends = cppNGS
cppNGSD-TEST.depends = cppNGSD

cppREST.depends = cppCORE
cppREST.depends = cppXML
cppREST.depends = cppNGS
cppREST.depends = cppNGSD
cppREST-TEST.depends = cppREST
cppPlots.depends = cppCORE
