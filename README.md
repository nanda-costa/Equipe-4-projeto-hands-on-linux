

# DevTITANS 10 - HandsOn Linux - Equipe 04

Bem-vindo ao repositório da Equipe 04 do HandsON de Linux do DevTITANS! Este projeto contém um firmware para o ESP32 escrito em formato Arduino `.ino`, bem como um driver do kernel Linux escrito em C. O objetivo é demonstrar como criar uma solução completa de hardware e software que integra um dispositivo ESP32 com um sistema Linux — o SmartLamp: um LED com brilho controlável por PWM e um sensor de luminosidade (LDR), controlados via `/sys/kernel/smartlamp/{led,ldr,threshold}`.

Documentação mais detalhada (protocolo serial, pinagem, guia de build/testes de cada etapa do driver) está no [wiki do projeto](https://github.com/nanda-costa/Equipe-4-projeto-hands-on-linux/wiki). Para apresentar o projeto ao professor, veja o [Guia de Demonstração](GUIA-DEMONSTRACAO.md).

## Tabela de Conteúdos

- [Contribuidores](#contribuidores)
- [Introdução](#introdução)
- [Recursos](#recursos)
- [Requisitos](#requisitos)
- [Configuração de Hardware](#configuração-de-hardware)
- [Instalação](#instalação)
- [Uso](#uso)
- [Contato](#contato)

## Contribuidores

- **João Vitor:** driver USB (`probe.c`) e escrita na porta serial (`serial_write.c`)
- **Luiz Barbosa:** leitura da porta serial (`serial_read.c`)
- **Antonio Fernandes:** firmware (`smartlamp.ino`) — leitura da serial
- **Fernanda Costa:** driver sysfs (`sysfs.c`), driver final consolidado (`smartlamp.c`) e documentação (wiki, README, guia de demonstração)

## Introdução

Este projeto serve como um exemplo para desenvolvedores interessados em construir e integrar soluções de hardware personalizadas com sistemas Linux. Inclui os seguintes componentes:
- Firmware para o microcontrolador ESP32 para lidar com operações específicas do dispositivo.
- Um driver do kernel Linux que se comunica com o dispositivo ESP32, permitindo troca de dados e controle.

## Recursos

- **Firmware ESP32:**
  - Leitura do sensor de luminosidade (LDR) e controle do LED via PWM.
  - Comunicação via Serial com o driver Linux (protocolo de comandos/respostas, mais detalhes no [wiki](https://github.com/nanda-costa/Equipe-4-projeto-hands-on-linux/wiki/Protocolo-Serial)).

- **Driver do Kernel Linux (`smartlamp-kernel-module/smartlamp.c`):**
  - Rotinas de inicialização e limpeza (`usb_probe`/`usb_disconnect`).
  - Expõe `GET_LED`/`SET_LED`, `GET_LDR` e `GET_THRESHOLD`/`SET_THRESHOLD` como arquivos sysfs (`led`, `ldr`, `threshold`).
  - Comunicação com o ESP32 via USB Serial (chip CP2102).

## Requisitos

- **Hardware:**
  - Placa de Desenvolvimento ESP32
  - Máquina Linux
  - Protoboard e Cabos Jumper
  - Sensor LDR
  
- **Software:**
  - Arduino IDE
  - Kernel Linux 4.0 ou superior
  - GCC 4.8 ou superior
  - Make 3.81 ou superior

## Configuração de Hardware

1. **Conecte o ESP32 à sua Máquina Linux:**
    - Use um cabo USB.
    - Conecte os sensores ao ESP32 conforme especificado no firmware.

2. **Garanta a alimentação e conexões adequadas:**
    - Use um protoboard e cabos jumper para montar o circuito.
    - Consulte a pinagem em [`esp32/pinos.txt`](esp32/pinos.txt) (ou o [guia de hardware/pinagem](https://github.com/nanda-costa/Equipe-4-projeto-hands-on-linux/wiki/Hardware-e-Pinagem) no wiki) para conexões detalhadas.

## Instalação

### Firmware ESP32

1. **Abra o Arduino IDE e carregue o firmware:**
    ```sh
    Arquivo -> Abrir -> Selecione `smartlamp.ino`
    ```

2. **Configure a Placa e a Porta:**
    ```sh
    Ferramentas -> Placa -> Node32s
    Ferramentas -> Porta -> Selecione a porta apropriada
    ```

3. **Carregue o Firmware:**
    ```sh
    Sketch -> Upload (Ctrl+U)
    ```

### Driver Linux

1. **Clone o Repositório:**
    ```sh
    git clone https://github.com/nanda-costa/Equipe-4-projeto-hands-on-linux.git
    cd Equipe-4-projeto-hands-on-linux
    ```

2. **Compile o Driver:**
    ```sh
    cd smartlamp-kernel-module
    make
    ```
    Confira antes que a linha `obj-m` do `Makefile` está como `obj-m += smartlamp.o` — é o driver final (os demais arquivos `.c` da pasta são etapas intermediárias de desenvolvimento, veja o [wiki](https://github.com/nanda-costa/Equipe-4-projeto-hands-on-linux/wiki/Build-e-Testes-do-Driver)).

3. **Libere a interface USB e carregue o Driver:**
    ```sh
    sudo rmmod cp210x 2>/dev/null   # o driver nativo cp210x reivindica a interface antes do nosso
    sudo insmod smartlamp.ko
    ```

4. **Verifique o Driver:**
    ```sh
    sudo dmesg | tail
    ```

## Uso

Depois que o driver e o firmware estiverem configurados, você poderá interagir com o dispositivo ESP32 através do sistema Linux, usando `/sys/kernel/smartlamp/{led, ldr, threshold}`.

- **Escrever para o Dispositivo:**
    ```sh
    echo 100 | sudo tee /sys/kernel/smartlamp/led
    echo 80  | sudo tee /sys/kernel/smartlamp/threshold
    ```

- **Ler do Dispositivo:**
    ```sh
    cat /sys/kernel/smartlamp/led
    cat /sys/kernel/smartlamp/ldr
    cat /sys/kernel/smartlamp/threshold
    ```

- **Verificar Mensagens do Driver:**
    ```sh
    sudo dmesg | tail
    ```

- **Remover o Driver:**
    ```sh
    sudo rmmod smartlamp
    sudo modprobe cp210x   # devolve o dispositivo pro driver padrão do Linux
    ```
    
## Contato

Para perguntas, sugestões ou feedback, entre em contato com o mantenedor do projeto em [maintainer@example.com](mailto:maintainer@example.com).
