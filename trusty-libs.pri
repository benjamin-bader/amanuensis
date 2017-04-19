QMAKE_LFLAGS += \
    -F /System/Library/Frameworks/Cocoa.framework/ \
    -F /System/Library/Frameworks/Foundation.framework/ \
    -F /System/Library/Frameworks/Security.framework \
    -F /System/Library/Frameworks/ServiceManagement.framework \
    -F /System/Library/Frameworks/System.framework/

LIBS += \
    -framework Cocoa \
    -framework Foundation \
    -framework Security \
    -framework ServiceManagement \
    -framework System
