#
# Autoconfiguration files for proxytest (a MiX proxy)
# Konrad Rieck <kr@r0q.cx>
#

AUTOMAKE_OPTIONS = foreign

noinst_LIBRARIES = libxml.a
libxml_a_SOURCES = DOM_Output.cpp DOM_Output.hpp

if USE_MINIXML
libxml_a_SOURCES += dom/mxml/mxmlDOM.hpp dom/mxml/mxmlDOMDocument.cpp dom/mxml/mxmlXercesDOMParser.cpp dom/mxml/mxmlDOMNode.cpp dom/mxml/mxmlDOMImplementation.cpp
endif

includes = @INCLUDES@

