#
# Inherit the full_base and device configurations
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base.mk)
$(call inherit-product, device/nufront/nusmart3_pad/device.mk)

#
# Overrides
PRODUCT_NAME := full_nusmart3_pad
PRODUCT_DEVICE := nusmart3_pad
PRODUCT_BRAND := Livall
PRODUCT_MODEL := N751
