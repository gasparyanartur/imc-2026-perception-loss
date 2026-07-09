# Quick Start

## 1. Build
```
cd lemon/lemon_v30
./build.sh
```

## 2. Test offline
```
for f in ../../data/synth_bench/*.obj; do
  echo "$(basename $f): $(timeout 30 ./Sharon < $f 2>/dev/null | head -1)"
done
```

## 3. Submit
```
cp lemon/lemon_v30/Sharon.cpp lemon_v<N>.cpp
python3 tools/limekit/submit.py lemon_v<N>.cpp "lemon_v<N>.cpp" lemon \
  --hypothesis "what changed" --predicted "score range"
```

## 4. Check status
```
python3 -c "
import subprocess, json
url = 'https://imc2-cvmaxxing.arturspace.dev/submission/<SUB_ID>'
r = subprocess.run(['curl', '-m', '10', '-s', url], capture_output=True, text=True)
d = json.loads(r.stdout)
print('Status:', d.get('status'))
print('Score:', d.get('score'))
print('Cases:', d.get('cases'))
"
```

## 5. Best version so far

`lemon_v_BEST_NON_DYNAMIC.cpp` — Score 89.30, all cases pass (PPPPPPP).

This is the canonical "non-dynamic" (frozen tier) version. Use this as the starting baseline.
