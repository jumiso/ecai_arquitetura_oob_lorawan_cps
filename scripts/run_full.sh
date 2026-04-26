#Projeto de Conclusão de Curso - Especialização Lato Sensu em Computação Aplicada a Industria 4.0 UFRR
#Discente: Jussara M Soares
#https://github.com/jumiso/ecai_arquitetura_oob_lorawan_cps
#Arquivo executável
set -euo pipefail

SIM="scratch/sim_oob"
RESULTS_DIR="results"
mkdir -p "$RESULTS_DIR"

SEED=1
RUNS=(1 2 3 4 5)
ENVS=("indoor" "outdoor")  # ambientes interno e externo
SFS=(7 12)                 # spread factor
SCENARIOS=("C0" "C1" "C2") # cenários

# Base
SIMTIME=1800    # tempo da simulação em segundos
FAILSTART=600   # inicio da janela de falha em segundos
FAILEND=1200    # fim da janela de falha em segundos
PAYLOAD=12      # OBS: no C++ o payload está fixo em 12 bytes (aqui não está sendo usado)
PERIOD_BASE=600 # intervalo de tempo entre os heartbeats (não usado; OOB usa os períodos abaixo)
NED_BASE=20     # quantidade de end devices / sensores
JITTER=5        # variação aleatoria de tempo

OOB_PERIOD_NORMAL=900
OOB_PERIOD_FAIL=120
OOB_TRIG_DELAY=20
IN_ECHO_INT=2   # intervalo do ACK no in-band (s)
MISS_TH=10       # N perdas consecutivas para detecção (Td)

run_one () {
  local out="$1"; shift
  echo "./ns3 run $SIM -- $* --out=$out"
  ./ns3 run "$SIM" -- "$@" --out="$out"
}

baseline () {
  local OUT="${RESULTS_DIR}/baseline.csv"
  rm -f "$OUT"

  for env in "${ENVS[@]}"; do
    for sc in "${SCENARIOS[@]}"; do
      for sf in "${SFS[@]}"; do
        for run in "${RUNS[@]}"; do

          # C0: não existe "janela de falha", então zera para não contaminar as métricas Win
          if [[ "$sc" == "C0" ]]; then
            FS=0
            FE=0
          else
            FS="$FAILSTART"
            FE="$FAILEND"
          fi

          run_one "$OUT" \
            --env="$env" --scenario="$sc" \
            --nEd="$NED_BASE" --sf="$sf" \
            --simTime="$SIMTIME" \
            --oobPeriodNormal="$OOB_PERIOD_NORMAL" --oobPeriodFail="$OOB_PERIOD_FAIL" \
            --failStart="$FS" --failEnd="$FE" \
            --seed="$SEED" --run="$run" \
            --nInt=0 --intPeriod=5 \
            --jitter="$JITTER" \
            --oobTrigDelay="$OOB_TRIG_DELAY" \
            --inEchoInt="$IN_ECHO_INT" --missTh="$MISS_TH"
        done
      done
    done
  done

  echo "Baseline done: $OUT"
  echo "Lines:"
  wc -l "$OUT"
  echo "Head:"
  head -n 5 "$OUT"
}

stress1 () {
  local OUT="${RESULTS_DIR}/stress1.csv"
  rm -f "$OUT"

  # Estresse 1: nEd=100 (mantém sem interferência)
  for env in "${ENVS[@]}"; do
    for sc in "${SCENARIOS[@]}"; do
      for sf in "${SFS[@]}"; do
        for run in "${RUNS[@]}"; do

          # C0: zerado
          if [[ "$sc" == "C0" ]]; then
            FS=0
            FE=0
          else
            FS="$FAILSTART"
            FE="$FAILEND"
          fi

          run_one "$OUT" \
            --env="$env" --scenario="$sc" \
            --nEd=100 --sf="$sf" \
            --simTime="$SIMTIME" \
            --oobPeriodNormal="$OOB_PERIOD_NORMAL" --oobPeriodFail="$OOB_PERIOD_FAIL" \
            --failStart="$FS" --failEnd="$FE" \
            --seed="$SEED" --run="$run" \
            --nInt=0 --intPeriod=5 \
            --jitter="$JITTER" \
            --oobTrigDelay="$OOB_TRIG_DELAY" \
            --inEchoInt="$IN_ECHO_INT" --missTh="$MISS_TH"
        done
      done
    done
  done

  echo "Stress1 done: $OUT"
  echo "Lines:"
  wc -l "$OUT"
  echo "Head:"
  head -n 5 "$OUT"
}

