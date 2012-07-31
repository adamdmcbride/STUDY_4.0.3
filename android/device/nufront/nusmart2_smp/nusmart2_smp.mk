#
# Inherit the full_base and device configurations
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base.mk)
$(call inherit-product, device/nufront/nusmart2_smp/device.mk)

#
# Overrides
PRODUCT_NAME := nusmart2_smp 
PRODUCT_DEVICE := nusmart2_smp
PRODUCT_BRAND := Android
PRODUCT_MODEL := Full AOSP on NS2816
