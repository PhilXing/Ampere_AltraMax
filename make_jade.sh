#!/bin/bash
OPATH=$PATH
. edk2.sh
make -f $WORKSPACE/edk2-ampere-tools/Makefile \
    BOARD_SETTING=$WORKSPACE/srp-ac02-fw-edk2-src-r2.03.20210910/altra_firmware_sdk/bin/atf/jade_board_setting.bin \
    VM_SHARED_DIR=$HOME/AmpereR \
    ATF_SLIM=$WORKSPACE/srp-ac02-fw-edk2-src-r2.03.20210910/altra_firmware_sdk/bin/atf/altra_atf_signed_2.03.20210719.slim \
    SCP_SLIM=$WORKSPACE/srp-ac02-fw-edk2-src-r2.03.20210910/altra_firmware_sdk/bin/scp/altra_scp_signed_2.03.20210719.slim \
    LINUXBOOT_BIN=$WORKSPACE/flashkernel \
    VER=1.07 BUILD=300 \
    all
    # PACKAGES_PATH=$WORKSPACE/adlink-platforms:"${PACKAGES_PATH}" \
export PATH=$OPATH
