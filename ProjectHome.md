# Firmware Modification Kit #

This kit is a collection of scripts and utilities to extract and rebuild linux based firmware images.

  1. Extract a firmware image into its component parts
  1. User makes desired modification to the firmware's file system or web UI (webif)
  1. Rebuild firmware
  1. Flash modified firmware onto device and brick it (ha)

WARNING: You are going to brick your device by using this kit (maybe not, but better to say you will). Brick means to effectively turn into a non-functional 'brick'. Recovery is sometimes possible without hardware modifications. Sometimes it requires hardware mods (e.g. serial or JTAG headers soldered onto the PCB). Sometimes it just isn't feasible, or would cost more in total recovery cost than the unit is worth.

Do NOT use this kit if you are not prepared to have your router bricked!

EULA: By downloading or using this kit, you agree to accept liability for consequences of use or misuse of the Firmware Mod Kit. These include the bricking of your device. The authors of this kit have duly warned you. This kit is only for embedded systems software engineers.

Please see the Wiki Documentation here: https://code.google.com/p/firmware-mod-kit/wiki/Documentation?tm=6