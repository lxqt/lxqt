include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${Qt5Widgets_INCLUDE_DIRS})

add_definitions(${Qt5Widgets_DEFINITIONS})

OPTION (QKSW_SHARED "Use QKeysequenseWidget as shared library" OFF)

set (QKSW_SRC
  src/qkeysequencewidget.cpp)

set (QKSW_HDR
  src/qkeysequencewidget.h
  src/qkeysequencewidget_p.h)

set (QKSW_QRC
  qkeysequencewidget.qrc)

qt5_add_resources(QKSW_QRC ${QKSW_QRC})

if(QKSW_SHARED)
    add_definitions(-DIS_SHARED="true")
	add_library(qkeysequencewidget SHARED ${QKSW_SRC} ${QKSW_QRC})
	set_property(TARGET qkeysequencewidget PROPERTY SOVERSION 1.0.0)
	INSTALL (TARGETS qkeysequencewidget DESTINATION ${SG_LIBDIR})
else(QKSW_SHARED)
	add_library(qkeysequencewidget STATIC ${QKSW_SRC} ${QKSW_QRC})
endif(QKSW_SHARED)

target_link_libraries(qkeysequencewidget Qt5::Widgets)
