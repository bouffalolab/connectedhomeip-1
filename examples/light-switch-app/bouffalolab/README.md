## Build
- BL61X
   ```shell
   ./scripts/build/build_examples.py --target bouffalolab-bl616dk-light-switch-wifi build
   ```

## Commissioning over BLE
-   Light

      ```shell
      ./chip-tool pairing ble-wifi <light_node_id> <wifi_ssid> <wifi_passwd> 20202021 3840
      ```

-   BL706

      ```shell
      ./chip-tool pairing ble-wifi <switch_node_id> hex:<thread_operational_dataset> 20202021 3840
      ```

## Testing the example

-   After successful commissioning, use the chip-tool to write the ACL in
    Lighting device to allow access from Lighting-switch device and chip-tool.

        $ ./out/debug/chip-tool accesscontrol write acl '[{"fabricIndex": 1, "privilege": 5, "authMode": 2, "subjects": [112233], "targets": null },{"fabricIndex": 1, "privilege": 3, "authMode": 2, "subjects": [<LIGHT SWITCH NODE ID>], "targets": null }]' <LIGHTING APP NODE ID> 0

-   After successful commissioning, use the chip-tool for binding in
    Lighting-switch.

        $ ./out/debug/chip-tool binding write binding '[{"fabricIndex": 1, "node":<LIGHTING APP NODE ID>, "endpoint":1, "cluster":6}]' <LIGHT SWITCH NODE ID> 1