#Project common
include(pebbleqt.pri)

TEMPLATE = app
TARGET = Pebble
#QT5 requires explicit add of Widgets to add QtWidgets which was in QtGui in earlier releases
QT += widgets core gui
#For QWebEngineView
QT += webenginewidgets

#See readme.md for build instructions and pre-requisites

#Enable this to look at config to debug conditionals. For example: debug and release both show up sometimes
#message($$CONFIG)

#So we don't have to specify paths everywhere
INCLUDEPATH += ../pebblelib
DEPENDPATH += ../pebblelib

#Conditional MUST match the Build Configuration name, Debug or Release or SomeCustomName
macx {

	#After much agony, finally figured out that this icon only appears in release, NOT debug builds
	#There doesn't appear to be any need to use setWindowIcon() either, just this in make file
	#But Pebble.icns must be specified in resources.qrc
	#Use Sketch to edit Pebble Icons
	#Use http://iconverticons.com/online/ to convert to icns file
	ICON = Resources/Pebble.icns


        #This technique allows us to execute system commands before any other targets are built
        #This happens on every build, not just when qmake is executed

        #Note 2 undocumented hacks.
        #   $${LITERAL_HASH} to escape #, \# doesn't work
        #   \$\$ to escape $, trial and error but this works
        #1st echo creates .h file with '>', subsequent echo's append with '>>'
        #Every commmand must end with ';' to separate it from following commands
        myPreTarget.commands += echo \"//Created by pebbleqt.pro every build \">buildinfo.h;
        myPreTarget.commands += echo \"$${LITERAL_HASH}define PEBBLE_QT  \"\\\"$${QT_VERSION}\\\">>buildinfo.h;
        myPreTarget.commands += echo \"$${LITERAL_HASH}define PEBBLE_DATE \"\\\"\$\$(date)\\\">>buildinfo.h;
        #Not sure why we need single quotes around 'git rev-list --count HEAD', but we do
        #Note the -C $${PWD} argument which tells git where the repository is.
        #Commands are executed in the 'build...' directory otherwise
        myPreTarget.commands += echo \"$${LITERAL_HASH}define PEBBLE_VERSION \"\\\"\$\$('git -C $${PWD} rev-list --count HEAD')\\\">>buildinfo.h;
        myPreTarget.depends = FORCE #Rebuild target every time
        QMAKE_EXTRA_TARGETS += myPreTarget #Defines this as a target for QMAKE
        PRE_TARGETDEPS += myPreTarget #Tells QMake to build this target before any other targets


	#Turn off warnings for unused variables so we can focus on real warnings
	#We completely replace this variable with our own warning flags in the order we want them
	#This turns on all warnings, then turns off selected ones
	QMAKE_CXXFLAGS_WARN_ON = "-Wall"
	QMAKE_CXXFLAGS_WARN_ON += "-Wno-unused-variable"

	#This will stop the creation of a Mac package (default) and just create a unix bin if we need to for testing
	#CONFIG-=app_bundle

	#dylib
	#static lib
        LIBS += -framework CoreServices

	LIBS += -L$${PWD}/../pebblelib/$${LIB_DIR} -lpebblelib.1

        #QT libraries expected by PebbleLib.  Adding here makes sure macdeployqt copies them all to Frameworks
        LIBS += -framework QtMultimedia
        LIBS += -framework QtNetwork

        #Only include if USE_FFTACCELERATE is defined
        LIBS += -framework Accelerate

	#INSTALLS is called when we manually make -install or add it to the Qt project build steps
        pebbleData.files += eibireadme.txt
        pebbleData.files += eibi.csv
        pebbleData.files += bands.csv
        pebbleData.files += memory.csv
        pebbleData.files += ../pebblelib/gpl.html
        pebbleData.files += ../readme.html
        pebbleData.files += ../gpl-3.0-standalone.html
        pebbleData.files += beep-07.wav
        pebbleData.files += "Pebble Buttons.pdf"
        pebbleData.files += Pebble.shuttleSettings
        pebbleData.files += unload_osx_ftdi_driver.scpt

	#We want this to be whatever the release or debug directory is or added to bundle
	#This will copy the files into the app bundle, same place ini files go
	#pebbleData.path = $${DESTDIR}/Pebble.app/Contents/MacOS
	#QMake will create folder in path if they don't exist
	pebbleData.path = $${DESTDIR}/PebbleData
	#message($${pebbleData.path})
	INSTALLS += pebbleData

	#To find all dependent dylib: otool -L ./Pebble.app/contents/macos/pebble >> dylib.txt
	#1st arg is the path and name of dylib as shown from otool ouput
	#2nd arg starts with @executable_path which expands to (path to bundle)/pebble.app/contents/macos
	#Up one dir to Frameworks which is where we copy .dylib files
	#3rd arg is the actual executable to patch
        # 1. Let macdeployqt handle all the Pebble fixups and move dylibs to Framewords and plugins to Plugins
        # 2. Set all PebbleLib and Pebble plugins to use @rpath
        # 3. Set rpath to Pebble.app/Contents/Frameworks
        # 4. Set rpath to ??? for SDR Garage?
        #This anchors @rpath references to use bundle's Frameworks directory
        QMAKE_LFLAGS += -rpath @executable_path/../Frameworks

        #Uncomment for debugging dependencies
        #pebblefix.commands += otool -L $${DESTDIR}/Pebble.app/contents/macos/pebble > temp.txt;
        #Use install_name_tool because macdeployqt occasionally can't find pebblelib
        #6/18/16: QT 5.6.1 stopped accepting libpebblelib.1.dylib, which was an alias.  Switched to full dylib name
        pebblefix.commands += install_name_tool -change libpebblelib.1.dylib @executable_path/../Frameworks/libpebblelib.1.0.0.dylib $${DESTDIR}/Pebble.app/contents/macos/pebble;

        #macdeployqt should pick up any fixups we missed and copy QT libraries to app Frameworks directory
        #pebblefix.commands += dsymutil $${DESTDIR}/Pebble.app/Contents/MacOS/Pebble;
        #1/26/16: macdeployqt has been strippping debug symbol for some time, ie no breakpoints
        #Some QT update starting running strip at the the end.  Adding -no-strip arg gives us bp back!
        #Use -verbose for debugging macdeployqt problems
        #-verbose= [0]No output [1]Errors and Warnings [2]Normal [3]Debug
        pebblefix.commands += macdeployqt $${DESTDIR}/Pebble.app -no-strip -verbose=0;

        #Uncomment for debugging dependencies
        #pebblefix.commands += otool -L $${DESTDIR}/Pebble.app/contents/macos/pebble >> temp.txt;
        pebblefix.path += $${DESTDIR}
        INSTALLS += pebblefix

        #Copy pebblelib and it's dependencies
        #Keep in sync with pebblelib.pro
        plib.files += $${PWD}/../D2XX/bin/10.5-10.7/libftd2xx.1.2.2.dylib
        #6/18/16: QT 5.6.1 stopped accepting libpebblelib.1.dylib, which was an alias.  Switched to full dylib name
        plib.files += $${PWD}/../pebblelib/$${LIB_DIR}/libpebblelib.1.0.0.dylib
        plib.path = $${DESTDIR}/Pebble.app/Contents/Frameworks
        INSTALLS += plib

}

