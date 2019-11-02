# Apache Arrow 0.15.1

Backports for the R legacy toolchain [lib-4.9.3](lib-4.9.3) built with [rtools-backports](https://github.com/r-windows/rtools-backports/blob/master/mingw-w64-arrow/PKGBUILD).

Now supports parquet (thrift) and snappy. Example flags to compile and link the R bindings:

```
PKG_CPPFLAGS = -I$(ARROW_INCLUDE) \
	-DARROW_R_WITH_ARROW -DARROW_R_WITH_PARQUET \
	-DARROW_STATIC -DPARQUET_STATIC

CXX_STD = CXX11

PKG_LIBS = \
	-L$(ARROW_LIBS) \
	-lparquet -larrow -lthrift \
	-lboost_system-mt-s -lboost_filesystem-mt-s -lboost_regex-mt-s \
	-ldouble-conversion -lsnappy -lz -lws2_32
```

To test this make sure you install the arrow package from a release tag:

```r
remotes::install_github("apache/arrow/r@apache-arrow-0.15.1")
```

To install R package from the arrow master branch you also would need to rebuild the master branch arrow C++ library from source.
