TOP = ..
include $(TOP)/configure/CONFIG_APP
#-----------------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

DBDINC = cvtRecord menuCvtMethod menuCvtInitState

LIB_SRCS = cvtRecord.c

LIBRARY_IOC += cvtRecord
cvtRecord_LIBS += csmbase $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES
#-----------------------------------------------
#  ADD RULES AFTER THIS LINE
