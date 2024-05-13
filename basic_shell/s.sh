#!/mnt/e/SO/basic_shell/main
echo "######################################"
echo "# Witam w skrypcie testującym shella #"
echo "######################################"
sleep 1
echo "1. Sprawdzanie wykonania procesów w tle przy użyciu znaku & na końcu linii"
sleep 1
echo "Wykonuję polecenie sleep 5 &"
sleep 5 &
echo "Testowanie przekierowania wyjścia"
sleep 1
echo "Wykonuję polecenie ls >> output.txt"
ls >> output.txt
echo "Wypisuję plik output.txt na ekran:"
cat output.txt
sleep 1
echo "Testowanie tworzenia potoków"
sleep 1
echo "Wykonuję polecenie ls -l | grep main | wc"
ls -l | grep main | wc
sleep 1
echo "W celu przetestowania możliwości wyświetlenia historii poleceń"
echo "proszę wprowadzić kombinację klawiszy Ctrl+\\"
./main


