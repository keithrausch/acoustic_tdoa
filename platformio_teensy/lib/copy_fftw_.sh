#!/bin/bash

DST="fftw_copy_fuck_platformio"
SEARCH_PATH="fftw/fftw-3.3.10"
mkdir "$DST"

thing() {
    FOLDER=$1
    EXTENSION=$2
    find "$FOLDER" -name "$EXTENSION" -print0 | while read -d '' file
    do
        target="${file#./}" # remove './' at the beginning
        target=${target#$SEARCH_PATH/}
        target="${target//\//_}" # replace '/' by '_' and add '.jpg'
        target="$DST/$target"
        if [[ $target == *"f77"* ]]; then
            continue
        fi
        # if [[ $target == *"guru"* ]]; then
        #     continue
        # fi
        # if [[ $target == *"tensor"* ]]; then
        #     continue
        # fi
        if [[ $target == *"simd"* ]]; then
            continue
        fi
        # cp "$file" "$target" # do the moving
        # echo "file was $file but is now $target"
        sed -e 's|\(include \+\"[^\/]\+\)\/|\1_|g' "$file" > "$target"
        sed -i -e 's|\(include \+\"[^\/]\+\)\/|\1_|g' "$target" # run again
        sed -i -e 's|\(include \+\"[^\/]\+\)\/|\1_|g' "$target" # run again
        sed -i -e 's|include \"guru.h\"|include \"api_guru.h\"|g' "$target"
        sed -i -e 's|include \"guru64.h\"|include \"api_guru64.h\"|g' "$target"
        sed -i -e 's|include \"plan-guru-|include \"api_plan-guru-|g' "$target"

    done

}

my_copy() {
    thing $1 "*.h"
    thing $1 "*.c"
}

my_copy "$SEARCH_PATH/api/"
my_copy "$SEARCH_PATH/dft/"
my_copy "$SEARCH_PATH/dft/scalar/"
my_copy "$SEARCH_PATH/dft/scalar/codelets/"
my_copy "$SEARCH_PATH/kernel/"
my_copy "$SEARCH_PATH/rdft/"
my_copy "$SEARCH_PATH/rdft/scalar/"
my_copy "$SEARCH_PATH/rdft/scalar/r2cb/"
my_copy "$SEARCH_PATH/rdft/scalar/r2cf/"
my_copy "$SEARCH_PATH/rdft/scalar/r2r/"
my_copy "$SEARCH_PATH/reodft/"
cp "$SEARCH_PATH/config.h" "$DST/config.h"

sed -i -e 's|include \"cycle.h\"|include \"kernel_cycle.h\"|g' "$DST/kernel_timer.c"
sed -i -e 's|include \"ct-hc2c.h\"|include \"rdft_ct-hc2c.h\"|g' "$DST/rdft_ct-hc2c-direct.c"
sed -i -e 's|include \"ct-hc2c.h\"|include \"rdft_ct-hc2c.h\"|g' "$DST/rdft_ct-hc2c.c"
sed -i -e 's|include \"ct-hc2c.h\"|include \"rdft_ct-hc2c.h\"|g' "$DST/rdft_khc2c.c"
sed -i -e 's|include \"guru64.h\"|include \"api_guru64.h\"|g' "$DST/api_plan-dft-2d.c"
sed -i -e 's|include \"guru.h\"|include \"api_guru.h\"|g' "$DST/api_plan-dft-c2r-1d.c"
sed -i -e 's|include \"guru.h\"|include \"api_guru.h\"|g' "$DST/api_mktensor-iodims64.c"
sed -i -e 's|include \"guru.h\"|include \"api_guru.h\"|g' "$DST/api_mktensor-iodims.c"
sed -i -e 's|include \"guru64.h\"|include \"api_guru64.h\"|g' "$DST/api_plan-dft-r2c-3d.c"
sed -i -e 's|include \"mktensor-iodims.h\"|include \"api_mktensor-iodims.h\"|g' "$DST/api_mktensor-iodims64.c"
sed -i -e 's|include \"guru.h\"|include \"api_guru.h\"|g' "$DST/api_mktensor-iodims.c"
sed -i -e 's|include \"mktensor-iodims.h\"|include \"api_mktensor-iodims.h\"|g' "$DST/api_mktensor-iodims.c"
sed -i -e 's|include \"guru64.h\"|include \"api_guru64.h\"|g' "$DST/api_mktensor-iodims64.c"

# cp fftw/fftw-3.3.10/api/*.{c,h} .
# cp fftw/fftw-3.3.10/dft/*.{c,h} .
# cp fftw/fftw-3.3.10/dft/scalar/*.{c,h} .
# cp fftw/fftw-3.3.10/dft/scalar/codelets/*.{c,h} .
# cp fftw/fftw-3.3.10/kernel/*.{c,h} .
# cp fftw/fftw-3.3.10/rdft/*.{c,h} .
# cp fftw/fftw-3.3.10/rdft/scalar/*.{c,h} .
# cp fftw/fftw-3.3.10/rdft/scalar/r2cb/*.{c,h} .
# cp fftw/fftw-3.3.10/rdft/scalar/r2cf/*.{c,h} .
# cp fftw/fftw-3.3.10/rdft/scalar/r2r/*.{c,h} .
# cp fftw/fftw-3.3.10/reodft/*.{c,h} .