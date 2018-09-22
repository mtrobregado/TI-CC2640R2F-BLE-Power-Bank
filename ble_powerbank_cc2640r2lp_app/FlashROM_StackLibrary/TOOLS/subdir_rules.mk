################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
build-874629903:
	@$(MAKE) --no-print-directory -Onone -f TOOLS/subdir_rules.mk build-874629903-inproc

build-874629903-inproc: ../TOOLS/app_ble.cfg
	@echo 'Building file: "$<"'
	@echo 'Invoking: XDCtools'
	"C:/ti/xdctools_3_50_07_20_core/xs" --xdcpath="C:/ti/simplelink_cc2640r2_sdk_1_50_00_58/source;C:/ti/simplelink_cc2640r2_sdk_1_50_00_58/kernel/tirtos/packages;C:/ti/simplelink_cc2640r2_sdk_1_50_00_58/source/ti/blestack;" xdc.tools.configuro -o configPkg -t ti.targets.arm.elf.M3 -p ti.platforms.simplelink:CC2640R2F -r release -c "C:/ti/ccsv8/tools/compiler/ti-cgt-arm_18.1.2.LTS" --compileOptions "-mv7M3 --code_state=16 -me -Ooff --opt_for_speed=0 --include_path=\"C:/ti/ccsv8workspace/CC2640R2F_LaunchpadV150/ble_powerbank_cc2640r2lp_app\" --include_path=\"C:/ti/ccsv8workspace/CC2640R2F_LaunchpadV150/ble_powerbank_cc2640r2lp_app/Application\" --include_path=\"C:/ti/ccsv8workspace/CC2640R2F_LaunchpadV150/ble_powerbank_cc2640r2lp_app/Startup\" --include_path=\"C:/ti/ccsv8workspace/CC2640R2F_LaunchpadV150/ble_powerbank_cc2640r2lp_app/PROFILES\" --include_path=\"C:/ti/ccsv8workspace/CC2640R2F_LaunchpadV150/ble_powerbank_cc2640r2lp_app/Include\" --include_path=\"C:/ti/simplelink_cc2640r2_sdk_1_50_00_58/source/ti/blestack/controller/cc26xx_r2/inc\" --include_path=\"C:/ti/simplelink_cc2640r2_sdk_1_50_00_58/source/ti/blestack/inc\" --include_path=\"C:/ti/simplelink_cc2640r2_sdk_1_50_00_58/source/ti/blestack/rom\" --include_path=\"C:/ti/simplelink_cc2640r2_sdk_1_50_00_58/source/ti/blestack/common/cc26xx\" --include_path=\"C:/ti/simplelink_cc2640r2_sdk_1_50_00_58/source/ti/blestack/icall/inc\" --include_path=\"C:/ti/simplelink_cc2640r2_sdk_1_50_00_58/source/ti/blestack/profiles/dev_info\" --include_path=\"C:/ti/simplelink_cc2640r2_sdk_1_50_00_58/source/ti/blestack/profiles/roles/cc26xx\" --include_path=\"C:/ti/simplelink_cc2640r2_sdk_1_50_00_58/source/ti/blestack/profiles/roles\" --include_path=\"C:/ti/simplelink_cc2640r2_sdk_1_50_00_58/source/ti/blestack/profiles/simple_profile/cc26xx\" --include_path=\"C:/ti/simplelink_cc2640r2_sdk_1_50_00_58/source/ti/blestack/profiles/simple_profile\" --include_path=\"C:/ti/simplelink_cc2640r2_sdk_1_50_00_58/source/ti/blestack/target\" --include_path=\"C:/ti/simplelink_cc2640r2_sdk_1_50_00_58/source/ti/blestack/hal/src/target/_common\" --include_path=\"C:/ti/simplelink_cc2640r2_sdk_1_50_00_58/source/ti/blestack/hal/src/target/_common/cc26xx\" --include_path=\"C:/ti/simplelink_cc2640r2_sdk_1_50_00_58/source/ti/blestack/hal/src/inc\" --include_path=\"C:/ti/simplelink_cc2640r2_sdk_1_50_00_58/source/ti/blestack/heapmgr\" --include_path=\"C:/ti/simplelink_cc2640r2_sdk_1_50_00_58/source/ti/blestack/icall/src/inc\" --include_path=\"C:/ti/simplelink_cc2640r2_sdk_1_50_00_58/source/ti/blestack/osal/src/inc\" --include_path=\"C:/ti/simplelink_cc2640r2_sdk_1_50_00_58/source/ti/blestack/services/src/saddr\" --include_path=\"C:/ti/simplelink_cc2640r2_sdk_1_50_00_58/source/ti/blestack/services/src/sdata\" --include_path=\"C:/ti/simplelink_cc2640r2_sdk_1_50_00_58/source/ti/devices/cc26x0r2\" --include_path=\"C:/ti/ccsv8/tools/compiler/ti-cgt-arm_18.1.2.LTS/include\" --define=DeviceFamily_CC26X0R2 --define=BOARD_DISPLAY_USE_LCD=1 --define=BOARD_DISPLAY_USE_UART=0 --define=BOARD_DISPLAY_USE_UART_ANSI=1 --define=CC2640R2_LAUNCHXL --define=WATCHDOG_ENx --define=CC26XX --define=CC26XX_R2 --define=ICALL_EVENTS --define=ICALL_JT --define=ICALL_LITE --define=ICALL_MAX_NUM_ENTITIES=6 --define=ICALL_MAX_NUM_TASKS=3 --define=ICALL_STACK0_ADDR --define=POWER_SAVING --define=RF_SINGLEMODE --define=STACK_LIBRARY --define=USE_ICALL --define=xdc_runtime_Assert_DISABLE_ALL --define=xdc_runtime_Log_DISABLE_ALL -g --c99 --gcc --diag_warning=225 --diag_wrap=off --display_error_number --gen_func_subsections=on --abi=eabi " "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

configPkg/linker.cmd: build-874629903 ../TOOLS/app_ble.cfg
configPkg/compiler.opt: build-874629903
configPkg/: build-874629903


