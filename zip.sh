cp out/arch/arm64/boot/Image ./anykernel/
cp out/arch/arm64/boot/dtbo.img ./anykernel/


cd anykernel && zip -r Kraken-kernel-AOSP-$(date +"%d-%m-%Y"-%H%M).zip * && mv Kraken-kernel-AOSP-$(date +"%d-%m-%Y-%H%M").zip ../out/

cd ..
