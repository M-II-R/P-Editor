#!/bin/sh
if [ ! -e nativefiledialog ]; then
  git clone "https://github.com/mlabbe/nativefiledialog"
  make -C nativefiledialog/build/gmake_linux
fi
cmake -S . -B build
make -C build
mv build/editor ./

