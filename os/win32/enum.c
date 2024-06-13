#include <windows.h>
#include <setupapi.h>
#include <devguid.h>
#include <stdio.h>

#pragma comment(lib, "setupapi.lib")

BYTE ListSerialPorts(WORD vid, WORD pid) {
    HDEVINFO deviceInfoSet;
    SP_DEVINFO_DATA deviceInfoData;
    DWORD i;
    char deviceName[256];
    char deviceInstanceId[256];
    char *vidstr, *pidstr, *comstr;
    unsigned short _vid, _pid;
    unsigned char com = 0;

    // Get the device information set for all present devices
    deviceInfoSet = SetupDiGetClassDevsA(&GUID_DEVCLASS_PORTS, NULL, NULL, DIGCF_PRESENT);
    if (deviceInfoSet == INVALID_HANDLE_VALUE) {
        /* printf("Error in SetupDiGetClassDevs\n"); */
        return 0;
    }

    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    for (i = 0; SetupDiEnumDeviceInfo(deviceInfoSet, i, &deviceInfoData); i++) {
        // Get the device name
        if (SetupDiGetDeviceRegistryPropertyA(
            deviceInfoSet,
            &deviceInfoData,
            SPDRP_FRIENDLYNAME,
            NULL,
            (PBYTE)deviceName,
            sizeof(deviceName),
            NULL
        )) {
            /* printf("Serial Port: %s\n", deviceName); */

            // Get the device instance ID to check for VID and PID
            if (SetupDiGetDeviceInstanceIdA(deviceInfoSet, &deviceInfoData, deviceInstanceId, sizeof(deviceInstanceId), NULL)) {
                if (strstr(deviceInstanceId, "VID_") && strstr(deviceInstanceId, "PID_")) {
                    /* printf("  Device Instance ID: %s\n", deviceInstanceId); */

                    // Extract and print VID and PID
                    vidstr = strstr(deviceInstanceId, "VID_");
                    pidstr = strstr(deviceInstanceId, "PID_");
                    
                    /* printf("  VID: %.4s, PID: %.4s\n", vidstr + 4, pidstr + 4); */

                    if (vidstr && pidstr) {
                        _vid = (unsigned short)strtol(vidstr + 4, NULL, 16);
                        _pid = (unsigned short)strtol(pidstr + 4, NULL, 16);
                        if (_vid == vid && _pid == pid) {
                            comstr = strstr(deviceName, "(COM");
                            com  = (unsigned char)strtol(comstr + 4, NULL, 10);
                            break;
                        }
                    }
                }
            }
        }
    }

    if (GetLastError() != NO_ERROR && GetLastError() != ERROR_NO_MORE_ITEMS) {
        /* printf("Error in SetupDiEnumDeviceInfo\n"); */
    }

    SetupDiDestroyDeviceInfoList(deviceInfoSet);

    return com;
}