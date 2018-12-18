# Apache Arrow

A backport of libarrow for Rtools 3.5. This was built using the automated 
rtools40 build system while targeting the old gcc-4.9.3 compilers. 
See [PKGBUILD](PKGBUILD) for details. It was built with:

 - arrow 0.11.1
 - boost 1.67.0

To rebuild this, install both rtools35 and rtools40. Then open the rtools40
shell, navigate to this directory, and run: `makepkg-mingw`.

Alternatively send a PR to the `backports` branch in the [rtools-packages](https://github.com/r-windows/rtools-packages/tree/backports) repo.

## How to link

The R package `Makevars.win` should contain `-DARROW_STATIC` to static link:

```make
CXX_STD=CXX11
PKG_CPPFLAGS=-I../windows/arrow-1.11.1/include -DARROW_STATIC
PKG_LIBS=-L../windows/arrow-1.11.1/lib${R_ARCH} -larrow
```
