export MINGW_PATH=/opt/mingw
export CPP=${MINGW_PATH}/usr/bin/i686-pc-mingw32-gcc
export CC=${MINGW_PATH}/usr/bin/i686-pc-mingw32-gcc 
export AR=${MINGW_PATH}/usr/bin/i686-pc-mingw32-ar
export RANLIB=${MINGW_PATH}/usr/bin/i686-pc-mingw32-ranlib
export LINK_CC=${MINGW_PATH}/usr/bin/i686-pc-mingw32-gcc

export PKG_CONFIG_PATH=/opt/mingw/usr/i686-pc-mingw32/lib/pkgconfig

./waf configure

