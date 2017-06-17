# Qt kernel library base module

HEADERS +=  \
	global/qglobal.h \
        global/qoperatingsystemversion.h \
        global/qoperatingsystemversion_p.h \
        global/qsystemdetection.h \
        global/qcompilerdetection.h \
        global/qprocessordetection.h \
	global/qnamespace.h \
        global/qendian.h \
        global/qendian_p.h \
        global/qnumeric_p.h \
        global/qnumeric.h \
        global/qfloat16_p.h \
        global/qfloat16.h \
        global/qglobalstatic.h \
        global/qlibraryinfo.h \
        global/qlogging.h \
        global/qtypeinfo.h \
        global/qsysinfo.h \
        global/qisenum.h \
        global/qtypetraits.h \
        global/qflags.h \
        global/qrandom.h \
        global/qrandom_p.h \
        global/qhooks_p.h \
        global/qversiontagging.h

SOURCES += \
        global/archdetect.cpp \
	global/qglobal.cpp \
        global/qlibraryinfo.cpp \
	global/qmalloc.cpp \
        global/qnumeric.cpp \
        global/qfloat16.cpp \
        global/qoperatingsystemversion.cpp \
        global/qlogging.cpp \
        global/qrandom.cpp \
        global/qhooks.cpp

VERSIONTAGGING_SOURCES = global/qversiontagging.cpp

darwin: SOURCES += global/qoperatingsystemversion_darwin.mm
win32: SOURCES += global/qoperatingsystemversion_win.cpp

# qlibraryinfo.cpp includes qconfig.cpp
INCLUDEPATH += $$QT_BUILD_TREE/src/corelib/global

# Only used on platforms with CONFIG += precompile_header
PRECOMPILED_HEADER = global/qt_pch.h

# qlogging.cpp uses backtrace(3), which is in a separate library on the BSDs.
LIBS_PRIVATE += $$QMAKE_LIBS_EXECINFO

if(linux*|hurd*):!cross_compile:!static:!*-armcc* {
   QMAKE_LFLAGS += -Wl,-e,qt_core_boilerplate
   prog=$$quote(if (/program interpreter: (.*)]/) { print $1; })
   DEFINES += ELF_INTERPRETER=\\\"$$system(LC_ALL=C readelf -l /bin/ls | perl -n -e \'$$prog\')\\\"
}

linux:!static {
    precompile_header {
        # we'll get an error if we just use SOURCES +=
        no_pch_assembler.commands = $$QMAKE_CC -c $(CFLAGS) $(INCPATH) ${QMAKE_FILE_IN} -o ${QMAKE_FILE_OUT}
        no_pch_assembler.dependency_type = TYPE_C
        no_pch_assembler.output = ${QMAKE_VAR_OBJECTS_DIR}${QMAKE_FILE_BASE}$${first(QMAKE_EXT_OBJ)}
        no_pch_assembler.input = NO_PCH_ASM
        no_pch_assembler.name = compiling[no_pch] ${QMAKE_FILE_IN}
        silent: no_pch_assembler.commands = @echo compiling[no_pch] ${QMAKE_FILE_IN} && $$no_pch_assembler.commands
        QMAKE_EXTRA_COMPILERS += no_pch_assembler
        NO_PCH_ASM += global/minimum-linux.S
    } else {
        SOURCES += global/minimum-linux.S
    }
}

qtConfig(slog2): \
    LIBS_PRIVATE += -lslog2

qtConfig(journald): \
    QMAKE_USE_PRIVATE += journald

gcc:ltcg {
    versiontagging_compiler.commands = $$QMAKE_CXX -c $(CXXFLAGS) $(INCPATH)

    # Disable LTO, as the symbols disappear somehow under GCC
    versiontagging_compiler.commands += -fno-lto

    versiontagging_compiler.commands += -o ${QMAKE_FILE_OUT} ${QMAKE_FILE_IN}
    versiontagging_compiler.dependency_type = TYPE_C
    versiontagging_compiler.output = ${QMAKE_VAR_OBJECTS_DIR}${QMAKE_FILE_BASE}$${first(QMAKE_EXT_OBJ)}
    versiontagging_compiler.input = VERSIONTAGGING_SOURCES
    versiontagging_compiler.variable_out = OBJECTS
    versiontagging_compiler.name = compiling[versiontagging] ${QMAKE_FILE_IN}
    silent: versiontagging_compiler.commands = @echo compiling[versiontagging] ${QMAKE_FILE_IN} && $$versiontagging_compiler.commands
    QMAKE_EXTRA_COMPILERS += versiontagging_compiler
} else {
    SOURCES += $$VERSIONTAGGING_SOURCES
}

# On AARCH64 the fp16 extension is mandatory, so we don't need the conversion tables.
!contains(QT_ARCH, "arm64") {
    QMAKE_QFLOAT16_TABLES_GENERATE = global/qfloat16.h

    qtPrepareTool(QMAKE_QFLOAT16_TABLES, qfloat16-tables)

    qfloat16_tables.commands = $$QMAKE_QFLOAT16_TABLES ${QMAKE_FILE_OUT}
    qfloat16_tables.output = global/qfloat16tables.cpp
    qfloat16_tables.depends = $$QMAKE_QFLOAT16_TABLES
    qfloat16_tables.input = QMAKE_QFLOAT16_TABLES_GENERATE
    qfloat16_tables.variable_out = SOURCES
    QMAKE_EXTRA_COMPILERS += qfloat16_tables
}
