make clean
make
rm output/*
rm -rf input_dir
cp -r input_dir_orig input_dir
./parallel_convert 7 output input_dir
