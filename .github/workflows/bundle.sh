#/bin/sh
set -e
PACKAGE=arrow

# Update
pacman -Syy --noconfirm
OUTPUT=$(mktemp -d)

# Download files (-dd skips dependencies)
pkgs=$(echo mingw-w64-{i686,x86_64,ucrt-x86_64}-{arrow,aws-sdk-cpp,brotli,openssl,lz4,re2,snappy,thrift,zstd,libutf8proc})
URLS=$(pacman -Spdd $pkgs --cache=$OUTPUT)
VERSION=$(pacman -Si mingw-w64-x86_64-${PACKAGE} | awk '/^Version/{print $3}')

# Set version for next step
echo "::set-output name=VERSION::${VERSION}"
echo "::set-output name=PACKAGE::${PACKAGE}"
echo "Bundling $PACKAGE-$VERSION"
echo "# $PACKAGE $VERSION" > README.md
echo "" >> README.md

for URL in $URLS; do
  curl -OLs $URL
  FILE=$(basename $URL)
  echo "Extracting: $URL"
  echo " - $FILE" >> README.md
  tar xf $FILE -C ${OUTPUT}
  rm -f $FILE
done

# Copy libs
rm -Rf lib
mkdir -p lib/{x64-ucrt,x64,i386}
cp -fv ${OUTPUT}/ucrt64/lib/*.a lib/x64-ucrt/
cp -fv ${OUTPUT}/mingw64/lib/*.a lib/x64/
cp -fv ${OUTPUT}/mingw32/lib/*.a lib/i386/

# Copy headers for some packages
rm -Rf include/*
cp -Rf ${OUTPUT}/mingw64/include/arrow include/
cp -Rf ${OUTPUT}/mingw64/include/parquet include/

# Cleanup temporary dir
rm -Rf ${OUTPUT}/*

# Setup backports repo
function finish {
  echo "Restoring pacman.conf"
  cp -f /etc/pacman.conf.bak /etc/pacman.conf
  rm -f /etc/pacman.conf.bak
  pacman -Scc --noconfirm
  pacman -Syy
}
trap finish EXIT
cp /etc/pacman.conf /etc/pacman.conf.bak
curl -Ol 'https://raw.githubusercontent.com/r-windows/rtools-backports/master/pacman.conf'
cp -f pacman.conf /etc/pacman.conf
pacman -Scc --noconfirm
pacman -Syy

# Download backports
backports=$(echo mingw-w64-{i686,x86_64}-{snappy,thrift,arrow})
URLS=$(pacman -Spdd $backports --cache=$OUTPUT)
for URL in $URLS; do
  curl -OLs $URL
  FILE=$(basename $URL)
  echo "Extracting: $URL"
  tar xf $FILE -C ${OUTPUT}
  rm -f $FILE
done

# Copy libs
rm -Rf lib-4.9.3
mkdir -p lib-4.9.3/{i386,x64}
cp -fv ${OUTPUT}/mingw32/lib/lib*.a lib-4.9.3/i386/
cp -fv ${OUTPUT}/mingw64/lib/lib*.a lib-4.9.3/x64/

# Cleanup temporary dir
rm -Rf ${OUTPUT} pacman.conf
