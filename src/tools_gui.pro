TEMPLATE = subdirs

#Library targets and depdendencies
SUBDIRS = cppCORE\
        cppXML \
        cppNGS \
        cppGUI \
        cppNGSD \
        cppPLOTS \
        cppVISUAL

cppXML.depends = cppCORE
cppPLOTS.depends = cppCORE
cppNGS.depends = cppXML
cppGUI.depends = cppXML
cppGUI.depends = cppPLOTS
cppNGSD.depends = cppNGS
cppVISUAL.depends = cppNGS cppGUI

SUBDIRS += GSvar
GSvar.depends = cppNGSD cppVISUAL cppPLOTS

SUBDIRS += MVHub
MVHub.depends = cppNGSD cppVISUAL
