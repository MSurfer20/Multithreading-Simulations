#!/bin/zsh
autoload -U colors && colors

make

echo ""
echo ""
echo ""

correct_tests=0
incorrect_tests=0

for test in ./tests/in/*.*
do
    echo "$fg_bold[green]Running file $test"$reset_color
    echo "$fg_bold[yellow]========================================================="$reset_color

    ./q1 < $test

    if [ $? -eq 0 ]; then
        echo "$fg_bold[yellow]========================================================="$reset_color
        echo "$fg_bold[green]Test $test passed"$reset_color
        correct_tests=$(echo "$correct_tests + 1" | bc)
    else
        echo "$fg_bold[yellow]========================================================="$reset_color
        echo "$fg_bold[red]Test $test failed"$reset_color
        incorrect_tests=$(echo "$incorrect_tests + 1" | bc)
    fi

    echo "$fg_bold[yellow]========================================================="$reset_color
    echo ""
    notify-send -t 5000 -i "alacritty" "Test $test" "Completed"
done

total_tests=$(echo "$incorrect_tests + $correct_tests" | bc)
echo "Total: $total_tests; Correct: $correct_tests;  Incorrect $incorrect_tests"
