
# Bouffalo Lab factory data

Bouffalo Lab factory data has two locations
- the common data shared with a mount of devices, such as vendor ID, etc.
- the credential data and the device unique data, such as DAC private key and Serial Number, etc.

# The common data

The common data is configured in DTS file, DTS example `examples/platform/bouffalolab/bl602/flash_config/factory_data.dts` has an new entry for this.
```dts
    ...
    MFD {
        vendor_name = "Bouffalo Lab";
        vendor_id = [0x0d 0x13];
        product_name = "Test Device";
        product_id = [0x01 0xf0];
        product_part_number = "Test Part Number";
        product_url = "Test Product URL";
        product_label = "Test Product Label";
        manufactoring_date = <2023 02 26>;
        hardware_version = [0x01 0x00];
        hardware_version_string = "ver_0.1";
    };
    ...
```

Then, please generate whole_flash_data.bin with the configured DTS file.

# The credential data and the device unique data

Using command line tool `bflb_eflash_loader.exe`, the both of credential data and device unique data are programming to device with whole_flash_data.bin built with DTS file; and the command is showed below.

```shell
.\bflb_eflash_loader.exe --chipname=bl602 -w --flash -p=<serial port> --mass=whole_flash_data.bin --dac=<credential data and unique data> --dacaddr=0x3FE000 --encrypt --publickey=<public_key> --privatekey=<private_key>
```

- -p=\<serial port\>, the bl602 serial port connected to flash station
- --dac=\<credential data and unique data\>, the date is list below in sequence and each item is delimated by a comma
    - DAC certificate, input as hex string
    - DAC certificate private key, input as hex string
    - pin code, input as hex string, 4bytes length with 8 hex characters. The following example `49808401` shows pin code with decimal `25460809`.
    - PAI certificate, input as hex string
    - Certificate Declare, input as hex string
    - Serial Number, input as hex string, 
    - Discriminator ID, input as hex string, 2 bytes length with 4 hex characters. The following example `b602` shows disciminator with decimal `694`.
        > Please note, discriminator ID only has **12 bits** length in Matter Protocol.
    - the following is a paramter example, and DAC/PAI cetificate is issued with Chip-Test-PAA-NoVID-Cert.pem/Chip-Test-PAA-NoVID-Key.pem
        ```
        --dac=308201d030820176a003020102020868a3b66e276b5b29300a06082a8648ce3d04030230303118301606035504030c0f4d617474657220546573742050414931143012060a2b0601040182a27c02010c04313330443020170d3231303632383134323334335a180f39393939313233313233353935395a3048311a301806035504030c114d6174746572205465737420444143203031143012060a2b0601040182a27c02010c043133304431143012060a2b0601040182a27c02020c04463030313059301306072a8648ce3d020106082a8648ce3d03010703420004d81993acf1c863bb042b8c2e4de408394ff93ea38919968c22a10feb4c202a8a12ffe4e6094f134ba835532fa49d8e798c07015c73ff0d1c34fe147fbec670f8a360305e300c0603551d130101ff04023000300e0603551d0f0101ff040403020780301d0603551d0e04160414639fbd918c90ed7abf64cde82e13108eee5d6f8e301f0603551d230418301680143ab2fa0627cc4adc84ac9c951e8c8fdc4c0dec01300a06082a8648ce3d0403020348003045022100d6c3ab6dbdd0a245bccfb72d6f5ed1bed7d4eaca9ef2a10977c343a22967633d02202bb839f831eb8331e231ebd25c82e4b714fd62971f64ff514383b4a9196f96cf,6511782ef7cd3059bd3add446fc5dd92e5d83c569d677a29c7e9a78f67c55ff2,49808401,308201a73082014ea0030201020208010f96a1425ce70c300a06082a8648ce3d040302301a3118301606035504030c0f4d61747465722054657374205041413020170d3231303632383134323334335a180f39393939313233313233353935395a30303118301606035504030c0f4d617474657220546573742050414931143012060a2b0601040182a27c02010c04313330443059301306072a8648ce3d020106082a8648ce3d03010703420004dbbeacd5f3d632360400e0a1168dbf0c748fdbd7037f8c4e6558272ffa2d64614ee572b8a5878c15c7255547c4c962dad70264d2842de82df169cb564286f855a366306430120603551d130101ff040830060101ff020100300e0603551d0f0101ff040403020106301d0603551d0e041604143ab2fa0627cc4adc84ac9c951e8c8fdc4c0dec01301f0603551d23041830168014785ce705b86b8f4e6fc793aa60cb43ea696882d5300a06082a8648ce3d040302034700304402200f122f8c83abdd3553bccde5ec2d849924b5e01a54453778ba88a85118f80e050220598416e1d1ee5ec650cd5d5847b81082a9a22f4f317ec75db66cc9c570b81ed8,3081e906092a864886f70d010702a081db3081d8020103310d300b0609608648016503040201304506092a864886f70d010701a03804361524000125010d1336020501f018250334122c04135a494732303134315a423333303030312d32342405002406002507769824080018317d307b020103801462fa823359acfaa9963e1cfa140addf504f37160300b0609608648016503040201300a06082a8648ce3d04030204473045022100d991c9ceaf8e815610631d1d693d0cbbb6176e0b91ba7b2357dc508023c98ed00220345debd738ca8ffba84e8be95b668b15b90a9af211820a7af29f3bddd8155107,546573742053657269616c204e756d626572,0b62
        ```
- Key Pairs, --publickey=<public_key> --privatekey=<private_key>


