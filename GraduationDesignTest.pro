#-------------------------------------------------
#
# Project created by QtCreator 2016-02-24T09:28:54
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = GraduationDesignTest
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main.cpp \
    kimagecvt.cpp \
    kpicinfo.cpp \
    common.cpp \
    kfeaturelbp.cpp \
    kprogressbar.cpp \
    ksplitimage.cpp \
    kcalpmk.cpp

#edited for openCV3.0
#库引入方�
INCLUDEPATH += E:/QTPrj/QtOpenCV3/include
LIBS += -LE:/QTPrj/QtOpenCV3/lib\
-llibopencv_calib3d300\
-llibopencv_core300\
-llibopencv_features2d300\
-llibopencv_flann300\
-llibopencv_highgui300\
-llibopencv_imgcodecs300\
-llibopencv_imgproc300\
-llibopencv_ml300\
-llibopencv_objdetect300\
-llibopencv_photo300\
-llibopencv_shape300\
-llibopencv_stitching300\
-llibopencv_superres300\
-lopencv_hal300
#库引入方�
#INCLUDEPATH += E:/QTPrj/QtOpenCV3/include
#LIBS += -L E:/QTPrj/QtOpenCV3/lib/libopencv_*.a

#edited for boost1.6.0
#static library
DEFINES += BOOST_THREAD_USE_LIB

