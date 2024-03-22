KLIB=/code/fei/acrn_project/acrn-kernel_137
KLIB_BUILD=/code/fei/acrn_project/acrn-kernel_137
#KLIB=/lib/modules/$(uname -r)
#KLIB_BUILD=/lib/modules/$(uname -r)/build
make CC=clang-12 KLIB=$KLIB KLIB_BUILD=$KLIB_BUILD modules -j31
