# Manual de Funções e Módulos

Este manual descreve as funções atualmente implementadas na aplicação de diagnóstico de rede com TUI em C para Windows, incluindo suas responsabilidades, parâmetros e valores de retorno.

## Tipos e Estruturas (src/app.h)
- `DeviceInfo`
  - Campos: `ip`, `hostname`, `mac`, `is_alive`, `open_ports[32]`, `open_ports_count`.
  - Representa um host na rede e seus atributos.
- `DeviceList`
  - Campos: `items`, `count`, `capacity`.
  - Lista dinâmica de `DeviceInfo`.
- `TuiState`
  - Campos: `hConsole`, `width`, `height`, `selected_index`, `scroll_offset`.
  - Estado da interface TUI.
- Funções de lista (implementadas em `src/main.c`):
  - `void device_list_init(DeviceList* list)`
  - `void device_list_clear(DeviceList* list)`
  - `void device_list_push(DeviceList* list, const DeviceInfo* info)`

## Utilitários (src/utils.h, src/utils.c)
- `int ip_to_uint(const char* ip, unsigned long* out)`
  - Converte IPv4 no formato texto para inteiro host-order.
  - Retorna 1 em sucesso, 0 em falha.
- `void uint_to_ip(unsigned long ip, char* buf, size_t buflen)`
  - Converte inteiro host-order para string IPv4.
- `void trim_newline(char* s)`
  - Remove `\n`/`\r` do fim da string.
- `void safe_strcpy(char* dst, size_t dstsz, const char* src)`
  - Cópia segura de string com terminação garantida.

## Rede (src/net.h, src/net.c)
- `int net_init(void)` / `void net_cleanup(void)`
  - Inicializa/encerra o Winsock2.
- `int net_ping_ipv4(const char* ip)`
  - Envia ICMP Echo (`IcmpSendEcho`) ao IP.
  - Retorna 1 se resposta recebida, 0 caso contrário.
- `int net_reverse_dns(const char* ip, char* hostname, size_t hostsz)`
  - DNS reverso usando `GetNameInfoA`.
  - Retorna 1 em sucesso; hostname preenchido.
- `int net_get_mac(const char* ip, char* macbuf, size_t macsz)`
  - Obtém MAC via ARP (`SendARP`) para hosts da mesma sub-rede.
  - Retorna 1 se obtido; caso contrário 0.
- `int net_scan_ports(const char* ip, const int* ports, int ports_count, int timeout_ms, int* open_ports, int* open_count)`
  - Verifica portas TCP usando conexão não bloqueante e `select`.
  - Retorna número de portas abertas encontradas; preenche `open_ports/open_count`.
- `int net_get_primary_subnet(SubnetV4* out)`
  - Determina a sub-rede IPv4 principal via `GetAdaptersAddresses`.
  - Preenche `network`, `mask`, `start_ip`, `end_ip`. Retorna 1 em sucesso.

## Escaneamento (src/scan.h, src/scan.c)
- `typedef struct ScanConfig`
  - Campos: `default_ports[16]`, `default_ports_count`, `port_timeout_ms`.
- `void scan_config_init(ScanConfig* cfg)`
  - Define portas padrão (22,80,443,139,445,3389) e timeout de porta (500 ms).
- `void identify_device(DeviceInfo* info, const ScanConfig* cfg)`
  - Para um IP: executa ping, DNS reverso, MAC e escaneia portas; preenche `DeviceInfo`.
- `void scan_subnet(DeviceList* out, const ScanConfig* cfg)`
  - Itera todos os hosts da sub-rede principal e identifica cada dispositivo.
- `void scan_range(DeviceList* out, const ScanConfig* cfg, const char* start_ip, const char* end_ip)`
  - Itera de `start_ip` a `end_ip` (inclusive), identificando dispositivos.

## TUI (src/tui.h, src/tui.c)
- `void tui_init(TuiState* st)`
  - Inicializa console, obtém dimensões e oculta cursor.
- `void tui_draw_frame(TuiState* st)`
  - Desenha moldura básica (bordas, separadores, áreas de resultados e log).
- `void tui_draw_header(TuiState* st, const char* title)`
  - Mostra atalhos (F1..F5, Q/Esc) e título.
- `void tui_draw_results(TuiState* st, const DeviceList* list)`
  - Exibe lista de dispositivos com IP, hostname, MAC, status e portas.
  - Suporta seleção e rolagem pelo `TuiState`.
- `void tui_draw_logs(TuiState* st, const char* logline)`
  - Escreve mensagem na área de log inferior.
- `int tui_read_key(INPUT_RECORD* rec)`
  - Lê um evento de teclado do console.
- `void tui_clear_area(int x, int y, int w, int h)`
  - Limpa área retangular com espaços.
- `void tui_goto(int x, int y)`
  - Move o cursor para coordenadas.

## Exportação (src/export.h, src/export.c)
- `int export_results_to_file(const char* path, const DeviceList* list)`
  - Gera arquivo texto CSV simples com cabeçalho `IP;Hostname;MAC;Status;Ports`.
  - Retorna 1 em sucesso.

## Ponto de Entrada (src/main.c)
- Inicializa Winsock (`net_init`) e TUI (`tui_init`).
- Mantém loop de eventos de teclado com as ações:
  - `F1`: Escaneia sub-rede local (ping/DNS/MAC/portas) e exibe resultados.
  - `F2`: Solicita IP inicial/final e escaneia faixa.
  - `F3`: Ping em IP informado; mostra resultado.
  - `F4`: DNS reverso de IP informado; mostra resultado.
  - `F5`: Exporta `results.txt` no diretório atual.
  - Setas: navegação na lista; `Q`/`Esc`: sair.
- Libera recursos (`device_list_clear`, `net_cleanup`).

## Considerações e Limitações
- MAC é obtido apenas para hosts na mesma sub-rede (ARP).
- Portas verificadas são TCP e a lista é configurável via `ScanConfig`.
- Escaneamentos são sequenciais; para redes grandes, considerar paralelismo (threads).
- Tempo de ping é fixo (1s); portas usam timeout configurável.

## Extensões Futuras Sugeridas
- Escaneamento concorrente com thread pool.
- Detecção de serviços (HTTP banners, SMB, RDP handshake básico).
- Exportação adicional (JSON/CSV com cabeçalhos customizados).
- Suporte a IPv6 (ping, DNS, escaneamento de portas).
- Filtros na TUI (mostrar apenas UP, com portas abertas etc.).