stress2 () {
  local OUT="${RESULTS_DIR}/stress2.csv"
  rm -f "$OUT"

  # Estresse 2: só distâncias (usa baseDist e step no C++)
  for env in "${ENVS[@]}"; do
    for sc in "${SCENARIOS[@]}"; do
      for sf in "${SFS[@]}"; do
        for run in "${RUNS[@]}"; do

          if [[ "$env" == "indoor" ]]; then
            BASEDIST=80
            STEP=10
          else
            BASEDIST=1200
            STEP=50
          fi

          # C0: zerado
          if [[ "$sc" == "C0" ]]; then
            FS=0
            FE=0
          else
            FS="$FAILSTART"
            FE="$FAILEND"
          fi

          run_one "$OUT" \
            --env="$env" --scenario="$sc" \
            --nEd="$NED_BASE" --sf="$sf" \
            --simTime="$SIMTIME" \
            --oobPeriodNormal="$OOB_PERIOD_NORMAL" --oobPeriodFail="$OOB_PERIOD_FAIL" \
            --failStart="$FS" --failEnd="$FE" \
            --seed="$SEED" --run="$run" \
            --nInt=0 --intPeriod=5 \
            --jitter="$JITTER" \
            --baseDist="$BASEDIST" --step="$STEP" \
            --oobTrigDelay="$OOB_TRIG_DELAY" \
            --inEchoInt="$IN_ECHO_INT" --missTh="$MISS_TH"
        done
      done
    done
  done

  echo "Stress2 done: $OUT"
  wc -l "$OUT"
}

stress3 () {
  local OUT="${RESULTS_DIR}/stress3.csv"
  rm -f "$OUT"

  # Estresse 3: nInt=20, intPeriod=5
  for env in "${ENVS[@]}"; do
    for sc in "${SCENARIOS[@]}"; do
      for sf in "${SFS[@]}"; do
        for run in "${RUNS[@]}"; do

          # C0: zeraso
          if [[ "$sc" == "C0" ]]; then
            FS=0
            FE=0
          else
            FS="$FAILSTART"
            FE="$FAILEND"
          fi

          run_one "$OUT" \
            --env="$env" --scenario="$sc" \
            --nEd="$NED_BASE" --sf="$sf" \
            --simTime="$SIMTIME" \
            --oobPeriodNormal="$OOB_PERIOD_NORMAL" --oobPeriodFail="$OOB_PERIOD_FAIL" \
            --failStart="$FS" --failEnd="$FE" \
            --seed="$SEED" --run="$run" \
            --nInt=20 --intPeriod=5 \
            --jitter="$JITTER" \
            --oobTrigDelay="$OOB_TRIG_DELAY" \
            --inEchoInt="$IN_ECHO_INT" --missTh="$MISS_TH"
        done
      done
    done
  done

  echo "Stress3 done: $OUT"
  echo "Lines:"
  wc -l "$OUT"
  echo "Head:"
  head -n 5 "$OUT"
}

stress4 () {
  local OUT="${RESULTS_DIR}/stress4.csv"
  rm -f "$OUT"

  # Estresse 4: cenário C2 (o enlace entre gateway e network server também falha)
  for env in "${ENVS[@]}"; do
    for sf in "${SFS[@]}"; do
      for run in "${RUNS[@]}"; do
        run_one "$OUT" \
          --env="$env" --scenario="C2" \
          --nEd="$NED_BASE" --sf="$sf" \
          --simTime="$SIMTIME" \
          --oobPeriodNormal="$OOB_PERIOD_NORMAL" --oobPeriodFail="$OOB_PERIOD_FAIL" \
          --failStart="$FAILSTART" --failEnd="$FAILEND" \
          --seed="$SEED" --run="$run" \
          --nInt=0 --intPeriod=5 \
          --jitter="$JITTER" \
          --oobTrigDelay="$OOB_TRIG_DELAY" \
          --inEchoInt="$IN_ECHO_INT" --missTh="$MISS_TH"
      done
    done
  done

  echo "Stress4 done: $OUT"
  echo "Lines:"
  wc -l "$OUT"
  echo "Head:"
  head -n 5 "$OUT"
}



case "${1:-}" in
  baseline) baseline ;;
  stress1) stress1 ;;
  stress2) stress2 ;;
  stress3) stress3 ;;
  stress4) stress4 ;;
  *)
    echo "Uso: $0 {baseline|stress1|stress2|stress3|stress4}"
    echo "Obs: baseline/stress1/stress2/stress3 rodam C0,C1,C2 via SCENARIOS."
    echo "Obs: stress4 roda apenas cenario C2 (falha de comunicação entre GW->NS na janela)."
   
    exit 1
    ;;
esac