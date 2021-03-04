TEMPLATE = subdirs
SUBDIRS =   cppCORE \
            cppCORE-TEST \
            cppXML \
            cppNGS \
            cppNGS-TEST \
            cppNGSD \
            cppNGSD-TEST

cppCORE-TEST.depends = cppCORE
cppXML.depends = cppCORE
cppNGS.depends = cppXML
cppNGS-TEST.depends = cppNGS
cppNGSD.depends = cppNGS
cppNGSD-TEST.depends = cppNGSD
