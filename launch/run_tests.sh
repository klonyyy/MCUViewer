#!/usr/bin/env bash
git config --global --add safe.directory /__w/STMViewer/STMViewer

(cd ..; rm -rf build_test)
(cd ..; mkdir build_test && cd build_test && cmake .. -DMAKE_TESTS=1 && make -j)
(cd ../build_test/test; ./STMViewer_test)

