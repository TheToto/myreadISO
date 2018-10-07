#!/bin/sh

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

[ "$#" -ne 2 ] && exit 1
for i in $(seq 1 5);do
  echo Test "$i" :
  cat -n tests/command"$i"
  "$1" "$2" < tests/command"$i" > tests/out"$i"
  if [ $? -eq 0 ];then
    printf "${GREEN}Return 0 : OK${NC}\n"
  else
    printf "${RED}Return $? : FAIL${NC}\n"
  fi
  diff tests/out"$i" tests/res_command"$i"
  if [ $? -eq 0 ];then
    printf "${GREEN}No diff : OK${NC}\n"
  else
    printf "${RED}diff != 0 : FAIL${NC}\n"
  fi
  echo
done
echo Test get :
diff README.TXT tests/res_README.TXT
if [ $? -eq 0 ];then
  printf "${GREEN}No diff : OK${NC}\n"
else
  printf "${RED}diff != 0 : FAIL${NC}\n"
fi

rm tests/out*
rm README.TXT
