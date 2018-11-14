defineTest(includeNeighborLib) {
  libname = $$1

  win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../$${libname}/release/ -l$${libname}
  else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../$${libname}/debug/ -l$${libname}
  else:unix: LIBS += -L$$OUT_PWD/../$${libname}/ -l$${libname}

  export(LIBS)
}
