autoreconf -i && rm -rf build && rm -rf installation && mkdir build && mkdir installation && cd build && CXX="ccache g++" ../configure --with-doxygen --prefix=$WORKSPACE/installation && make && make check && cd tests/ && make check-valgrind && cd ../lib/tests/ && make check-valgrind