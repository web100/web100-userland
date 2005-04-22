from distutils.core import setup, Extension

setup(name="libweb100",
      description="SWIG wrapper for libweb100",
      url="http://www.web100.org/",
      ext_modules=[Extension("_libweb100", ["libweb100_wrap.c"], \
                             include_dirs=["../lib"], \
                             library_dirs=["../lib/.libs"], libraries=["web100"])],
      py_modules=['libweb100'])

setup(name="Web100",
      version="1.0",
      description="Web100 statistics interface",
      author="John Heffner",
      author_email="jheffner@psc.edu",
      url="http://www.web100.org/",
      license="LGPL 1.2",
      py_modules=['Web100'])
