success=0
failure=0

for i in {1..100}; do
  echo "=== round $i ==="

  curl -sS -o /tmp/random.out -w "random: %{http_code}\n" \
    http://192.168.29.70:8080/random &
  p1=$!

  curl -sS -o /tmp/root.out -w "root:   %{http_code}\n" \
    http://192.168.29.70:8080/ &
  p2=$!

  wait $p1
  r1=$?

  wait $p2
  r2=$?

  if [ "$r1" -eq 0 ]; then
    success=$((success + 1))
  else
    failure=$((failure + 1))
  fi

  if [ "$r2" -eq 0 ]; then
    success=$((success + 1))
  else
    failure=$((failure + 1))
  fi
done

echo
echo "=== summary ==="
echo "successes: $success"
echo "failures:  $failure"
echo "total:     $((success + failure))"
