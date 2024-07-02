#!/usr/bin/env python3
# Copyright (c) 2021 Project CHIP Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import importlib.metadata
import logging
import os
import pathlib
import re
import shutil
import sys
import subprocess
import platform
import shutil
import toml
import binascii
import configparser
import coloredlogs
import firmware_utils

coloredlogs.install(level='DEBUG')

# Additional options that can be use to configure an `Flasher`
# object (as dictionary keys) and/or passed as command line options.
def any_base_int(s): return int(s, 0)

BOUFFALO_OPTIONS = {
    # Configuration options define properties used in flashing operations.
    'configuration': {
        'chipname': {
            'help': "Bouffalolab chip name",
            'default': None,
            'argparse': {
                'metavar': 'CHIP_NAME',
            }
        },
        'pt': {
            'help': 'Partition table for board for bl iot sdk',
            'default': None,
            'argparse': {
                'metavar': 'PARTITION_TABLE_FILE',
                'type': pathlib.Path
            }
        },
        'dts': {
            'help': 'Device tree file for bl iot sdk',
            'default': None,
            'argparse': {
                'metavar': 'DEVICE_TREE_FILE',
                'type': pathlib.Path
            }
        },
        'xtal': {
            'help': 'XTAL configuration for bl iot sdk',
            'default': None,
            'argparse': {
                'metavar': 'XTAL',
            }
        },
        'port': {
            'help': 'UART port to flash device',
            'default': None,
            'argparse': {
                'metavar': 'PORT',
            }
        },
        'baudrate': {
            'help': 'UART baudrate to flash device',
            'default': None,
            'argparse': {
                'metavar': 'BAUDRATE',
            },
        },
        'sk': {
            'help': 'private key to sign firmware to flash or sign ota image.',
            'default': None,
            'argparse': {
                'metavar': 'path',
                'type': pathlib.Path
            }
        },
        'mfd': {
            'help': 'matter factory data',
            'default': None,
            'argparse': {
                'metavar': 'path',
                'type': pathlib.Path
            }
        },
        'key': {
            'help': 'data key in security engine for matter factory data decryption',
            'default': None,
            'argparse': {
                'metavar': 'key',
            }
        },
        'boot2': {
            'help': 'boot2 image',
            'default': None,
            'argparse': {
                'metavar': 'path',
            }
        },
        'config': {
            'help': 'firmware programming configuration for bouffalo sdk',
            'default': None,
            'argparse': {
                'metavar': 'path',
            }
        },
        'build-ota': {
            'help': 'build ota image',
            'default': None,
            'argparse': {
                'action': 'store_true'
            },
        },
        'vendor-id': {
            'help': 'vendor id passes to ota_image_tool.py ota image if --build_ota present',
            'default': None,
            'argparse': {
                'metavar': 'vendor_id',
                "type": any_base_int
            }
        },
        'product-id': {
            'help': 'product id passes to ota_image_tool.py ota image if --build_ota present',
            'default': None,
            'argparse': {
                'metavar': 'product_id',
                "type": any_base_int
            }
        },
        'version': {
            'help': 'software version (numeric) passes to ota_image_tool.py ota image if --build_ota present',
            'default': None,
            'argparse': {
                'metavar': 'version',
                "type": any_base_int
            }
        },
        'version-str': {
            'help': 'software version string passes to ota_image_tool.py ota image if --build_ota present',
            'default': None,
            'argparse': {
                'metavar': 'version_str',
            }
        },
        'digest-algorithm': {
            'help': 'digest algorithm passes to ota_image_tool.py ota image if --build_ota present',
            'default': None,
            'argparse': {
                'metavar': 'digest_algorithm',
            }
        },
        "min-version": {
            'help': 'minimum software version passes to ota_image_tool.py ota image if --build_ota present',
            'default': None,
            'argparse': {
                'metavar': 'min_version',
                "type": any_base_int
            }
        },
        "max-version": {
            'help': 'maximum software version passes to ota_image_tool.py ota image if --build_ota present',
            'default': None,
            'argparse': {
                'metavar': 'max_version',
                "type": any_base_int
            }
        },
        "release-notes": {
            'help': 'release note passes to ota_image_tool.py ota image if --build_ota present',
            'default': None,
            'argparse': {
                'metavar': 'release_notes',
            }
        }
    },
}

MATTER_ROOT = os.getcwd()

