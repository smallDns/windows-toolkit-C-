# Windows Toolkit (C)

Port em **C puro (WinAPI)** do [windows-toolkit](https://github.com/smallDns/windows-toolkit), originalmente em Python.
Mesmo menu, mesmas telas e mesmos relatórios (TXT + JSON) — sem Python e sem `psutil`, em um único `.exe`.

## Recursos

- Versão do Windows, arquitetura, hostname, usuário e horário de boot
- Modelo da CPU, núcleos físicos/lógicos, uso e frequência
- RAM e memória virtual (swap)
- Partições e uso de disco
- Adaptadores de rede, endereços IPv4/IPv6 e contadores de tráfego
- Lista de processos por consumo de RAM (somente leitura)
- Hardware (consultas PowerShell, best-effort)
- Diagnósticos básicos (disco, RAM, CPU, DNS, ping)
- Relatórios em TXT e JSON na pasta `reports/`

## Requisitos

- Windows 10 ou 11
- Um compilador C: **MinGW-w64** (recomendado o toolchain **UCRT64** do MSYS2) ou **MSVC**

## Compilando

MinGW-w64 (CMD, PowerShell ou MSYS2):

```powershell
mingw32-make
.\windows-toolkit.exe
```

MSVC ("x64 Native Tools Command Prompt"):

```bat
mkdir build
build.bat
windows-toolkit.exe
```

Ou, sem make: 

```powershell
gcc -std=gnu11 -O2 -Iinclude -D_WIN32_WINNT=0x0A00 -o windows-toolkit.exe src/*.c -liphlpapi -lws2_32 -lpsapi -lpowrprof -ladvapi32
```

## Estrutura

```text
windows-toolkit-c/
├── Makefile / build.bat
├── include/            headers (um por módulo)
└── src/
    ├── main.c          menu principal           (main.py)
    ├── sysinfo.c       informações do sistema   (core/system.py)
    ├── cpu.c           CPU                      (core/cpu.py)
    ├── meminfo.c       RAM / swap               (core/memory.py)
    ├── disk.c          discos                   (core/disk.py)
    ├── network.c       rede                     (core/network.py)
    ├── processes.c     processos                (core/processes.py)
    ├── hardware.c      hardware via PowerShell  (core/hardware.py)
    ├── diagnostics.c   diagnósticos             (core/diagnostics.py)
    ├── report.c        relatórios TXT/JSON      (core/report.py)
    ├── banner.c        banner                   (utils/banner.py)
    ├── formatter.c     format_bytes, barras...  (utils/formatter.py)
    ├── json.c          escritor JSON mínimo     (json.dumps)
    └── util.c          helpers de string
```

## Equivalência com o `psutil`

| Python / psutil                | WinAPI usada                                              |
|--------------------------------|-----------------------------------------------------------|
| `cpu_percent`, `cpu_count`     | `GetSystemTimes`, `GetLogicalProcessorInformationEx`      |
| `cpu_freq`                     | `CallNtPowerInformation` (fallback: registro)             |
| `virtual_memory`, `swap_memory`| `GlobalMemoryStatusEx`                                    |
| `disk_partitions/disk_usage`   | `GetLogicalDrives`, `GetDiskFreeSpaceEx`, `GetVolumeInformation` |
| `net_if_addrs/net_if_stats`    | `GetAdaptersAddresses`                                    |
| `net_io_counters`              | `GetIfTable2`                                             |
| `process_iter`                 | `CreateToolhelp32Snapshot`, `GetProcessMemoryInfo`        |
| `boot_time`                    | `GetTickCount64`                                          |

## Licença

MIT.