win32 {
	CONFIG(debug, debug|release) {
		DESTDIR = ../WinDebug
	} else {
		DESTDIR = ../WinRelease
	}

	OBJECTS_DIR = $$PWD/WinO
	#Locataion for MOC files
	MOC_DIR = $$PWD/WinMoc

	LIBS +=	../PebbleII/PortAudio.lib
	LIBS +=	../fftw3/libfftw3f-3.lib
	#only needed for FCD HID Windows support
	#2/3/12 Use mingw libsetupapi instead of Microsoft Lib
	LIBS += libsetupapi
	LIBS += C:/Program Files/Microsoft SDKs/Windows/v6.0A/Lib/setupapi.lib

	#Win only source files
	SOURCES +=

	CONFIG(release, debug|release) {
		DESTDIR = ../WinRelease
		#Relative to source dir
		installfiles.path = ../Release
		installfiles.files = \
			../fftw3/libfftw3f-3.dll \
			#PennyMerge versions of file
			../HPSDR/ozy_janus.rbf \
			../HPSDR/ozyfw-sdr1k.hex \
			$$QTDIR/mingw/bin/libgcc_s_dw2-1.dll \
			$$QTDIR/mingw/bin/mingwm10.dll \
		#WARNING: Version of qtcore4.dll in $$QTDIR/bin is missing qt_message_output!
		#Make sure we're always getting the version as built
				$$QTDIR/bin/qtcore4.dll \
				$$QTDIR/bin/qtgui4.dll \
				$$QTDIR/bin/qtmultimedia4.dll \
			../"Release DLLs"/PortAudio.dll

		#target.path = ../Release
		INSTALLS += target installfiles

	}

	CONFIG(debug, debug|release) {
		DESTDIR = ../WinDebug
		#Relative to source dir
		installfiles.path = ../Debug
		installfiles.files = \
			../fftw3/libfftw3f-3.dll \
			$$QTDIR/mingw/bin/libgcc_s_dw2-1.dll \
			$$QTDIR/mingw/bin/mingwm10.dll \
		#PennyMerge versions of file
			../HPSDR/ozy_janus.rbf \
			../HPSDR/ozyfw-sdr1k.hex \
		#WARNING: Version of qtcore4.dll in $$QTDIR/bin is missing qt_message_output!
		#Make sure we're always getting the version as built
				$$QTDIR/bin/qtcored4.dll \
				$$QTDIR/bin/qtguid4.dll \
				$$QTDIR/bin/qtmultimediad4.dll \
			../"Release DLLs"/PortAudio.dll

		target.path = ../Debug
		INSTALLS += target installfiles

		#INSTALLS is called when we manually make -install or add it to the Qt project build steps
		pebbleData.files = eibireadme.txt eibi.csv bands.csv memory.csv gpl.h readme.md

		#We want this to be whatever the release or debug directory is or added to bundle
		pebbleData.path = $$DESTDIR
		INSTALLS += pebbleData

	}
}


