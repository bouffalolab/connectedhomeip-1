# Matter `Bouffalo Lab` Lighting Example

This example functions as a light bulb device type, with on/off and level
capabilities and uses a test Vendor ID (VID) and a Product ID (PID)
of **0x8005**.

Current supported boards:

- `BL602DK`

- `BL616DK`

  > Please contact `Bouffalo Lab` for BL616 SDK.

- `BL704LDK`

-   `BL706DK`

Legacy supported boards:

-   `BL602-IoT-Matter-V1`, [here](https://www.amazon.com/dp/B0B9ZVGXD8) to purchase.
-   `BL602-NIGHT-LIGHT`
-   `XT-ZB6-DevKit`
-   `BL706-NIGHT-LIGHT`

> Warning: Changing the VID/PID may cause compilation problems, we recommend leaving it as the default while using this example.

## `Bouffalo Lab` SoCs

- BL602

  BL602/BL604 is combo chip-set for Wi-Fi 802.11b/g/n and BLE 5.0 base-band/MAC.

- BL61X

  BL61X is combo chip-set for Wi-Fi 6, Classic Bluetooth, Bluetooth low energy 5.3 and IEEE 802.15.4/ZigBee/Thread. 

  BL61X has fully certified with all Thread 1.3 features, included Thread `SSED` and Thread Border Router.

- BL70X

  BL70X is combo chip-set for BLE and IEEE 802.15.4/ZigBee/Thread. 

  BL70X has fully certified with all Thread 1.3 features, included Thread `SSED` and Thread Border Router.

  - BL702/BL706 has 14dbm tx power and is recommended for routing devices. SDK uses BL702 as a general name.
  - BL702L/BL704L is designed for low power application. SDK uses BL702L as a general name.

## Solutions introduction

`Bouffalo Lab` has full connectivity supports for Matter Applications.

-   Wi-Fi 4/6 application, we have
    -   BL602, Wi-Fi 4 application.
    -   BL706 + BL602, Wi-Fi 4 application.
    -   BL616, Wi-Fi 6 application
-   Thread application, we have
    -   BL70X/BL616
    -   BL704L, Thread low power application.
-   Ethernet application, we have
    -   BL706/BL618
-   Openthread Border Router application based on FreeRTOS
    -   BL706 + BL602, Wi-Fi 4 as infrastructure network
    -   BL616, Wi-Fi 6 as infrastructure network
    -   BL706/BL616, Ethernet as as infrastructure network
-   Matter ZigBee Bridge application based on FreeRTOS - BL706 + BL602, Wi-Fi 4
    as infrastructure network - BL616, Wi-Fi 6 as infrastructure network -
    BL706/BL616, Ethernet as as infrastructure network
    > Please contact `Bouffalo Lab` for supports on OTBR and Matter ZigBee
    > Bridge application

## Initial setup

The following steps in this document were validated on Ubuntu 20.04.

-   Install dependencies as specified in the **connectedhomeip** repository:
    [Building Matter](https://github.com/project-chip/connectedhomeip/blob/master/docs/guides/BUILDING.md).

-   Clone and initialize the **connectedhomeip** repo

    ```
    git clone https://github.com/project-chip/connectedhomeip.git
    cd connectedhomeip
    git submodule update --init --recursive
    source ./scripts/activate.sh -p bouffalolab
    ```

-   Setup build environment for `Bouffalo Lab` SoC

    ```
    ./integrations/docker/images/stage-2/chip-build-bouffalolab/setup.sh
    ```

    Script `setpu.sh` requires to select install path, and please execute
    following command to export `BOUFFALOLAB_SDK_ROOT` before building.

    ```
    export BOUFFALOLAB_SDK_ROOT="Your install path"
    ```

## Build options with build_examples.py

Please try `./scripts/build/build_examples.py targets` to check supports
options.

-   supported board options, select one of the following options to build

    -   `-bl602dk`
    -   `-bl616dk`
    -   `-bl704ldk`
    -   `-bl706dk`
    -   `-bl602-night-light`
    -   `-bl706-night-light`
    -   `-bl602-iot-matter-v1`
    -   `-xt-zb6-devkit`

-   supported example options, select one of the following options to build

    -   `-light`
    -   `-contact-sensor`

-   connectivity options, select one of the following options to build

    -   `-wifi`, specifies to use Wi-Fi for Matter application.

    -   `-ethernet`, specifies to use Ethernet for Matter application.

    -   `-thread`, specifies to use Thread FTD for Matter application.

    -   `-thread-ftd`, specifies to use Thread FTD for Matter application.

    -   `-thread-mtd`, specifies to use Thread MTD for Matter application.

-   storage options, select one of the following options to build

    -   `-littlefs`, specifies to use `littlefs` for flash access.

    -   `-easyflash`, specifies to use `easyflash` for flash access.

        > `littlefs` has different format with `easyflash`, please uses
        > `-easyflash` for your in-field production

-   `-rotating_device_id`, enable rotating device id

-   `-mfd`, enable Matter factory data feature, which load factory data from
    `MFD` partition
    -   Please refer to
        [Bouffalo Lab Matter factory data guide](../../../docs/platforms/bouffalolab/matter_factory_data.md)
        or contact to `Bouffalo Lab` for support.
-   `-shell`, enable command line
-   `-rpc`, enable Pigweed RPC feature
-   `-cdc`, enable USB CDC feature, only support for BL706, and can't work with
    Ethernet Board
-   `-mot`, to specify to use openthread stack under
    `third_party/openthread/repo`
    -   Without `-mot` specified, Matter Thread will use openthread stack under
        `Bouffalo Lab` SDK

By default, `Bouffalo Lab` Matter project uses UART `baudrate` 2000000 for
logging output by default, please change variable `baudrate` in `BUILD.gn` under
example project.

## Build CHIP Lighting App example

The following steps take examples for `BL602DK`, `BL616DK`, `BL704LDK` and
`BL706DK`.

-   Build lighting app with UART baudrate 2000000

    ```
    ./scripts/build/build_examples.py --target bouffalolab-bl602dk-light-wifi-littlefs build
    ./scripts/build/build_examples.py --target bouffalolab-bl616dk-light-wifi-littlefs build
    ./scripts/build/build_examples.py --target bouffalolab-bl616dk-light-thread-littlefs build
    ./scripts/build/build_examples.py --target bouffalolab-bl704ldk-light-thread-littlefs build
    ./scripts/build/build_examples.py --target bouffalolab-bl706dk-light-thread-littlefs build
    ```

-   Build lighting app with RPC enabled.

    ```
    ./scripts/build/build_examples.py --target bouffalolab-bl602dk-light-wifi-littlefs-rpc build
    ./scripts/build/build_examples.py --target bouffalolab-bl704ldk-light-thread-littlefs-rpc build
    ./scripts/build/build_examples.py --target bouffalolab-bl706dk-light-thread-littlefs-rpc build
    ```

## Download image

After Matter project compiled, take BL602DK lighting app with Wi-Fi and
`littlefs` supported as example, `chip-bl602-lighting-example.flash.py` will be
generated out under `./out/bouffalolab-bl602dk-light-wifi-littlefs/`.

Download operation steps as below, please check `help` option of script for more
detail.

-   Connect the board to your build machine with USB cable

-   Put the board to the download mode:

    -   Press and hold the **BOOT** button.
    -   Click the **RESET** or **EN** button.
    -   Release the **BOOT** button.

-   Type following command for image download. Please set serial port
    accordingly, here we use /dev/ttyACM0 as a serial port example.

    -   `BL602DK`, `BL616DK`, `BL704LDK` and `BL706DK`.

        ```shell
        ./out/bouffalolab-bl602dk-light-wifi-littlefs/chip-bl702-lighting-example.flash.py --port /dev/ttyACM0
        ```

    -   To wipe out flash and download image, please append `--erase` option.

        ```shell
        ./out/bouffalolab-bl602dk-light-wifi-littlefs/chip-bl702-lighting-example.flash.py --port /dev/ttyACM0 --erase
        ```

        > Note, better to append --erase option to download image for BL602
        > develop board at first time.

## Run the example

You can open the serial console. For example, if the device is at `/dev/ttyACM0`
with UART baudrate 2000000 built:

```shell
picocom -b 2000000 /dev/ttyACM0
```

-   To reset the board, Click the **RESET** or **EN** button.

-   To toggle the light bulb’s on/off state by clicking BOOT button, which also
    toggles the LED.

-   To do factory reset, press BOOT button over 4 seconds, release BOOT button
    after led blink stopped.

## Test Commission and Control with chip-tool

Please follow [chip_tool_guide](../../../docs/guides/chip_tool_guide.md) and
[guide](../../chip-tool/README.md) to build and use chip-tool for test.

### Prerequisite for Thread Protocol

Thread wireless protocol runs on BL704L/BL706/BL616, which needs a Thread border
router to connect Thread network to Wi-Fi/Ethernet network. Please follow this
[guide](../../../docs/guides/openthread_border_router_pi.md) to setup a
raspberry Pi border router.

After Thread border router setup, please type following command on Thread border
router to get Thread network credential.

```shell
sudo ot-ctl dataset active -x
```

### Commissioning over BLE

-   Reset the board or factory reset the board

-   Enter build out folder of chip-tool and running the following command to do
    BLE commission

    -   Wi-Fi

        ```shell
        ./chip-tool pairing ble-wifi <node_id> <wifi_ssid> <wifi_passwd> 20202021 3840
        ```

    -   Thread

        ```shell
        ./chip-tool pairing ble-thread <node_id> hex:<thread_operational_dataset> 20202021 3840
        ```

    > `<node_id>`, which is node ID assigned to device within chip-tool
    > fabric<br> `<wifi_ssid>`, Wi-Fi network SSID<br> `<wifi_passwd>`, Wi-FI
    > network password<br> `<thread_operational_dataset>`, Thread network
    > credential which running `sudo ot-ctl dataset active -x` command on border
    > router to get.

### Cluster control

After successful commissioning, cluster commands available to control the board.

-   OnOff cluster

    The following command shows to toggle the LED on the board

    ```
    $ ./chip-tool onoff toggle <node_id> 1
    ```

-   Level cluster

    The following command shows to move level to 128.

    ```
    $ ./chip-tool levelcontrol move-to-level 128 10 0 0 <node_id> 1
    ```

-   Color cluster

    The following command shows to change hue and saturation to 240 and 100

    ```
    $ ./chip-tool colorcontrol move-to-hue-and-saturation 240 100 0 0 0 <node_id> 1
    ```

-   Identify Light

    The following command shows to identify the board 10 seconds

    ```shell
    ./chip-tool identify identify 10 <node_id> 1
    ```

## Test OTA software upgrade with ota-provider-app

Please take [guide](../../ota-provider-app/linux/README.md) for more detail on
ota-provider-app build and usage.

### Build on OTA image

After Matter project compiled, take BL602DK lighting app with Wi-Fi and
`littlefs` supported as example, `chip-bl702-lighting-example.flash.py` will be
generated out under `./out/bouffalolab-bl602dk-light-wifi-littlefs/`.

Type following command to generated OTA images:

```shell
./out/bouffalolab-bl602dk-light-wifi-littlefs/chip-bl702-lighting-example.flash.py --build-ota --vendor-id <vendor id> --product-id <product id> --version <version number> --version-str <version number string> --digest-algorithm <digest algorithm>
```

Please find `./src/app/ota_image_tool.py` for information on `vendor id`,
`product id`, `version number`, `version number string` and `digest algorithm`.

Here is an example to generate an OTA image,

> please change `CHIP_DEVICE_CONFIG_DEVICE_SOFTWARE_VERSION` in
> CHIPProjectConfig.h under example folder before to build a firmware image.

```shell
./out/bouffalolab-bl602dk-light-wifi-littlefs/chip-bl702-lighting-example.flash.py --build-ota --vendor-id 0xFFF1 --product-id 0x8005 --version 10 --version-str "1.0" --digest-algorithm sha256
```

All of BL602, BL702L and BL706 have same OTA image format. Take BL602DK lighting
app with Wi-Fi and `littlefs` supported as example, after command executed, OTA
images will generated under
`out/bouffalolab-bl602dk-light-wifi-littlefs/ota_images`:

-   `chip-bl602dk-lighting-example.bin.hash.matter`, OTA image packed with raw
    firmware image.
-   `chip-bl602dk-lighting-example.bin.xz.hash.matter`, OTA image packed with
    compressed firmware image.

BL616 SoC platform uses different OTA image format. Take BL616D lighting app
with Wi-Fi and `littlefs` supported as example:

-   `chip-bl616-lighting-example.bin.ota.matter`, OTA image packed with raw
    firmware image.
-   `chip-bl616-lighting-example.xz.ota.matter`, OTA image packed with
    compressed firmware image.

> Please contact `Bouffalo Lab` for more security requirements on firmware and
> OTA images.

### Start ota-provider-app

-   Start ota-provider-app with OTA image.

    ```shell
    $ rm -r /tmp/chip_*
    $ out/linux-x64-ota-provider/chip-ota-provider-app -f out/bouffalolab-bl602dk-light-wifi-littlefs/ota_images/chip-bl702-lighting-example.bin.xz.hash.matter
    ```

-   Provision ota-provider-app with assigned node id to 1
    ```shell
    $ ./chip-tool pairing onnetwork 1 20202021
    $ ./chip-tool accesscontrol write acl '[{"fabricIndex": 1, "privilege": 5, "authMode": 2, "subjects": [112233], "targets": null}, {"fabricIndex": 1, "privilege": 3, "authMode": 2, "subjects": null, "targets": null}]' 1 0
    ```

### Start ota software upgrade

-   BLE commission BL602/BL616/BL702 lighting if not commissioned.
-   Start OTA software upgrade process
    ```shell
    ./chip-tool otasoftwareupdaterequestor announce-otaprovider 1 0 0 0 <node_id_to_lighting_app> 0
    ```
    where `<node_id_to_lighting_app>` is node id of BL602/BL616/BL702 lighting
    app.
-   After OTA software upgrade gets done, BL602/BL616/BL702 will get reboot
    automatically.

## Run RPC Console

-   Build chip-console following this
    [guide](../../common/pigweed/rpc_console/README.md)

-   Start the console

    ```
    $ chip-console --device /dev/ttyUSB0 -b 2000000
    ```

-   Get or Set the light state

    `rpcs.chip.rpc.Lighting.Get()`

    `rpcs.chip.rpc.Lighting.Set(on=True, level=128)`
