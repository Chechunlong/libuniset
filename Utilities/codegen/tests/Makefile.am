noinst_PROGRAMS = test test2
#test2

test_LDADD 		= $(top_builddir)/lib/libUniSet.la
test_CXXFLAGS	= -I$(top_builddir)/include
test_SOURCES 	= TestGen_SK.cc TestGen.cc TestGen-main.cc

test2_LDADD 	= $(top_builddir)/lib/libUniSet.la
test2_CXXFLAGS	= -I$(top_builddir)/include
test2_SOURCES 	= TestGenAlone_SK.cc TestGenAlone.cc TestGenAlone-main.cc

GENERATED=TestGen_SK.h TestGen_SK.cc TestGen-main.cc
GENERATED2=TestGenAlone_SK.h TestGenAlone_SK.cc TestGenAlone-main.cc
GENUOBJ=UObject_SK.cc UObject_SK.h

$(GENERATED): ../@PACKAGE@-codegen testgen.src.xml ../*.xsl
	../@PACKAGE@-codegen -l $(top_builddir)/Utilities/codegen --local-include -n TestGen testgen.src.xml

$(GENERATED2): ../@PACKAGE@-codegen testgen-alone.src.xml ../*.xsl
	../@PACKAGE@-codegen -l $(top_builddir)/Utilities/codegen --local-include --alone -n TestGenAlone testgen-alone.src.xml

$(GENUOBJ): ../@PACKAGE@-codegen uobject.src.xml ../*.xsl
	../@PACKAGE@-codegen -l $(top_builddir)/Utilities/codegen -n UObject --no-main uobject.src.xml

clean-local:
	rm -rf $(GENERATED) $(GENERATED2) $(GENUOBJ)

all-local: $(GENERATED) $(GENERATED2) $(GENUOBJ)
	
	
