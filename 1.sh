#!/sh

# Start of file. This line should not cause errors
echo Command 000
echo Command 001
echo Command 002
ehchhho Command 003
echo Command 004
echo Command 005
echo Command 006 # Look, it's command 6. This shouldn't print
echo Command 007
echo Command 008
# Empty commands (like this one) and blank lines should not go in history





/ls /
mkdir a
mkdir a/b
mkdir a/b/c
mkdir a/b/c/hello
cd a
cd b/c/hello
/ls
/wc /README.md
cd /
/wc README.md
echo ---
history
echo ---
history -t
exit
echo If you see this line, exit is not working.
