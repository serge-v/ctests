SRCDIR=$HOME/src/xtree/ctests

mkdir -p ~/b/ctestsx
cd ~/b/ctestsx
cmake -G Xcode -DCMAKE_TOOLCHAIN_FILE=$SRCDIR/macports.cmake $SRCDIR

mkdir -p ~/b/ctestsb
cd ~/b/ctestsb
cmake -DCMAKE_TOOLCHAIN_FILE=$SRCDIR/macports.cmake $SRCDIR
