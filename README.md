# Smart Presenter Glove
This repository contains client code for streaming, pre-/postprocessing, simple motion detection with the IMU data from the M5 Stick C Plus. This client application was built for a handsfree, ubiquitous and wearable computing project, "Smart Presenter", which allows for intuitive gestures to control Mouse and Keyboard events. Workflow, process and ideation of this project can be found on this [Miro board](https://miro.com/app/board/uXjVKQi3msY=/?share_link_id=143508401314). Included are a complete set of presentation slides and a small image gallery.

The server code can be found [here](https://github.com/01604987/BTD-server).

# Requirements

## Hardware

- [M5 Stick C Plus ESP32-PICO Mini IoT Development Kit](https://shop.m5stack.com/products/m5stickc-plus-esp32-pico-mini-iot-development-kit) ``required``
- Middle button on the M5 Stick ``for testing``
- Right button on the M5 Stick ``for testing``
- Glove with touch capacitive fingertips
- Wires connecting touch capacitive fingertips to GPIO pins
- Pull up resistor: 10k ohm
- GPIO 26 (index finger)
- GPIO 36 (middle finger)
- 3.3v
- GND

A simple wiring diagram can also be found in our [Miro board](https://miro.com/app/board/uXjVKQi3msY=/), with proper setup of pull-up resistors that are required for the configured buttons on the M5 Stick C Plus.

## ESP IDF

This client was developed on the esp-idf. Please make sure to use esp-idf version ``5.3.0`` as compatibility with different versions cannot be guaranteed.

For a guide on how to setup esp idf with visual studio code, please follow these [instructions](https://github.com/espressif/vscode-esp-idf-extension/blob/master/docs/tutorial/install.md).

## Server
This client will only run with the correct setup of the accompanying [server found here](https://github.com/01604987/BTD-server). Please follow the instructions provided in the README.md of the server repository. 


# How to install the Client
- Have esp-idf installed for visual studio code, or use the system wide esp idf installation
- Clone this repository and open it in vscode
- For initial client setup, use `ctrl + shift + p` to open the vscode context menu and search for ``SDK configuration editor (Menuconfig)``
- Scroll down to `NETWORK Configuration`, choose IPv4 and input the [server](https://github.com/01604987/BTD-server?tab=readme-ov-file) ip and port: 
    - ``IP``: If the server ip is unknown, check the ip of the host machine running the server with the following commands:
        - Windows: ``ipconfig`` <br>
        - Mac: ``ifconfig | grep "inet " | grep -v 127.0.0.1`` <br>
        This will tell you which ip the host computer is currently assigned to. For a better experience, we recommend setting up a static ip for the host computer running the server.
    - ``PORT``: 5500 (by default)

- Scroll down to `Example Connection Configuration` and input your wifi ssid and password. Make sure the host running the server is connected under the same network.
- With `ctrl + shift + p` search for Device Configuration.
    - Configure the ``Device Target``: 
        - choose the current project
        - choose eps32 
        - choose ESP32 chip (via ESP USB Bridge)
    - Configure the ``Device Port``:
        - choose the usb port which the M5 stick C plus is currently connected to. The correct port should be highlighted by default.
    - Configure the ``Flash Baud Rate``: ``115200``
- Don't forget to save the configuration. 
- Full clean the project (or delete build folder) by searching ``ESP-IDF: Full Clean Project`` or clicking the small trashcan icon at the bottom of vscode. It is generally a good idea to full clean the build if substantial changes has been made in IDF configuration, which may lead to build errors.
- ``ESP-IDF: Build your project`` or the small icon bellow to build the project
- After the build has succeeded, use ``EPS-IDF: Flash your project`` or the small icon bellow to flash the build onto the M5 stick 

# How to use the Client
## Configure for Testing (optional)
This project is designed to be used with a glove that has touch capacitive fingertips, which are correctly wired to the GPIO pins found on top of the M5 Stick. Simple documentations on the wiring and glove can be found on the provided [Miro Board](https://miro.com/app/board/uXjVKQi3msY=/). <br>
For testing purposes this client can also be controlled with the 2 integrated buttons found on the M5 Stick C plus, after changing parameters in ``components/buttons/buttons.c`` & ``components/buttons/buttons.h`` <br>
In ``buttons.h`` simply uncomment these 2 following lines:
```
//#define BUTTON_PIN_INDEX GPIO_NUM_37
//#define BUTTON_PIN_MIDDLE GPIO_NUM_39
```
and comment out these following lines:
```
#define BUTTON_PIN_INDEX GPIO_NUM_25
#define BUTTON_PIN_MIDDLE GPIO_NUM_26
```

In ``buttons.c`` in the ``init_button()`` function change the following parameters from 0 to 1
```
io_conf_index.pull_up_en = 1;
io_conf_middle.pull_up_en = 1;
```

Rebuild and flash the project. The server should now be controllable through the integrated buttons on the M5 Stick C Plus.

```
Index finger = middle button
Middle finger = right side button    
```

## Controlling the Host
- After starting the server, wait for the client to connect to the network access point you have configured and to establish a connection to the server.
- Upon successful connection, the server should display the ip address of the client on the terminal while the screen of the M5 Stick C Plus should display ``Initialized``. 

Upon successful connection, the following functionalities can be applied:
``The application can also be easily tested without a glove. Please see the instructions above.``
- Keyboard ``left``/``right``: abrupt left / right swipe of the hand without any buttons pressed
- Mouse ``lmb click``: short tap of index and thumb
- Mouse ``lmb double click``: short double tap of index and thumb
- Mouse ``cursor control``: holding index and thumb with pitch and roll of hand in horizontal position
- Mouse ``drag & drop with cursor``: double tap and hold index and thumb with pitch and roll of hand in horizontal position
- Mouse ``rmb click``: short tap of middle finger and thumb
- Keyboard ``l + ctrl``: double tap of middle finger and thumb for entering/exiting presentation mode for pdf slides
- Hostmachine ``volume control``: hold middle finger and thumb with rotating wrist left/right 
- Mouse/Keyboard ``ctrl + scroll wheel``: double tap and hold middle finger and thumb with rotating wrist up/down

``note: All functionalities are only guaranteed on windows machines running Windows 10/11. On Apple Macs, volume control is not implemented.``




# Notes
- The c_cpp_properties.json file within the .vscode folder is explicitly not ignored from remote due to the required paths for intellisense not being properly auto generated.
