# a nino fork

A small terminal-based text editor.

## Features

- Familiar keybindings
- UTF-8 support
- Mouse support
- Cut, copy, paste
- Undo, redo
- Search
- Tabs

## Setup

### Requirements

- gcc
- make

### Building

```bash
make
```

### Installing

```bash
make install
```

## Configuration

Configurations are set by running commands.

Use `Ctrl+P` to open the command prompt.

To run commands on start, create `ninorc` in the configuration directory.

Configuration directory:

- `~/.config/nino`

The `exec` command executes a config file. The command will first search for
the file in the current directory, then the configuration directory.

## Commands and ConVars

| Name    | Default | Description                               |
| ------- | ------- | ----------------------------------------- |
| color   | cmd     | Change the color of an element.           |
| exec    | cmd     | Execute a config file.                    |
| newline | cmd     | Set the EOL sequence (LF/CRLF).           |
| echo    | cmd     | Clear all console output.                 |
| help    | cmd     | Find help about a convar/concommand.      |
| find    | cmd     | Find concommands by their name/help text. |

## Color

`color <element> [color]`

When color code is `000000` it will be transparent.

### Default Theme

| Element        | Default |
| -------------- | ------- |
| bg             | 1e1e1e  |
| top.fg         | e5e5e5  |
| top.bg         | 252525  |
| top.tabs.fg    | 969696  |
| top.tabs.bg    | 2d2d2d  |
| top.select.fg  | e5e5e5  |
| top.select.bg  | 575068  |
| prompt.fg      | e5e5e5  |
| prompt.bg      | 3c3c3c  |
| status.fg      | e1dbef  |
| status.bg      | 575068  |
| status.lang.fg | e1dbef  |
| status.lang.bg | a96b21  |
| status.pos.fg  | e1dbef  |
| status.pos.bg  | d98a2b  |
| lineno.fg      | 7f7f7f  |
| lineno.bg      | 1e1e1e  |
| cursorline     | 282828  |
| hl.normal      | e5e5e5  |
| hl.comment     | 6a9955  |
| hl.keyword1    | c586c0  |
| hl.keyword2    | 569cd6  |
| hl.keyword3    | 4ec9b0  |
| hl.string      | ce9178  |
| hl.number      | b5cea8  |
| hl.space       | 3f3f3f  |
| hl.match       | 592e14  |
| hl.select      | 264f78  |
| hl.trailing    | ff6464  |

## Controls

| Action                        | Keybinding          |
| ----------------------------- | ------------------- |
| Quit                          | Ctrl+Q              |
| Close Tab                     | Ctrl+W              |
| Open File                     | Ctrl+O              |
| Save                          | Ctrl+S              |
| Save All                      | Alt+Ctrl+S          |
| Save As                       | Ctrl+N              |
| Previous Tab                  | Ctrl+[              |
| Next Tab                      | Ctrl+]              |
| Prompt                        | Ctrl+P              |
| Find                          | Ctrl+F              |
| Copy                          | Ctrl+C              |
| Paste                         | Ctrl+V              |
| Cut                           | Ctrl+X              |
| Undo                          | Ctrl+Z              |
| Redo                          | Ctrl+Y              |
| Copy Line Up                  | Shift+Alt+Up        |
| Copy Line Down                | Shift+Alt+Down      |
| Move Line Up                  | Alt+Up              |
| Move Line Down                | Alt+Down            |
| Go To Line                    | Ctrl+G              |
| Move Up                       | Up                  |
| Move Down                     | Down                |
| Move Right                    | Right               |
| Move Left                     | Left                |
| Move Word Right               | Ctrl+Right          |
| Move Word Left                | Ctrl+Left           |
| To Line Start                 | Home                |
| To Line End                   | End                 |
| To File Start                 | Ctrl+Home           |
| To File End                   | Ctrl+End            |
| To Next Page                  | PageUp              |
| To Previous Page              | PageDown            |
| To Next Blank Line            | Ctrl+PageUp         |
| To Previous Blank Line        | Ctrl+PageDown       |
| Scroll Line Up                | Ctrl+Up             |
| Scroll Line Down              | Ctrl+Down           |
| Select All                    | Ctrl+A              |
| Select Word                   | Ctrl+D              |
| Select Move Up                | Shift+Up            |
| Select Move Down              | Shift+Down          |
| Select Move Right             | Shift+Right         |
| Select Move Left              | Shift+Left          |
| Select Move Word Right        | Shift+Ctrl+Right    |
| Select Move Word Left         | Shift+Ctrl+Left     |
| Select To Line Start          | Shift+Home          |
| Select To Line End            | Shift+End           |
| Select To Next Page           | Shift+PageUp        |
| Select To Previous Page       | Shift+PageDown      |
| Select To Next Blank Line     | Shift+Ctrl+PageUp   |
| Select To Previous Blank Line | Shift+Ctrl+PageDown |

The terminal emulator might have some keybinds overlapping with nino, make sure
to change them before using.
