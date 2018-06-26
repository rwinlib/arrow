[Apache arrow](https://github.com/apache/arrow) built with the gcc 4.9.6 toolchain

Statically linked to a boost dependency compiled with msys2-mingw gcc 7.3.0
toolchain with -D_GLIBCXX_USE_CXX11_ABI=0 defined to provide ABI compatibility
with gcc 4.9.3 shipped with R.

[arrow.patch](arrow.patch) provides changes needed to compile arrow 0.9.0_1 with this toolchain.

arrow cmake invocation was

cmake -G"MSYS Makefiles" .. \
  -DARROW_BUILD_TESTS=FALSE \
  -DARROW_WITH_SNAPPY=FALSE \
  -DARROW_WITH_ZSTD=FALSE \
  -DARROW_WITH_LZ4=FALSE \
  -DARROW_JEMALLOC=FALSE \
  -DARROW_BUILD_STATIC=TRUE \
  -DARROW_BOOST_VENDORED=FALSE \
  -DARROW_BOOST_USE_SHARED=FALSE \
  -DARROW_WITH_ZLIB=FALSE \
  -DARROW_WITH_BROTLI=FALSE

