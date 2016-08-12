#Makefile at top of application tree
TOP = .
include $(TOP)/configure/CONFIG

DIRS := configure csmApp cvtRecordApp documentation

cvtRecordApp_DEPEND_DIRS += csmApp
documentation_DEPEND_DIRS += csmApp

include $(TOP)/configure/RULES_TOP

upload:
	darcs push wwwcsr@www-csr.bessy.de:www/control/SoftDist/csm/repo/csm
	rsync -r html/ wwwcsr@www-csr.bessy.de:www/control/SoftDist/csm/
