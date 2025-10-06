# CatNet Scanner (C)

Aplicação em C com interface TUI (console) para Windows, similar a ferramentas de diagnóstico de rede. Funcionalidades:

- Escaneamento de IPs locais (sub-rede) e de faixa
- Identificação de dispositivos (ping, DNS reverso, MAC via ARP)
- Ping de IPs
- Resolução de nome DNS
- Verificação de portas abertas (lista padrão)
- Exibição de resultados e logs com navegação via teclado
- Exportação dos resultados para arquivo texto

## Build

Pré-requisitos: Windows com um compilador C (MSVC `cl` do Visual Studio Build Tools ou MinGW `gcc`).

### MSVC (cl)

Execute no PowerShell:

```
powershell -ExecutionPolicy Bypass -File build.ps1 -Compiler MSVC
```

### MinGW (gcc)

```
powershell -ExecutionPolicy Bypass -File build.ps1 -Compiler MinGW
```

O executável será gerado em `bin\catnet_tui_scanner.exe`.

### GacUI (GUI)

Para usar a interface gráfica com GacUI:

1. Obtenha e compile o GacUI (incluindo as bibliotecas `GacUI.lib` e `Vlpp.lib`).
2. Defina variáveis de ambiente com os caminhos:
   - `GACUI_INCLUDE`: pasta dos headers do GacUI.
   - `GACUI_LIBS`: pasta das libs (`GacUI.lib`, `Vlpp.lib`).
3. Compile com MSVC usando a opção `-UseGacUI`:

```
powershell -ExecutionPolicy Bypass -File build.ps1 -Compiler MSVC -UseGacUI -GacUIInclude "C:\\Dev\\GacUI\\Import" -GacUILibs "C:\\Dev\\GacUI\\Lib\\Release"
```

O executável será gerado em `bin\catnet_gui_scanner.exe`.

Observação: se encontrar erros para compilar ou linkar o GacUI na sua máquina, você pode utilizar o `GacGen.exe` para gerar código a partir de recursos (XAML) do GacUI. Ainda será necessário linkar com `GacUI.lib`/`Vlpp.lib`. Caso queira, posso integrar um fluxo de geração via `GacGen` no build.

## Uso

Execute o programa no console do Windows:

```
bin\net_tui_scanner.exe
```

Atalhos:
- F1: Escanear sub-rede local
- F2: Escanear faixa (informar início e fim)
- F3: Ping de IP
- F4: Resolução DNS reversa de IP
- F5: Exportar resultados para texto
- Setas: Navegar na lista
- Q / Esc: Sair

Resultados incluem IP, hostname (se disponível), MAC (se disponível), e portas abertas (
22, 80, 443, 139, 445, 3389 por padrão).

## Observações

- MAC via ARP (`SendARP`) funciona apenas em hosts da mesma sub-rede.
- O escaneamento de portas usa conexões TCP não bloqueantes com timeout.
- O ping usa ICMP (`IcmpSendEcho`).