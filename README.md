# Respositório TCC - ECAI
### Especialização em Computação Aplicada à Indústria 4.0 com ênfase em Internet das Coisas / UFRR 
## Arquitetura de Diagnóstico Out-of-Band Baseada em LoRaWAN para Sistemas Ciberfísicos Industriais

> **Discente:** Jussara M. Soares  
> **Orietadora** Profa. Dra. Josiane Rodrigues

Este repositório contém a modelagem, os scripts de automação e as modificações necessárias no simulador **ns-3** para validar o uso do protocolo **LoRaWAN** como um canal de comunicação de diagnóstico "Fora de Banda" (Out-of-Band) resiliente para sistemas ciberfísicos.

---

## Contexto
Os sistemas ciberfísicos industriais são grandes habilitadores da Indútria 4.0. Um dos maiores desafios têm sido manter a disponibilidade desses sistemas, principalmente para os que utilizam a comunicação *in-band* para a supervisão destes. 

Quando ocorre falhas na rede *in-band*, o sistema fica impedindo de receber diagnósticos de erro. Esse ponto de falha pode gerar efeitos em cascata na perda de dados importantes da borda do sistemas ciberfísicos industriais, principalmente em infraestruturas críticas.

A solução validada através do simulador de redes NS-3 neste projeto acopla módulos de rádio LoRa aos nós sensores. Ao detectarem a queda da rede principal de forma autônoma, os dispositivos ativam o rádio e utilizam o meio sem fio do LoRa, operando sob o protocolo LoRaWAN, para enviar alertas vitais ao Network Server.

---

