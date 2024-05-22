#!/bin/bash

#
# Clone proton-clang toolchain if needed
#

KERNEL_DEFCONFIG=vendor/sm8250_defconfig
DIR=$PWD
export ARCH=arm64
export SUBARCH=arm64
export CLANG_PATH="~/toolchains/bin"
export PATH="$CLANG_PATH:$PATH"
export CROSS_COMPILE=aarch64-linux-gnu-
export CROSS_COMPILE_ARM32=arm-linux-gnueabi-
export KBUILD_BUILD_USER=Amog
export KBUILD_BUILD_HOST=Us

echo
echo "Kernel is going to be built using $KERNEL_DEFCONFIG"
echo

make CC=clang AR=llvm-ar NM=llvm-nm OBJCOPY=llvm-objcopy OBJDUMP=llvm-objdump STRIP=llvm-strip LLVM=1 LLVM_IAS=1 O=out $KERNEL_DEFCONFIG

make O=${OUT_DIR} ${TARGET_KERNEL_MAKE_ENV} LLVM_IAS=1 HOSTLDFLAGS="${TARGET_LINCLUDES}" ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} NM=llvm-nm OBJCOPY=llvm-objcopy OBJDUMP=llvm-objdump STRIP=llvm-strip -j10 LLVM_IAS=1 vendor/$config

make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} LLVM_IAS=1 HOSTCFLAGS="${TARGET_INCLUDES}" HOSTLDFLAGS="${TARGET_LINCLUDES}" O=${OUT_DIR} ${TARGET_KERNEL_MAKE_ENV} NM=llvm-nm OBJCOPY=llvm-objcopy OBJDUMP=llvm-objdump STRIP=llvm-strip LLVM_IAS=1 -j$(nproc --all) |& tee build.log
}

zipping() {
echo zipping kernel

cd anykernel || exit 1
    rm *zip
    cp ../out/arch/arm64/boot/Image .
    cp ../out/arch/arm64/boot/dtbo.img .
    cp ../out/arch/arm64/boot/dtb .
    zip -r9 phoeniX-AOSP-${DT}.zip *
    cd ..
}

compile
zipping
