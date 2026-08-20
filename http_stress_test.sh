success=0
failure=0

base_url="http://192.168.29.70:8080"

paths=(
  "/STYLE.CSS"
  "/APP.JS"
  "/TEST.TXT"
  "/favicon.ico"
  "/random"
)

for i in {1..100}; do
  echo "=== round $i ==="

  idx=$(( (i - 1) % ${#paths[@]} ))
  test_path="${paths[$idx]}"

  curl -sS -o /tmp/root.out -w "root:     %{http_code}\n" \
    "$base_url/" &
  p1=$!

  curl -sS -o /tmp/test.out -w "mixed:    %{http_code}  $test_path\n" \
    "$base_url$test_path" &
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
echo "=== path validation ==="

curl --path-as-is -sS -o /dev/null -w "/../TEST.TXT:        %{http_code}\n" \
  "$base_url/../TEST.TXT"

curl --path-as-is -sS -o /dev/null -w "/foo/../TEST.TXT:    %{http_code}\n" \
  "$base_url/foo/../TEST.TXT"

curl --path-as-is -sS -o /dev/null -w "/foo\\bar.txt:        %{http_code}\n" \
  "$base_url/foo\\bar.txt"

curl --path-as-is -sS -o /dev/null -w "/foo:bar.txt:         %{http_code}\n" \
  "$base_url/foo:bar.txt"

echo
echo "=== summary ==="
echo "time:      $(date '+%d %b %Y, %I:%M %p')"
echo "successes: $success"
echo "failures:  $failure"
echo "total:     $((success + failure))"