## Tecnologias Utilizadas
* **Simulador:** [ns-3](https://www.nsnam.org/) + [Módulo LoRaWAN - Magrin *et al.*](https://github.com/signetlabdei/lorawan)
* **Linguagem:** C++ (Modelagem de rede e protocolos).
* **Automação:** Bash Script (Bateria de testes e coleta de dados).

---

## Tutorial de Reprodução

### Pré-requisitos e Ambiente de Instalação

#### Sistema Operacional Recomendado
O ambiente testado para a execução deste projeto foi o **Linux Mint 22.3 (Xfce 64-bit)**.

> **Nota sobre o uso no Windows (WSL):** > Embora seja possível executar o simulador via WSL (Windows Subsystem for Linux), essa abordagem **não é recomendada** para este projeto. Durante os testes, o ambiente WSL apresentou instabilidades constantes relacionadas à ausência de bibliotecas de compilação e dependências do sistema, exigindo configuração manual avançada.

#### Instalação do ns-3 e Módulo LoRaWAN

1. Sugiro seguir o  [tutorial de instalação do ns-3 junto com o módulo LoRaWAN](https://github.com/signetlabdei/lorawan) para clonar o repositório e compilar os arquivos necessários já voltados para o Lorawan.
2. Após garantir que o simulador está rodando, aplique as modificações descritas no tutorial deste repositório.

Devido às atualizações constantes do ns-3, a clonagem direta deste repositório não é recomendada. Em vez disso, siga as instruções abaixo para adicionar as modificações no seu ambiente ns-3 local.

### Passo 1: Adição das Modificações no Módulo LoRaWAN
Para que o Network Server consiga diferenciar o que é um pacote de alerta de emergência do que é apenas ruído/tráfego de fundo, foi necessário criar uma nova Tag (`OobMetricTag`) e modificar o `PeriodicSender` do módulo LoRaWAN para anexá-la aos pacotes.

Copie os arquivos da pasta `modulos_ns3/` deste repositório para dentro do diretório de código-fonte do módulo LoRaWAN no seu ns-3:

1. **Adicionar o novo arquivo de Tag:**
   * Copie deste repositório `modulos_ns3/oob-metric-tag.h` para o sua pasta do ns-3 `~/ns-3-dev/src/lorawan/model` 
   * Copie deste repositório `modulos_ns3/oob-metric-tag.cc` para o sua pasta do ns-3 `~/ns-3-dev/src/lorawan/model`
2. **Substituir os arquivos da Aplicação:**
   * Copie  `modulos_ns3/periodic-sender.cc` e substitua em `ns-3.xx/src/lorawan/model/`
   * Copie  `modulos_ns3/CMakeLists.txt` e substitua em `~/ns-3-dev/src/lorawan`


### Passo 2: Adicionar os Scripts da Simulação
1. Copie o arquivo `simulador/sim_oob.cc` e cole-o dentro da pasta `scratch/` do seu ns-3.
2. Copie o arquivo `scripts/run_full.sh` e cole-o dentro da pasta raiz do seu ns-3.
### Passo 3: Recompilar o ns-3
Como arquivos do núcleo (`src/lorawan`) foram alterados, você deve recompilar o simulador:
No seu termional, entre na pasta raiz do seu ns-3:
```bash
cd ns-3-dev/
./ns3 build
```
### Passo 4: Execução da Simulação

Ainda na pasta raiz, utilize os comandos abaixo para execução da simulação.

Para executar o cenário referência (*baseline*)
```bash
./run-full.sh baseline
```
Para executar o estresse 1 (*Fator: Escalabilidade*)
```bash
./run-full.sh stress1
```
Para executar o estresse 2 (*Fator: Cobertura*)
```bash
./run-full.sh stress2
```
Para executar o estresse 3 (*Fator: Interferencias*)
```bash
./run-full.sh stress3
```
Para executar o estresse 4 (*Fator: Ruptura na Infraestrutura*)
```bash
./run-full.sh stress4
```

### Passo 5: Coleta e Análise de Dados

Após a execução de qualquer um dos cenários acima, o simulador irá gerar arquivos .csv automaticamente na pasta `results` diretório do ns-3. Esses arquivos contêm as métricas fundamentais extraídas da simulação, incluindo:

PDR (Packet Delivery Ratio): Taxa de entrega de pacotes na rede principal e na rede de diagnóstico.

Latência: Tempo e atraso na entrega dos alertas via LoRaWAN.

Contagem de Pacotes: Volume de dados trafegados dentro e fora da janela de falha (apagão).

Leia o [read me](datasets/read_me_datasets.md) da pasta de Datasets que contêm as descrições a respeito dos parâmetros informados nos arquivos csv. 


---

## Principais Contribuições
Este projeto demonstra que a utilização do LoRaWAN como rede Out-of-Band (OOB) é uma alternativa viável e confiável para ambientes industriais. Os resultados obtidos nos cenários de estresse comprovam que:

**Alta Disponibilidade**: A rede OOB assume a comunicação com sucesso quase imediato durante a queda do canal in-band. 

**Resiliência a Interferências**: O uso de configurações específicas (como adaptação de Spreading Factor) garante a entrega de mensagens críticas mesmo em espectros ruidosos. Observa-se a entrega de ~98% das taxas de entrega de pacotes durante o período de falha.

**Escalabilidade**: A arquitetura suporta o aumento na densidade de nós sensores sem comprometer o tempo de resposta da rede de emergência.

---
## Agradecimentos
O projeto a partir do qual este estudo foi derivado foi apoiado pela SUFRAMA, com recursos da Lei nº 8.387, de 1991, no âmbito do PPI-Indústria 4.0, oordenado pelo CITS-Amazônia, aprovado pelo CAPDA/SUFRAMA e executado pelo Departamento de Ciência da COmputação - DCC/UFRR.

Este repositório é fruto do Trabalho de Conclusão de Curso (TCC) da Especialização em Computação Aplicada à Indústria 4.0 pela Universidade Federal de Roraima (UFRR).

Agradecimentos especiais à minha orientadora, Profa. Dra. Josiane Rodrigues, por todo o suporte, direcionamento e ao longo do desenvolvimento desta pesquisa.

---
## ✉️ Contato e Licença

Este projeto é de código aberto e distribuído sob a licença **GNU General Public License v3.0 (GPLv3)**. Sinta-se à vontade para clonar, estudar, modificar e utilizar os scripts deste repositório para fins acadêmicos e de pesquisa contínua na área de Redes Industriais e Internet das Coisas, desde que mantenha a natureza aberta do projeto. Consulte o arquivo `LICENSE` para mais detalhes.

Caso tenha dúvidas sobre a modelagem, a instrumentação do código C++ ou a configuração do ns-3, você pode entrar em contato:

* **Autora:** Jussara M. Soares
* [LinkedIn] (https://www.linkedin.com/in/jussara-miliano-soares)
* **E-mail:** jmilianosoares@gmail.com
