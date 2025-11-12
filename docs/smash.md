# smàsh

`smàsh` is a simple Unix-like shell implemented for educational purposes. It supports basic built-in commands, history tracking, aliases, input/output redirection, and pipelines. The shell prompts the user with a colorful prompt displaying the exit status of the last command, the command number, and the current working directory.

## Features

### Prompt

The shell prompt has the following format:

```
[<last_status>]-[<cmd_number>]─[<current_directory>]$ 
```

* `<last_status>`: Exit status of the last command.
* `<cmd_number>`: Number of commands executed so far.
* `<current_directory>`: Current working directory.

### Built-in Commands

* `cd <directory>`: Change the current working directory.
* `exit`: Exit the shell.
* `history`: Show the list of previously executed commands.
* `alias`: Create or show command aliases.
* `# <comment>`: Lines starting with `#` are treated as comments and ignored.

### History Execution

The shell supports history execution using the following syntax:

* `!!`: Execute the last command.
* `!<number>`: Execute the command with the specified history number.

### Input/Output Redirection

* `>`: Redirect stdout to a file (overwrite).

### Pipes

* Commands can be connected using the `|` operator to create pipelines.
* Supports multiple commands in a pipeline.
* Each command in a pipeline can optionally use input/output redirection.

### Aliases

* Create a new alias: `alias name="value"`
* List all aliases: `alias`
* Aliases allow substituting one command for another.

### Command Execution

* Supports both foreground and background execution.
* Background commands end with `&` and do not block the shell prompt.

### Limitations

* Supports a fixed number of aliases (32) and history entries (100).
* Does not support complex shell scripting constructs.

### Example Usage

```bash
[0]-[1]─[/]$ echo hello > file.txt
[0]-[2]─[/]$ cat file.txt
hello
[0]-[3]─[/]$ echo again >> file.txt
[0]-[4]─[/]$ cat file.txt
hello
again
[0]-[5]─[/]$ ls | sort
(sorted output)
[0]-[6]─[/]$ cd /tests
[0]-[7]─[/]$ history
 0 echo hello > file.txt
  1 cat file.txt
   2 echo again >> file.txt
    3 cat file.txt
     4 ls | sort
      5 cd /tests
      [0]-[8]─[/]$ !cat
      cat file.txt
      hello
      again
      ```

      `smàsh` provides a lightweight and educational environment to explore shell behavior, I/O redirection, pipelines, history management, and aliasing.
      
