# GhostLock 5.10

This is a kernel root exploit for the Amazon Fire Max 11 (`sunstone`), adapted from CVE-2026-43499 in [CyberMeowfia](https://github.com/NebuSec/CyberMeowfia).

## Supported devices

Only the Amazon Fire Max 11 is supported for now. The exploit works on Fire OS 8.

The following table lists the FireOS versions that have been tested with this exploit:

| Fire OS | Build | Incremental | Kernel |
|---|---|---|---|
| 8.3.3.8 | RS8338.3339N | `0030132734852` | `-ge7a6aa0ea53f` |
| 8.3.3.7 | RS8337.3186N | `0029461607044` | `-gee3b4a65f51d` |
| 8.3.3.6 | RS8336.3103N | `0028656279428` | `-gda010c50409f` |
| 8.3.3.5 | RS8335.2972N | `0027716721796` | `-g7ec28efd738a` |
| 8.3.3.3 | RS8333.2734N | `0026441592452` | `-g76402f9d634d` |
| 8.3.3.2 / 8.3.3.1 / 8.3.3.0 / 8.3.2.7 | RS8332.3115N | `0025837710212` | `-gf66ba269681b` |
| 8.3.2.4 | RS8324.2314N | `0023153150596` | `-g82c06325e9f8` |
| 8.3.2.2 | RS8322.2053N | `0022045787524` | `-g89f110867b4a` |
| 8.3.2.1 | RS8321.1924N | `0021911536772` | `-g096b58f0a5d7` |
| 8.3.2.0 | RS8320.1807N | `0021777289092` | `-g66ce35ca5a96` |
| 8.3.1.9 | RS8319.1664N | `0021508817028` | `-gec87eda1378d` |


## Building

Requires GNU Make and Android NDK r29.

```sh
make
```

Or build a distributable zip, which handles everything for you:

```sh
./makedist.sh
```

The result is written to `dist/ghostlock-<date>.zip`.

## Usage

```sh
./root.sh
```

On Windows, run `root.bat`.

The script reboots the device, runs the exploit, and drops you into a root shell when it finishes. If it fails, it will automatically retry up to 8 times.

If successful, you will see a root shell prompt like this:

```
root@sunstone:/ #
```

On newer versions of Fire OS, Amazon added KASLR, so it might take a few minutes to fully run. Be patient. Once it is up, `su` works from any `adb shell` until the next reboot.

There is also a bind shell on `127.0.0.1:9999`, kept open for as long as root lasts. The easiest way in is to run `nc` on the device itself:

```sh
adb shell -t "nc 127.0.0.1 9999"
```

Or forward the port and connect from the host:

```sh
adb forward tcp:9999 tcp:9999
stty raw -echo; nc 127.0.0.1 9999; stty sane
```

> [!CAUTION]
> While you are root, our device still uses dm-verity, so any modification to the system/vendor/etc. partitions will result in a brick. Damaging critical partitions such as LK, TEE, or Preloader will also result in a brick. Use this exploit at your own risk.

## OTAs 
OTA updates are disabled automatically as soon as root succeeds. A single OTA can move you to a build that is not supported here, or that fixes the exploit.

When running the exploit, look for the following lines in the output to confirm that OTAs are disabled:
```
OTA: com.amazon.device.software.ota disabled
OTA: com.amazon.kindle.otter.oobe.forced.ota disabled
```

If either says `not disabled`, run it yourself from the root shell:

```sh
pm disable com.amazon.device.software.ota
pm disable com.amazon.kindle.otter.oobe.forced.ota
sync
```

This survives reboots but not a factory reset. If you wish to enable them back, spawn a root shell and run:

```sh
pm enable com.amazon.device.software.ota
pm enable com.amazon.kindle.otter.oobe.forced.ota
sync
```

## Credits

- [CyberMeowfia](https://github.com/NebuSec/CyberMeowfia): original IonStack (CVE-2026-43499) exploit
- [IonStackQuest3](https://github.com/F-19-F/IonStackQuest3): first public port for 5.10 kernels