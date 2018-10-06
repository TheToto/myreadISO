#!/bin/sh

[ "$#" -ne 2 ] && exit 1
for i in $(seq 1 5);do
  echo Test "$i" :
  cat -n tests/command"$i"
  "$1" "$2" < tests/command"$i" > tests/out"$i"
  if [ $? -eq 0 ];then
    echo No diff : OK
  else
    echo diff != 0 : FAIL
  fi
  echo $(diff tests/out"$i" tests/res_command"$i")
done
echo Test get :
diff README.TXT tests/res_README.TXT
if [ $? -eq 0 ];then
  echo No diff : OK
else
  echo diff != 0 : FAIL
fi

rm tests/out*
rm README.TXT