INCLUDEPATH += E:/boost_1_60_0/QtLib/include
LIBS += -LE:/boost_1_60_0/QtLib/lib\
-lboost_atomic-mgw49-mt-s-1_60\
-lboost_atomic-mgw49-mt-sd-1_60\
-lboost_chrono-mgw49-mt-s-1_60\
-lboost_chrono-mgw49-mt-sd-1_60\
-lboost_container-mgw49-mt-s-1_60\
-lboost_container-mgw49-mt-sd-1_60\
-lboost_context-mgw49-mt-s-1_60\
-lboost_context-mgw49-mt-sd-1_60\
-lboost_coroutine-mgw49-mt-s-1_60\
-lboost_coroutine-mgw49-mt-sd-1_60\
-lboost_date_time-mgw49-mt-s-1_60\
-lboost_date_time-mgw49-mt-sd-1_60\
-lboost_exception-mgw49-mt-s-1_60\
-lboost_exception-mgw49-mt-sd-1_60\
-lboost_filesystem-mgw49-mt-s-1_60\
-lboost_filesystem-mgw49-mt-sd-1_60\
-lboost_iostreams-mgw49-mt-s-1_60\
-lboost_iostreams-mgw49-mt-sd-1_60\
-lboost_locale-mgw49-mt-s-1_60\
-lboost_locale-mgw49-mt-sd-1_60\
-lboost_log-mgw49-mt-s-1_60\
-lboost_log-mgw49-mt-sd-1_60\
-lboost_log_setup-mgw49-mt-s-1_60\
-lboost_log_setup-mgw49-mt-sd-1_60\
-lboost_math_c99-mgw49-mt-s-1_60\
-lboost_math_c99-mgw49-mt-sd-1_60\
-lboost_math_c99f-mgw49-mt-s-1_60\
-lboost_math_c99f-mgw49-mt-sd-1_60\
-lboost_math_c99l-mgw49-mt-s-1_60\
-lboost_math_c99l-mgw49-mt-sd-1_60\
-lboost_math_tr1-mgw49-mt-s-1_60\
-lboost_math_tr1-mgw49-mt-sd-1_60\
-lboost_math_tr1f-mgw49-mt-s-1_60\
-lboost_math_tr1f-mgw49-mt-sd-1_60\
-lboost_math_tr1l-mgw49-mt-s-1_60\
-lboost_math_tr1l-mgw49-mt-sd-1_60\
-lboost_prg_exec_monitor-mgw49-mt-s-1_60\
-lboost_prg_exec_monitor-mgw49-mt-sd-1_60\
-lboost_program_options-mgw49-mt-s-1_60\
-lboost_program_options-mgw49-mt-sd-1_60\
-lboost_python-mgw49-mt-s-1_60\
-lboost_python-mgw49-mt-sd-1_60\
-lboost_python3-mgw49-mt-s-1_60\
-lboost_python3-mgw49-mt-sd-1_60\
-lboost_random-mgw49-mt-s-1_60\
-lboost_random-mgw49-mt-sd-1_60\
-lboost_regex-mgw49-mt-s-1_60\
-lboost_regex-mgw49-mt-sd-1_60\
-lboost_serialization-mgw49-mt-s-1_60\
-lboost_serialization-mgw49-mt-sd-1_60\
-lboost_signals-mgw49-mt-s-1_60\
-lboost_signals-mgw49-mt-sd-1_60\
-lboost_system-mgw49-mt-s-1_60\
-lboost_system-mgw49-mt-sd-1_60\
-lboost_test_exec_monitor-mgw49-mt-s-1_60\
-lboost_test_exec_monitor-mgw49-mt-sd-1_60\
-lboost_thread-mgw49-mt-s-1_60\
-lboost_thread-mgw49-mt-sd-1_60\
-lboost_timer-mgw49-mt-s-1_60\
-lboost_timer-mgw49-mt-sd-1_60\
-lboost_type_erasure-mgw49-mt-s-1_60\
-lboost_type_erasure-mgw49-mt-sd-1_60\
-lboost_unit_test_framework-mgw49-mt-s-1_60\
-lboost_unit_test_framework-mgw49-mt-sd-1_60\
-lboost_wserialization-mgw49-mt-s-1_60\
-lboost_wserialization-mgw49-mt-sd-1_60
#dynamic library
#-lboost_atomic-mgw49-mt-1_60\
#-lboost_atomic-mgw49-mt-d-1_60\
#-lboost_chrono-mgw49-mt-1_60\
#-lboost_chrono-mgw49-mt-d-1_60\
#-lboost_container-mgw49-mt-1_60\
#-lboost_container-mgw49-mt-d-1_60\
#-lboost_context-mgw49-mt-1_60\
#-lboost_context-mgw49-mt-d-1_60\
#-lboost_coroutine-mgw49-mt-1_60\
#-lboost_coroutine-mgw49-mt-d-1_60\
#-lboost_date_time-mgw49-mt-1_60\
#-lboost_date_time-mgw49-mt-d-1_60\
#-lboost_exception-mgw49-mt-1_60\
#-lboost_exception-mgw49-mt-d-1_60\
#-lboost_filesystem-mgw49-mt-1_60\
#-lboost_filesystem-mgw49-mt-d-1_60\
#-lboost_iostreams-mgw49-mt-1_60\
#-lboost_iostreams-mgw49-mt-d-1_60\
#-lboost_locale-mgw49-mt-1_60\
#-lboost_locale-mgw49-mt-d-1_60\
#-lboost_log-mgw49-mt-1_60\
#-lboost_log-mgw49-mt-d-1_60\
#-lboost_log_setup-mgw49-mt-1_60\
#-lboost_log_setup-mgw49-mt-d-1_60\
#-lboost_math_c99-mgw49-mt-1_60\
#-lboost_math_c99-mgw49-mt-d-1_60\
#-lboost_math_c99f-mgw49-mt-1_60\
#-lboost_math_c99f-mgw49-mt-d-1_60\
#-lboost_math_c99l-mgw49-mt-1_60\
#-lboost_math_c99l-mgw49-mt-d-1_60\
#-lboost_math_tr1-mgw49-mt-1_60\
#-lboost_math_tr1-mgw49-mt-d-1_60\
#-lboost_math_tr1f-mgw49-mt-1_60\
#-lboost_math_tr1f-mgw49-mt-d-1_60\
#-lboost_math_tr1l-mgw49-mt-1_60\
#-lboost_math_tr1l-mgw49-mt-d-1_60\
#-lboost_prg_exec_monitor-mgw49-mt-1_60\
#-lboost_prg_exec_monitor-mgw49-mt-d-1_60\
#-lboost_program_options-mgw49-mt-1_60\
#-lboost_program_options-mgw49-mt-d-1_60\
#-lboost_python-mgw49-mt-1_60\
#-lboost_python-mgw49-mt-d-1_60\
#-lboost_python3-mgw49-mt-1_60\
#-lboost_python3-mgw49-mt-d-1_60\
#-lboost_random-mgw49-mt-1_60\
#-lboost_random-mgw49-mt-d-1_60\
#-lboost_regex-mgw49-mt-1_60\
#-lboost_regex-mgw49-mt-d-1_60\
#-lboost_serialization-mgw49-mt-1_60\
#-lboost_serialization-mgw49-mt-d-1_60\
#-lboost_signals-mgw49-mt-1_60\
#-lboost_signals-mgw49-mt-d-1_60\
#-lboost_system-mgw49-mt-1_60\
#-lboost_system-mgw49-mt-d-1_60\
#-lboost_test_exec_monitor-mgw49-mt-1_60\
#-lboost_test_exec_monitor-mgw49-mt-d-1_60\
#-lboost_thread-mgw49-mt-1_60\
#-lboost_thread-mgw49-mt-d-1_60\
#-lboost_timer-mgw49-mt-1_60\
#-lboost_timer-mgw49-mt-d-1_60\
#-lboost_type_erasure-mgw49-mt-1_60\
#-lboost_type_erasure-mgw49-mt-d-1_60\
#-lboost_unit_test_framework-mgw49-mt-1_60\
#-lboost_unit_test_framework-mgw49-mt-d-1_60\
#-lboost_wserialization-mgw49-mt-1_60\
#-lboost_wserialization-mgw49-mt-d-1_60

#edited for GDAL200
INCLUDEPATH += E:/QTPrj/GraduationDesignTest/GDAL200/include
LIBS += -LE:/QTPrj/GraduationDesignTest/GDAL200/lib\
-lgdal
#LIBS += E:/QTPrj/GraduationDesignTest/GDAL200/lib/libgdal.a

#edited for tinyxml
INCLUDEPATH += E:/QTPrj/GraduationDesignTest/QtTinyxml/include
LIBS += -LE:/QTPrj/GraduationDesignTest/QtTinyxml/lib\
-ltinyxml

#edited for libsvm
#库包含方�
INCLUDEPATH += E:/QTPrj/GraduationDesignTest/QtLibsvm/include
LIBS += -LE:/QTPrj/GraduationDesignTest/QtLibsvm/lib\
-lsvm
#库包含方�
#INCLUDEPATH += E:/QTPrj/QtLibsvm/include
#LIBS += E:/QTPrj/QtLibsvm/lib/svm.o

HEADERS += \
    kimagecvt.h \
    kpicinfo.h \
    common.h \
    kfeaturelbp.h \
    kprogressbar.h \
    ksplitimage.h \
    kcalpmk.h
