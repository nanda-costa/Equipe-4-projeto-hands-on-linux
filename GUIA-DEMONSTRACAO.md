# Guia de Demonstração — SmartLamp

Roteiro para apresentação do projeto ao professor: o que preparar antes,
a sequência de comandos a rodar, o que apontar em cada etapa, e como reagir
se algo não sair como esperado na hora. Documentação técnica mais profunda
está no [wiki do projeto](https://github.com/nanda-costa/Equipe-4-projeto-hands-on-linux/wiki) — este guia é só o roteiro da demo.

## 1. Antes da demonstração (preparar com antecedência, não na hora)

- [ ] ESP32 com o firmware `smartlamp.ino` já gravado e funcionando.
- [ ] LED e LDR montados na protoboard conforme `esp32/pinos.txt`.
- [ ] `smartlamp-kernel-module/Makefile` com `obj-m += smartlamp.o` (é o
      driver final — os outros arquivos `.c` da pasta são só etapas
      intermediárias de desenvolvimento).
- [ ] Módulo compilado (`make`) e testado pelo menos uma vez antes da
      apresentação, pra não descobrir um problema na frente do professor.
- [ ] Saber de cor (ou ter num post-it) a senha do `sudo` da máquina que vai
      usar — todo comando de carregar/remover módulo e ler `dmesg` precisa.
- [ ] Terminal com fonte grande o suficiente pra quem estiver assistindo ler.

## 2. Roteiro da demonstração

### Passo 0 — Contexto rápido

Explique a arquitetura em uma frase: "o ESP32 controla um LED e lê um LDR,
fala com o Linux por USB serial, e um driver de kernel que a gente escreveu
expõe isso como arquivos comuns em `/sys/kernel/smartlamp/`, então controlar
o hardware vira só um `cat`/`echo`."

### Passo 1 — Liberar a interface USB e carregar o driver

O chip serial do ESP32 (CP2102) tem um driver nativo do Linux (`cp210x`) que
carrega sozinho ao plugar o cabo. Como só um driver pode reivindicar a
mesma interface por vez, ele precisa estar descarregado antes do nosso:

```bash
cd smartlamp-kernel-module
sudo rmmod cp210x 2>/dev/null   # ignora erro se já não estiver carregado
sudo dmesg -C                   # limpa o log pra ficar fácil de ler o que vem a seguir
sudo insmod smartlamp.ko
sudo dmesg | tail -10
```

Aponte no `dmesg`: mensagem de dispositivo conectado, configuração da porta
serial e baud rate — é o driver reconhecendo o ESP32 (VendorID/ProductID do
CP2102) e preparando a comunicação.

### Passo 2 — Mostrar os arquivos que o driver criou

```bash
ls -la /sys/kernel/smartlamp/
```

Aponte: `led`, `ldr` e `threshold` aparecem como arquivos comuns do
filesystem — é o sysfs escondendo todo o protocolo USB/serial atrás de uma
interface simples.

### Passo 3 — Ler os valores atuais

```bash
cat /sys/kernel/smartlamp/led
cat /sys/kernel/smartlamp/ldr
cat /sys/kernel/smartlamp/threshold
```

Explique rapidamente: cada `cat` dispara um comando serial (`GET_LED`,
`GET_LDR`, `GET_THRESHOLD`) pro ESP32 e espera a resposta (`RES ... <valor>`).
Se quiser, mostre isso ao vivo com `sudo dmesg | tail` logo depois de um `cat`.

### Passo 4 — Controlar o LED (efeito visível, o ponto alto da demo)

```bash
echo 100 | sudo tee /sys/kernel/smartlamp/led   # brilho máximo — LED deve acender forte
cat /sys/kernel/smartlamp/led
echo 10 | sudo tee /sys/kernel/smartlamp/led    # brilho baixo — LED deve escurecer visivelmente
cat /sys/kernel/smartlamp/led
```

Esse é o momento de apontar pro LED físico mudando de brilho em tempo real.

### Passo 5 — Mostrar o LDR reagindo à luz

Tampe o sensor com a mão (ou aponte uma lanterna de celular nele) e leia de
novo:

```bash
cat /sys/kernel/smartlamp/ldr
```

Repita tampando/destampando pra mostrar o valor mudando ao vivo — prova que
não é um valor fixo/mockado.

### Passo 6 — Mostrar o `threshold`

```bash
echo 80 | sudo tee /sys/kernel/smartlamp/threshold
cat /sys/kernel/smartlamp/threshold
```

Explique que esse valor é o limite de luminosidade guardado no firmware,
pensado pra uso futuro por um daemon que decida quando acender o LED
automaticamente (hoje só está guardado, ainda não há um daemon reagindo a ele).

### Passo 7 — Mostrar que `ldr` é somente leitura (validação, não é bug)

```bash
echo 50 | sudo tee /sys/kernel/smartlamp/ldr
```

**Espera-se erro** (`Permission denied`) — é o driver recusando escrita de
propósito, porque o LDR é um sensor, não faz sentido "setar" a leitura dele.
Vale mostrar isso como prova de que o driver valida entradas, não só aceita
qualquer coisa.

### Passo 8 — Encerrar

```bash
sudo dmesg | tail -20      # log completo da sessão de demo, se quiser fechar mostrando isso
sudo rmmod smartlamp
sudo modprobe cp210x       # devolve o dispositivo pro driver padrão do Linux
```

## 3. Se o professor pedir pra ver o processo incremental

O driver foi construído em etapas, cada uma um arquivo `.c` independente em
`smartlamp-kernel-module/` (`probe.c` → `serial_write.c` → `serial_read.c`
→ `sysfs.c` → `smartlamp.c`, esse último é a versão final usada na demo
acima). Detalhes de cada etapa e como testar isoladamente estão no wiki, em
[Build e Testes do Driver](https://github.com/nanda-costa/Equipe-4-projeto-hands-on-linux/wiki/Build-e-Testes-do-Driver) e
[Home](https://github.com/nanda-costa/Equipe-4-projeto-hands-on-linux/wiki).

## 4. Perguntas prováveis do professor (e resposta curta)

- **"Por que precisou desabilitar o `cp210x`?"** — Só um driver pode
  reivindicar a mesma interface USB por vez; o `cp210x` é o driver nativo do
  chip serial e carrega automaticamente, então precisa sair do caminho pro
  nosso `smartlamp` conseguir se conectar.
- **"Como o driver sabe que resposta é de qual comando?"** — Cada resposta
  vem no formato `RES <COMANDO> <valor>`; o driver manda o comando e fica
  lendo linhas até achar uma que comece (ou contenha) esse prefixo,
  descartando o que não bate — inclusive o broadcast periódico de LDR que o
  firmware manda sozinho a cada 2s. Detalhes em
  [Protocolo Serial](https://github.com/nanda-costa/Equipe-4-projeto-hands-on-linux/wiki/Protocolo-Serial) no wiki.
- **"O que é esse `/sys/kernel/smartlamp`?"** — É sysfs, um filesystem
  virtual do kernel Linux pra expor/configurar drivers como arquivos comuns,
  sem precisar de uma API especial — só `cat`/`echo`.

## 5. Se algo der errado na hora

- **`insmod` reclama "File exists"**: o módulo já está carregado de um teste
  anterior — não é erro, pode seguir direto pro `cat`.
- **`echo ... | sudo tee ...` dá "Permission denied" em `led` ou
  `threshold`** (não em `ldr`, que é esperado): normalmente é só o driver
  não tendo confirmado a resposta do ESP32 a tempo (ruído elétrico do PWM,
  ver [Protocolo Serial](https://github.com/nanda-costa/Equipe-4-projeto-hands-on-linux/wiki/Protocolo-Serial)) — o comando geralmente
  já foi aplicado mesmo assim. Dê um `cat` no mesmo arquivo logo em seguida
  pra confirmar, e tente o `echo` de novo se quiser mostrar sem esse aviso.
- **`cat` trava ou demora muito**: espere — o driver tem um tempo de espera
  configurado antes de desistir. Se travar de vez, `Ctrl+C`, confira o
  `dmesg`, e no pior caso desconecte/reconecte fisicamente o cabo USB antes
  de tentar de novo.
