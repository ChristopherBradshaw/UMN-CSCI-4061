make clean
make
rm output/*
rm -rf input_dir
cp -r input_dir_orig_40_images input_dir
./parallel_convert $1 output input_dir
