# DATASETS

Esta pasta contém os resultados que foram gerados em arquivo CSV ao final das simulações.
| Nome  | Descrição |
| ------------- |:-------------:|
| *baseline.csv*      | Resultados da simulação do cenário de referência.      |
| *stress1.csv*      | Resultados da simulação com a aplicação do estresse 1 do fator escalabilidade.     |
| *stress2.csv*      | Resultados da simulação com a aplicação do estresse 2 do fator cobertura.    |
| *stress3.csv*      | Resultados da simulação com a aplicação do estresse 3 do fator interferência.     |
| *stress4.csv*      | Resultados da simulação com a aplicação do estresse 4 do fator de ruptura de infraestrutura     |

## Tabela de singificados das colunas dos datasets

Nem todas as colunas foram usadas para coleta de dados para análise do comportamento da arquitetura, algumas colunas contém informações para confirmar a lógica execução da simulação.

| Variável  | Descrição |
| ------------- |:-------------:|
| `env`      | Identifica se é o ambiente *indoor* (interno) ou *outdoor* (externo)   |
| `scenario`      | Identifica em qual dos cenários foi realizado   |
| `nED`      | Número de *End Device* / nós sensores    |
| `oobPeriodNormal`      | Para confirmar que no canal Lorawan os heartbeats estavam sendo enviados em um espaço de tempo maior fora da janela de falha.   |
| `oobPeriodFail`      | Para confirmar que no canal Lorawan os pacotes de diagnósticos/dados estavam sendo enviados em um espaço de tempo menor dentro da janela de falhas.   |
| `failStart`      | Para confirmar o início da execução do período de falha, o cenário C0 deve ter sempre o valor 0.   |
| `failEnd`      | Para confirmar o fim da execução do período de falha, o cenário C0 deve ter sempre o valor 0.    |
| `seed`      | Grupo do gerador de números aleatórios   |
| `run`      | Ordem de execução da simulação de acordo com o seed.   |
| `oobTx`      | Total de pacotes enviados pelo canal out-of-band durante todo o período da simulação.    |
| `oobRX`      | Total de pacotes recebidos do canal out-of-band durante todo o período da simulação.   |
| `oobPDR`      | Taxa de Entrega de Pacotes durante toda a simulação.   |
| `latP50`      | Percentil 50 da latência dos pacotes durante toda a simulação.   |
| `latWinP50`      | Percentil 50 da latência dos pacotes apenas na janela de falha.   |
| `oobTxWin`      | Total de pacotes enviados pelo canal out-of-band durante a janela de falha.|
| `oobRxWin`      | Total de pacotes recebidos do canal out-of-band durante a janela de falha. |
| `winPDR`      |Taxa de entrega dos pacotes durante a janela de falha.    |
| `inTxTotal`      | Para confirmar se os pacotes mantiveram a constância de envio.    |
| `InAckRxTotal`  | Valor total de ACK retornados pelo canal *in-band*, utilizado para confirmar a execução da função.    |
| `InAckPDR`      |Taxa de entrega de ACK pelo canal *in-band*. |
| `InTxWin`      |Tentativas de envio de ACK durante a janela de falha.   |
| `InAckRxWin`      |Número de Ack recebidos durante o canal de falha. Para confirmar a execução da função.   |
| `InACKWinPDR`      | Taxa de entrega de de ACKA durante o período de falha, também para a confirmação da execução da simulação.   |
| `detPdP50`      | Percentil 50 do período de até a ativação do LoraWan. Usado também para confirmação da função.   |