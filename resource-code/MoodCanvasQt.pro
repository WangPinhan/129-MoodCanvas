QT += widgets network concurrent

CONFIG += c++17
CONFIG -= app_bundle

TARGET = MoodCanvasQt
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    moodentry.cpp \
    llmclient.cpp \
    datastore.cpp \
    plantwidget.cpp \
    moodcanvaswidget.cpp \
    resultdialog.cpp \
    thememanager.cpp \
    timelinewindow.cpp \
    weeklyreportwindow.cpp \
    weeklyscenerywidget.cpp \
    reportartutils.cpp \
    arttitlelabel.cpp \
    toolboxdialog.cpp \
    settingsdialog.cpp \
    userservice.cpp \
    logindialog.cpp \
    loginbackdropwidget.cpp \
    cloudapiclient.cpp \
    cloudtreehole.cpp \
    treeholeemotion.cpp

HEADERS += \
    mainwindow.h \
    moodentry.h \
    llmconfig.h \
    llmclient.h \
    plantwidget.h \
    datastore.h \
    moodcanvaswidget.h \
    resultdialog.h \
    thememanager.h \
    timelinewindow.h \
    weeklyreportwindow.h \
    weeklyscenerywidget.h \
    reportartutils.h \
    arttitlelabel.h \
    toolboxdialog.h \
    settingsdialog.h \
    userprofile.h \
    userservice.h \
    logindialog.h \
    loginbackdropwidget.h \
    cloudconfig.h \
    cloudapiclient.h \
    cloudtreehole.h \
    treeholeemotion.h

FORMS += \
    mainwindow.ui

RESOURCES += \
    resources.qrc
