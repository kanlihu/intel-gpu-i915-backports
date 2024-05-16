KLIB=/code/fei/dual_i915_port/virtualization.hypervisors.acrn.acrn-automotive.acrn-kernel/
KLIB_BUILD=/code/fei/dual_i915_port/virtualization.hypervisors.acrn.acrn-automotive.acrn-kernel/
SRC=`pwd`
#KLIB=/lib/modules/$(uname -r)
#KLIB_BUILD=/lib/modules/$(uname -r)/build
#make CC=clang-12 KLIB=$KLIB KLIB_BUILD=$KLIB_BUILD mrproper
#make CC=clang-12 KLIB=$KLIB KLIB_BUILD=$KLIB_BUILD menuconfig
#make CC=clang-12 KLIB=$KLIB KLIB_BUILD=$KLIB_BUILD olddefconfig
make CC=clang-12 -f  modules -j32
#make CC=clang-12 KLIB=$KLIB KLIB_BUILD=$KLIB_BUILD bindeb-pkg -j32
#make CC=clang-12 KLIB=$KLIB KLIB_BUILD=$KLIB_BUILD modules_install -j32
