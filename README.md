# Apache Arrow 0.14.0

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

The easiest way to test this is using the R package from the [arrow-r-dev](https://github.com/jeroen/arrow-r-dev) repository:

```r
remotes::install_github("jeroen/arrow-r-dev")
```

The upstream R bindings require the libarrow verion from the master branch so it is typically broken.
