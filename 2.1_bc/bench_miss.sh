#!/usr/bin/env bash
# bench_miss.sh —— 一键采集 6 种循环顺序的 Cache miss 计数器
# 输出: perf_<order>.txt + summary.csv
#
# 前置: ./gemm_one 已经编译好；perf 已安装；perf_event_paranoid <= 1
#
# 使用: chmod +x bench_miss.sh && ./bench_miss.sh

set -euo pipefail

ORDERS=(ijk ikj jik jki kij kji)
EVENTS="cycles,instructions,L1-dcache-loads,L1-dcache-load-misses,LLC-loads,LLC-load-misses"
REPEAT=3

# 输出表头
echo "order,cycles,instructions,ipc,l1_loads,l1_misses,l1_miss_rate,llc_loads,llc_misses,llc_miss_rate,time_s" > summary.csv

for o in "${ORDERS[@]}"; do
    echo "===========  $o  ==========="
    log="perf_${o}.txt"

    # -x, 用","做分隔，便于解析
    perf stat -r "$REPEAT" -x , -e "$EVENTS" ./gemm_one "$o" 2> "$log"
    cat "$log"

    # 提取数值 ( perf -x, 输出格式: 数值,unit,event_name,... )
    cycles=$(awk -F, '$3=="cycles"{print $1}' "$log" | head -1)
    insns=$(awk -F, '$3=="instructions"{print $1}' "$log" | head -1)
    l1l=$(awk -F, '$3=="L1-dcache-loads"{print $1}' "$log" | head -1)
    l1m=$(awk -F, '$3=="L1-dcache-load-misses"{print $1}' "$log" | head -1)
    llcl=$(awk -F, '$3=="LLC-loads"{print $1}' "$log" | head -1)
    llcm=$(awk -F, '$3=="LLC-load-misses"{print $1}' "$log" | head -1)
    tsec=$(awk -F, '/seconds time elapsed/{print $1}' "$log" | head -1)

    ipc=$(awk -v a="$insns" -v b="$cycles" 'BEGIN{ if(b>0) printf "%.3f", a/b; else print "0" }')
    l1r=$(awk -v a="$l1m" -v b="$l1l" 'BEGIN{ if(b>0) printf "%.4f", a/b; else print "0" }')
    llcr=$(awk -v a="$llcm" -v b="$llcl" 'BEGIN{ if(b>0) printf "%.4f", a/b; else print "0" }')

    echo "$o,$cycles,$insns,$ipc,$l1l,$l1m,$l1r,$llcl,$llcm,$llcr,$tsec" >> summary.csv
done

echo
echo "=========== Summary (summary.csv) ==========="
column -s, -t < summary.csv