#Build instructions for build.h using build.tmpl
#doesn't seem to work
#buildh.target = build.h
#buildh.depends = build.tmpl
#buildh.commands = SubWCRev build.tmpl build.h
#QMAKE_EXTRA_TARGETS += buildh

#Undocumented(?) Note brackets not paren
#${QMAKE_FILE_BASE) = build for build.h
#${QMAKE_FILE_PATH} = ../PebbleII

PREPROCESS = build.tmpl
preprocess.name = SubversionBuildNumberUpdate
preprocess.input = PREPROCESS
preprocess.output = ../PebbleII/build.h #What make looks at to see if build is needed
#preprocess.depends = ../PebbleII/_svn #Datetime modified changes with every checkin, so this will trigger update
preprocess.CONFIG = no_link #This is required to stop make from trying to compile output
#preprocess.commands = SubWCRev ../PebbleII ../PebbleII/build.tmpl ../PebbleII/build.h
#QMAKE_EXTRA_COMPILERS += preprocess

#QMAKE_PRE_LINK += copy $(DEPENDPATH)/presets.csv $(DESTDIR)
#QMAKE_POST_LINK

#CONFIG += qt sse

#This is how we can add custom defines and options based on release vs debug builds
#See release{} and debug{} install section for an alternate way
#CONFIG(debug, debug|release): DEFINES += DEBUG
#CONFIG(release, debug|release): DEFINES += RELEASE


#Shouldn't have to use this, but CONFIG not working.  Uncomment for SSE testing
#QMAKE_CXXFLAGS += \
	#-msse3 -mssse3 -mfpmath=sse

#QMAKE_LFLAGS += -static-libgcc

OTHER_FILES += \
    firmware.txt \
    build.tmpl \
    bands.csv \
    eibi.csv \
    eibireadme.txt \
    memory.csv \
	frequencylist-santacruz.csv \
    pebble.qss \
    codingstyle.txt \
    pebbleqt.pri

HEADERS += \
    spectrumwidget.h \
    smeterwidget.h \
	signalstrength.h \
	signalspectrum.h \
        processstep.h \
	settings.h \
	receiverwidget.h \
	receiver.h \
    presets.h \
    pebbleii.h \
	noisefilter.h \
	noiseblanker.h \
	demod.h \
	#build.h \
	agc.h \
	iqbalance.h \
    global.h \
	demod/demod_wfm.h \
    demod/rbdsconstants.h \
    demod/rdsdecode.h \
    devices/extio.h \
    bargraphmeter.h \
    testbench.h \
    demod/demod_am.h \
    demod/demod_sam.h \
    demod/demod_nfm.h \
    plugins.h \
    sdroptions.h \
    dcremoval.h \
    bandpassfilter.h \
    doubleslider.h

SOURCES += \
    spectrumwidget.cpp \
    smeterwidget.cpp \
	signalstrength.cpp \
	signalspectrum.cpp \
        processstep.cpp \
	settings.cpp \
	receiverwidget.cpp \
	receiver.cpp \
    presets.cpp \
    pebbleii.cpp \
	noisefilter.cpp \
	noiseblanker.cpp \
    main.cpp \
	demod.cpp \
	agc.cpp \
    iqbalance.cpp \
    global.cpp \
	demod/demod_wfm.cpp \
    demod/rdsdecode.cpp \
    bargraphmeter.cpp \
    testbench.cpp \
    demod/demod_am.cpp \
    demod/demod_sam.cpp \
    demod/demod_nfm.cpp \
    plugins.cpp \
    sdroptions.cpp \
    dcremoval.cpp \
    bandpassfilter.cpp \
    doubleslider.cpp

FORMS += \
    spectrumwidget.ui \
    smeterwidget.ui \
    receiverwidget.ui \
    pebbleii.ui \
    iqbalanceoptions.ui \
    sdr.ui \
    directinput.ui \
    bargraphmeter.ui \
    testbench.ui \
    data-band.ui

RESOURCES += \
    resources.qrc

DISTFILES += \
    todo.txt \
    frequencylist-raleigh.csv
