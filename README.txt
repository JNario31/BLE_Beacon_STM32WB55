This project was created so that I can learn how to
use Zephyr RTOS, I am using the STM32 Nucleo-WB55RG
and a BMP180 temperature/pressure sensor. Here I am
putting notes for me to remember and reference when I
create new projects, and also to better understand how
to navigate through this complicated system.

The Nucleo-WB55RG uses a Dual-chip configuration, one
running the application and host, the other with the
controller and radio hardware.

Devicetree Notes:
The reason the i2c1 node does not require the keyword
compatible is because it references an existing node in
zephyr/dts/arm/st/wb/stm32wb.dtsi.

Compatible is only required when a new node is created.
Zephyr uses the string "bosch,bmp180" to match the node to
a driver DT_DRV_COMPAT bosch_bmp180

Mental model to remember:
    controllers already exist -> enable them
    peripherals are new -> describe them

bmp180 is a child node, it is connected to the parent bus,
i2c1. The @77 is a devicetree naming convention to show the
address of the device.

Understanding i2c1 node:
	i2c1: i2c@40005400 {
			compatible = "st,stm32-i2c-v2";
			clock-frequency = <I2C_BITRATE_STANDARD>;
			#address-cells = <1>;
			#size-cells = <0>;
			reg = <0x40005400 0x400>;
			clocks = <&rcc STM32_CLOCK(APB1, 21)>;
			interrupts = <30 0>, <31 0>;
			interrupt-names = "event", "error";
			status = "disabled";
		};

    Compatible tells Zephyr which driver to bind
    clock-frequency sets the bus speed, check st,stm32-i2c-v2.yaml
    reg, this is the MCU peripheral address, base address is 
    0x40005400 and its 0x400 bytes, the driver uses this to map i2c
    registers, r/w control/status.
    clocks tells Zephyr which clock domain feeds the peripheral
    status is originally disabled, thats why we enable it in our
    devicetree overlay.




To build this project run: 
west build -p always -b nucleo_wb55rg -- -DDTC_OVERLAY_FILE=boards/nucleo_wb55rg.overlay

to flash:
west flash

project structure:
/:
    applications:
        ble_beacon
    zephyr
    bootloader
    modules
    tools
    venv