# Tarefas do driver de kernel do SmartLamp

Cada tarefa é um arquivo `.c` independente dentro de `smartlamp-kernel-module/`.
O `Makefile` compila **um arquivo por vez** (linha `obj-m += <arquivo>.o`) — não
são módulos que se combinam entre si; cada um é uma etapa incremental do mesmo
driver, evoluindo até a versão final da Tarefa 3.1.

## Tarefa 2.1 — `probe.c`

**Responsabilidade:** reconhecer o SmartLamp quando ele é conectado na USB.

- Declara `VENDOR_ID`/`PRODUCT_ID` do chip serial CP2102 (Silicon Labs) usado
  pelo ESP32, para o kernel saber qual dispositivo esse driver deve reivindicar.
- Implementa `usb_probe`: roda quando o dispositivo é plugado — descobre os
  endpoints de entrada/saída (`usb_find_common_endpoints`), aloca os buffers
  USB (`usb_in_buffer`/`usb_out_buffer`) e chama `smartlamp_config_serial`
  para habilitar a UART do CP2102 e configurar o baud rate.
- Implementa `usb_disconnect`: libera os buffers quando o dispositivo é
  desconectado.
- Ainda não sabe enviar nem ler comandos — só detecta e prepara a conexão.

## Tarefa 2.2 — `serial_write.c`

**Responsabilidade:** enviar comandos para o ESP32 pela USB.

- Implementa `usb_write_serial(cmd, param)`: formata a string no padrão
  `"COMANDO PARAMETRO\n"` e envia pelo endpoint de saída com `usb_bulk_msg`.
- Exemplo: `usb_write_serial("SET_LED", 80)` deve mandar `"SET_LED 80\n"`.
- Não trata leitura de resposta — só escreve.

## Tarefa 2.3 — `serial_read.c`

**Responsabilidade:** ler e interpretar as respostas que o ESP32 manda de volta.

- Implementa `usb_read_serial()`: os dados chegam fragmentados (byte a byte ou
  em blocos), então a função acumula os caracteres em `recv_line` até achar
  um `'\n'`, extrai o valor numérico da resposta (ex: `"RES GET_LDR 450"` →
  `450`) e retorna esse valor.
- Não sabe qual comando está esperando — só lê a próxima linha completa que
  chegar.

## Tarefa 2.4 — `sysfs.c`

**Responsabilidade:** expor o SmartLamp como arquivos comuns em
`/sys/kernel/smartlamp/{led, ldr, threshold}`, escondendo o protocolo
USB/serial atrás de `cat`/`echo`. É a junção das tarefas 2.1 a 2.3 com uma
interface de usuário em cima.

- Reaproveita a detecção/config da Tarefa 2.1 (`usb_probe`,
  `smartlamp_config_serial`).
- Implementa `usb_write_serial` (equivalente à 2.2) e `usb_read_serial(cmd)`
  (equivalente à 2.3, mas agora recebendo o comando esperado como parâmetro,
  pois vários comandos diferentes — `GET_LED`, `GET_LDR`,
  `GET_THRESHOLD`, `SET_LED`, `SET_THRESHOLD` — passam a existir).
- Cria os arquivos sysfs com `kobject_create_and_add` + `sysfs_create_group`
  dentro do `usb_probe`, e os remove com `sysfs_remove_group`/`kobject_put`
  no `usb_disconnect`.
- `attr_show`: roda quando alguém faz `cat` num dos arquivos — identifica
  qual arquivo foi lido (`attr->attr.name`), manda o `GET_*` correspondente e
  devolve o valor recebido.
- `attr_store`: roda quando alguém faz `echo valor > arquivo` — valida o
  número (`kstrtol`), recusa escrita em `ldr` (somente leitura) e manda
  `SET_LED`/`SET_THRESHOLD` para `led`/`threshold`.
- Particularidade resolvida aqui: como o ESP32 também manda um
  `RES GET_LDR <valor>` sozinho a cada 2s (broadcast periódico), o
  `usb_read_serial` precisa **ignorar linhas que não são a resposta esperada**
  sem desistir cedo demais — por isso ele tem dois contadores separados:
  `retries` (erro real de USB) e `attempts` (teto geral de leituras, cobre
  linhas de broadcast/lixo no meio do caminho).

## Tarefa 3.1 — `test_driver.c` / `smartlamp.c`

**Responsabilidade:** versão final consolidada do driver, juntando tudo (2.1 a
2.4) num único arquivo de entrega, com a função de envio/leitura unificada em
`usb_send_cmd(cmd, param)` (que manda o comando e já espera a resposta certa,
em vez de `write` e `read` separados). No repositório atual (`smartlamp.c`)
ainda está com placeholders não implementados (`BUFFER`, `?` no lugar dos
parâmetros de `usb_bulk_msg`, variável `ret` não declarada) — é a última etapa,
ainda não iniciada pela equipe.

## Como as tarefas se conectam

```
probe.c (2.1)         → detecta o dispositivo, configura a serial
serial_write.c (2.2)  → + envia comandos
serial_read.c (2.3)   → + lê respostas
sysfs.c (2.4)          → + expõe tudo via /sys/kernel/smartlamp 
smartlamp.c (3.1)     → versão final, tudo junto, comando+resposta unificados
```

Cada arquivo é autocontido (não há `#include` entre eles) — o `sysfs.c` não
precisa dos arquivos das tarefas 2.2/2.3 para compilar ou funcionar, porque já
tem sua própria implementação de escrita/leitura serial internamente.
