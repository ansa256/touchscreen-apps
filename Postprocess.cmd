arm-none-eabi-objcopy -O ihex TouchScreenApps.elf  "TouchScreenApps.hex"
arm-none-eabi-objdump -h -S TouchScreenApps.elf > "TouchScreenApps.lst"
arm-none-eabi-size  --format=berkeley TouchScreenApps.elf