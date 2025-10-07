# CatNet Scanner (GUI, WIP)

CatNet Scanner is a Windows network scanner with a graphical UI built using Raygui (raylib).

## Features

- Scan local subnet and custom IP ranges.
- Identify devices: ping, reverse DNS, MAC via ARP.
- Check common TCP open ports (configurable list).
- Export results to a text file.

## Build (Windows, MSVC)

### Compile

```
powershell -ExecutionPolicy Bypass -File build.ps1 -Compiler MSVC
```

Produces `bin\catnet_scanner.exe`.

### Run

```
bin\catnet_scanner.exe
```

## Notes

- The GUI is created programmatically in `src\main_raygui.c`.
- MAC via ARP works only on the same subnet; ping uses ICMP; ports are checked via non-blocking TCP.
- WIP: UI and flow may change.

## Third-Party and Acknowledgements

- Uses `raylib` and `raygui` as Git submodules:
  - raylib: https://github.com/raysan5/raylib
  - raygui: https://github.com/raysan5/raygui
- Huge thanks to the authors and contributors of raylib and raygui.

## Submódulos (como clonar e atualizar)

Para clonar o repositório já com os submódulos:

```
git clone --recursive https://github.com/mendsec/catnet_scanner.git
```

Se você já clonou sem `--recursive`, inicialize e atualize os submódulos:

```
git submodule update --init --recursive
```

Para manter os submódulos atualizados (avançar para as últimas mudanças nas upstreams) e registrar no repositório:

```
# Atualiza cada submódulo para o branch padrão
git submodule foreach "git checkout master || git checkout main"
git submodule foreach "git pull"

# Registra os novos commits no repositório pai
git add third_party/raylib third_party/raygui
git commit -m "Update submodules"
git push
```