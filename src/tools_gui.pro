TEMPLATE = subdirs

#Library targets and depdendencies
SUBDIRS = cppCORE\
        GSvar-TEST \
        cppXML \
        cppNGS \
        cppGUI \
        cppNGSD \
        cppVISUAL

cppXML.depends = cppCORE
cppNGS.depends = cppXML
cppGUI.depends = cppXML
cppNGSD.depends = cppNGS
cppVISUAL.depends = cppNGS cppGUI

SUBDIRS += GSvar
GSvar.depends = cppNGSD cppVISUAL

SUBDIRS += MVHub
MVHub.depends = cppNGSD cppVISUAL