class DictObject:
    def __init__(self, data_dict):
        for key, value in data_dict.items():
            if isinstance(value, dict):
                self.__dict__[key] = DictObject(value)
            else:
                self.__dict__[key] = value

    def __getattr__(self, name):
        return self.__dict__.get(name, None)

    def __setattr__(self, name, value):
        self.__dict__[name] = value

    def __repr__(self):
        return str(self.__dict__)

class Flasher(firmware_utils.Flasher):

    arguments = []
    chip_name = None
    work_dir = None
    isErase = False
    uart_port = None
    mfd = None
    key = None

    args = {}

    # parameters to program firmware for bl iot sdk
    dts_path = None
    xtal_value = None
    boot2_image = None

    bouffalo_sdk_chips = ["bl616"]
    # parameters to process & program firmware for bouffalo sdk
    firmware = None

    def __init__(self, **options):
        super().__init__(platform=None, module=__name__, **options)
        self.define_options(BOUFFALO_OPTIONS)

    def parse_argv(self, argv):
        """Handle command line options."""
        self.argv0 = argv[0]
        self.parser.parse_args(argv[1:], namespace=self.option)


        print ("parse_argv", self.parser)

        self._postprocess_argv()
        return self

    def find_file(self, path_dir, name_partten):

        ret_files = []

        for root, dirs, files in os.walk(path_dir, topdown=False):
            for name in files:
                if re.match(name_partten, name):
                    ret_files.append(os.path.join(path_dir, name))

        return ret_files

    def get_iv(self):

        self.args['iv'] = None
        if not self.mfd or not self.key:
            return None

        with open(self.mfd, 'rb') as f:
            bytes_obj = f.read()

        sec_len = int.from_bytes(bytes_obj[0:4], byteorder='little')
        crc_val_calc = int.from_bytes(bytes_obj[4 + sec_len:4 + sec_len + 4], byteorder='little')
        crc_val = int.from_bytes(bytes_obj[4 + sec_len:4 + sec_len + 4])
        if crc_val_calc != crc_val_calc:
            raise Exception("MFD partition file has invalid format in secured data.")

        if 0 == sec_len:
            return None

        raw_start = 4 + sec_len + 4
        raw_len = int.from_bytes(bytes_obj[raw_start:raw_start + 4], byteorder='little')
        crc_val_calc = binascii.crc32(bytes_obj[raw_start + 4:raw_start + 4 + raw_len])
        crc_val = int.from_bytes(bytes_obj[raw_start + 4 + raw_len:raw_start + 4 + raw_len + 4], byteorder='little')
        if crc_val_calc != crc_val_calc:
            raise Exception("MFD partition file has invalid format in raw data.")

        offset = 0
        while offset < raw_len:
            type_id = int.from_bytes(bytes_obj[raw_start + 4 + offset: raw_start + 4 + offset + 2], byteorder='little')
            type_len = int.from_bytes(bytes_obj[raw_start + 4 + offset + 2:raw_start + 4 + offset + 4], byteorder='little')

            if 0x8001 == type_id:
                iv = bytes_obj[raw_start + 4 + offset + 4:raw_start + 4 + offset + 4 + type_len]
                self.args['iv'] = iv.hex()
                return 

            offset += (4 + type_len)

        if sec_len > 0 and self.key:
            raise Exception("missing AES IV information.")

        return None

    def iot_sdk_prog(self):

        def get_tools():
            flashtool_path = os.environ.get('BOUFFALOLAB_SDK_ROOT') + "/flashtool/BouffaloLabDevCube-v1.8.9"
            bflb_tools_dict = {
                "linux": {"flash_tool": "bflb_iot_tool-ubuntu"},
                "win32": {"flash_tool": "bflb_iot_tool.exe"},
                "darwin": {"flash_tool": "bflb_iot_tool-macos"},
            }

            try:
                flashtool_exe = flashtool_path + "/" + bflb_tools_dict[sys.platform]["flash_tool"]
            except Exception as e:
                raise Exception("Do NOT support {} operating system to program firmware.".format(sys.platform))

            if not os.path.exists(flashtool_exe):
                logging.fatal('*' * 80)
                logging.error('Flashtool is not installed, or environment variable BOUFFALOLAB_SDK_ROOT is not exported.')
                logging.fatal('\tPlease make sure Bouffalo Lab SDK installs as below:')
                logging.fatal('\t\t./third_party/bouffalolab/env-setup.sh')

                logging.fatal('\tPlease make sure BOUFFALOLAB_SDK_ROOT exports before building as below:')
                logging.fatal('\t\texport BOUFFALOLAB_SDK_ROOT="your install path"')
                logging.fatal('*' * 80)
                raise Exception(e)

            return flashtool_exe

        def get_boot_image(config_path, boot2_image):

            boot_image_guess = None

            for root, dirs, files in os.walk(config_path, topdown=False):
                for name in files:
                    if boot2_image:
                        return os.path.join(root, boot2_image)
                    else:
                        if name == "boot2_isp_release.bin":
                            return os.path.join(root, name)
                        elif not boot_image_guess and name.find("release") >= 0:
                            boot_image_guess = os.path.join(root, name)

            return boot_image_guess

        def get_dts_file(config_path, xtal_value, chip_name):

            for root, dirs, files in os.walk(config_path, topdown=False):
                for name in files:
                    if chip_name == 'bl616':
                        if name.find("bl_factory_params_IoTKitA_auto.dts") >= 0:
                            return os.path.join(config_path, name)
                    elif chip_name == 'bl702':
                        if name.find("bl_factory_params_IoTKitA_32M.dts") >= 0:
                            return os.path.join(config_path, name)
                    else:
                        if name.find(xtal_value) >= 0:
                            return os.path.join(config_path, name)
            return None


        def get_prog_cmd():
            pass

        flashtool_exe = get_tools()

        if not self.dts_path and self.xtal_value:
            chip_config_path = os.path.join(flashtool_path, "chips", self.chip_name, "device_tree")
            dts_path = get_dts_file(chip_config_path, self.xtal_value, self.chip_name)
            self.arguments.append("--dts")
            self.arguments.append(self.dts_path)

        if self.boot2_image:
            chip_config_path = os.path.join(flashtool_path, "chips", self.chip_name, "builtin_imgs")
            self.boot2_image = get_boot_image(chip_config_path, self.boot2_image)
            self.arguments.append("--boot2")
            self.arguments.append(self.boot2_image)
        else:
            if self.option.erase:
                self.arguments.append("--erase")

            if self.chip_name in {"bl602", "bl702", "bl616"}:
                chip_config_path = os.path.join(flashtool_path, "chips", self.chip_name, "builtin_imgs")
                self.boot2_image = get_boot_image(chip_config_path, self.boot2_image)
                self.arguments.append("--boot2")
                self.arguments.append(self.boot2_image)

        self.arguments = [flashtool_exe] + self.arguments
            
        os.chdir(self.work_dir)
        logging.info("Arguments {}".format(self.arguments))
        process = subprocess.Popen(self.arguments, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        while process.poll() is None:
            line = process.stdout.readline().decode('utf-8').rstrip()
            if line:
                logging.info(line)

    def bouffalo_sdk_prog(self):

        def int_to_lhex(intvalue):
            l = ((intvalue & 0xff000000) >> 24) | ((intvalue & 0xff0000) >> 8) 
            l |= ((intvalue & 0xff00) << 8) | ((intvalue & 0xff) << 24)
            
            return "%08x" % l

        def get_tools():
            bflb_tools = os.path.join(MATTER_ROOT, "third_party/bouffalolab/bouffalo_sdk/tools/bflb_tools")
            bflb_tools_dict = {
                "linux": {"fw_proc": "bflb_fw_post_proc/bflb_fw_post_proc-ubuntu", "flash_tool": "bouffalo_flash_cube/BLFlashCommand-ubuntu"},
                "win32": {"fw_proc": "bflb_fw_post_proc/bflb_fw_post_proc.exe", "flash_tool": "bouffalo_flash_cube/BLFlashCommand.exe"},
                "darwin": {"fw_proc": "bflb_fw_post_proc/bflb_fw_post_proc-macos", "flash_tool": "bouffalo_flash_cube/BLFlashCommand-macos"},
            }

            try:
                fw_proc_exe = os.path.join(bflb_tools, bflb_tools_dict[sys.platform]["fw_proc"])
                flashtool_exe = os.path.join(bflb_tools, bflb_tools_dict[sys.platform]["flash_tool"])
            except Exception as e:
                raise Exception("Do NOT support {} operating system to program firmware.".format(sys.platform))
            
            if not os.path.exists(flashtool_exe) or not os.path.exists(fw_proc_exe):
                logging.fatal('*' * 80)
                logging.error("Expecting tools as below:")
                logging.error(fw_proc_exe)
                logging.error(flashtool_exe)
                raise Exception("Flashtool or fw tool doesn't contain in SDK")

            return fw_proc_exe, flashtool_exe

        def prog_config(configDir, output, isErase = False):

            partition_file = self.find_file(configDir, r'^partition.+\.toml$')
            if len(partition_file) != 1:
                raise Exception("No partition file or one more partition file found.")

            partition_file = partition_file[0]
            with open(partition_file, 'r') as file:
                partition_config = toml.load(file)

            part_addr0 = partition_config["pt_table"]["address0"]
            part_addr1 = partition_config["pt_table"]["address1"]

            config = configparser.ConfigParser()

            config.add_section('cfg')
            config.set('cfg', 'erase', '2' if isErase else '1')
            config.set('cfg', 'skip_mode', '0x0, 0x0')
            config.set('cfg', 'boot2_isp_mode', '0')

            config.add_section('boot2')
            config.set('boot2', 'filedir', os.path.join(self.work_dir, "boot2*.bin"))
            config.set('boot2', 'address', '0x000000')

            config.add_section('partition')
            config.set('partition', 'filedir', os.path.join(self.work_dir, "partition*.bin"))
            config.set('partition', 'address', hex(part_addr0))

            config.add_section('partition1')
            config.set('partition1', 'filedir', os.path.join(self.work_dir, "partition*.bin"))
            config.set('partition1', 'address', hex(part_addr1))

            config.add_section('FW')
            config.set('FW', 'filedir', self.args["firmware"])
            config.set('FW', 'address', '@partition')

            if self.mfd:
                config.add_section("MFD")
                config.set('MFD', 'filedir', self.mfd)
                config.set('MFD', 'address', '@partition')

            with open(output, 'w') as configfile:
                config.write(configfile)

        def exe_proc_cmd(fw_proc_exe):

            os.system("rm -rf {}/ota_images".format(self.work_dir))

            boot2_proc_cmd = None
            fw_proc_cmd = [
                fw_proc_exe,
                "--chipname", self.args["chipname"],
                "--brdcfgdir", os.path.join(self.work_dir, "config"),
                "--imgfile", self.args["firmware"],
            ]

            if self.args["sk"]:
                fw_proc_cmd += [
                    "--privatekey", self.args["sk"],
                ]

            if self.args["key"]:
                lock0 = int_to_lhex((1 << 30) | (1 << 20))
                lock1 = int_to_lhex((1 << 25) | (1 << 15))
                fw_proc_cmd += [
                    "--edata", "0x80,{};0x7c,{};0xfc,{}".format(self.args["key"], lock0, lock1)
                ]

            logging.info("firmware process command: {}".format(" ".join(fw_proc_cmd)))
            process = subprocess.Popen(fw_proc_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            while process.poll() is None:
                line = process.stdout.readline().decode('utf-8').rstrip()
                if line:
                    logging.info(line)

            os.system("mkdir -p {}/ota_images".format(self.work_dir))
            os.system("mv {}/*.ota {}/ota_images/".format(self.work_dir, self.work_dir))

        def exe_prog_cmd(flashtool_exe):
            prog_cmd = [
                flashtool_exe,
                "--chipname", self.args["chipname"],
                "--baudrate", str(self.args["baudrate"]),
                "--config",self.args["config"]
            ]

            if self.args["sk"] or (self.args["key"] and self.args["iv"]):
                prog_cmd += [
                    "--efuse", os.path.join(self.work_dir, "efusedata.bin")
                ]

            if self.args["port"]:
                prog_config(os.path.join(self.work_dir, "config"), os.path.join(self.work_dir, "flash_prog_cfg.ini"), self.option.erase)

                prog_cmd += [
                    "--port", self.args["port"],
                ]

                logging.info("firwmare programming: {}".format(" ".join(prog_cmd)))
                process = subprocess.Popen(prog_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                while process.poll() is None:
                    line = process.stdout.readline().decode('utf-8').rstrip()
                    if line:
                        logging.info(line)

        fw_proc_exe, flashtool_exe = get_tools()
        os.chdir(self.work_dir)

        self.get_iv()

        exe_proc_cmd(fw_proc_exe)
        exe_prog_cmd(flashtool_exe)

    def gen_ota_image(self):
        sys.path.insert(0, os.path.join(MATTER_ROOT, 'src', 'app'))
        import ota_image_tool

        bflb_ota_images = self.find_file(os.path.join(self.work_dir, "ota_images"), r".+\.ota$")
        if len(bflb_ota_images) == 0:
            raise Exception("No bouffalo lab OTA image found.")

        ota_image_cfg_list = [
            "vendor_id",
            "product_id",
            "version",
            "version_str",
            "digest_algorithm",
            "min_version",
            "max_version",
            "release_notes",
        ]
        ota_image_cfg = {}
        for k in ota_image_cfg_list:
            if self.args[k] is not None:
                ota_image_cfg[k] = self.args[k]
        ota_image_cfg = DictObject(ota_image_cfg)

        for img in bflb_ota_images:
            ota_image_cfg.input_files = [img]
            ota_image_cfg.output_file = img + ".matter"
            ota_image_tool.validate_header_attributes(ota_image_cfg)
            ota_image_tool.generate_image(ota_image_cfg)

            self.log(0, 'Matter OTA image generated:', ota_image_cfg.output_file)

    def verify(self):
        """Not supported"""
        self.log(0, "Verification is done after image flashed.")

    def reset(self):
        """Not supported"""
        self.log(0, "Reset is triggered automatically after image flashed.")

    def actions(self):
        """Perform actions on the device according to self.option."""
        self.log(3, 'Options:', self.option)

        is_for_ota_image_building = None
        is_for_programming = False
        has_private_key = False
        has_public_key = False
        ota_output_folder = None
        options_keys = BOUFFALO_OPTIONS["configuration"].keys()

        if platform.machine() not in ["x86_64"]:
            raise Exception("Only support x86_64 CPU machine to program firmware.")

        if self.option.reset:
            self.reset()
        if self.option.verify_application:
            self.verify()

        self.args = dict(vars(self.option))
        self.args["application"] = os.path.join(os.getcwd(), str(self.args["application"]))
        self.work_dir = os.path.dirname(self.args["application"])
        if self.args["sk"]:
            self.args["sk"] = str(self.args["sk"])

        self.args["application"] = os.path.join(os.getcwd(), str(self.args["application"]))
        self.args["firmware"] = str(pathlib.Path(self.args["application"]).with_suffix(".bin"))
        shutil.copy2(self.args["application"], self.args["firmware"])

        for (key, value) in dict(vars(self.option)).items():

            if key == "application":
                continue
            elif key == "boot2":
                boot2_image = value
                continue
            elif key == "mfd":
                if value:
                    self.mfd = os.path.join(os.getcwd(), str(value))
                continue
            elif key == "key":
                self.key = value
                continue
            elif key in options_keys:
                pass
            else:
                continue

            if value:
                if value is True:
                    arg = ("--{}".format(key)).strip()
                elif isinstance(value, pathlib.Path):
                    arg = ("--{}={}".format(key, os.path.join(os.getcwd(), str(value)))).strip()
                else:
                    arg = ("--{}={}".format(key, value)).strip()

                self.arguments.append(arg)

            if key == "chipname":
                self.chip_name = value
            elif key == "xtal":
                self.xtal_value = value
            elif key == "dts":
                self.dts_path = value
            elif "port" == key:
                if value:
                    is_for_programming = True
                    self.uart_port = value
            elif "build" == key:
                if value:
                    is_for_ota_image_building = True
            elif "sk" == key:
                if value:
                    has_private_key = True

        if is_for_ota_image_building and is_for_programming:
            logging.error("ota imge build can't work with image programming")
            raise Exception("Wrong operation.")

        if is_for_ota_image_building == "ota_sign" and (not has_private_key or not has_public_key):
            logging.error("Expecting key pair to sign OTA image.")
            raise Exception("Wrong key pair.")

        if ota_output_folder:
            if os.path.exists(ota_output_folder):
                shutil.rmtree(ota_output_folder)
            os.mkdir(ota_output_folder)

        if self.args["build_ota"]:
            if self.args["port"]:
                raise Exception("Do not generate OTA image with firmware programming.")

        if self.chip_name in self.bouffalo_sdk_chips:
            self.bouffalo_sdk_prog()
        else:
            self.iot_sdk_prog()

        if self.args["build_ota"]:
            self.gen_ota_image()

        if ota_output_folder:
            ota_images = os.listdir(ota_output_folder)
            for img in ota_images:
                if img not in ['FW_OTA.bin.xz.hash']:
                    os.remove(os.path.join(ota_output_folder, img))

        return self

if __name__ == '__main__':

    sys.argv[0] = re.sub(r'(-script\.pyw|\.exe)?$', '', sys.argv[0])

    sys.exit(Flasher().flash_command(sys.argv))
