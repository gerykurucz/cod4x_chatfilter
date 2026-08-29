# COD4X Chat Filter Plugin
**Version:** 3.2

A powerful server-side plugin that filters chat messages to maintain a clean and respectful gaming environment. It blocks non-ASCII characters, obscene language, cheat accusations, and toxic behavior with intelligent word matching.

---

## Features

### Chat Filtering Capabilities
- **Non-ASCII Blocking** – Rejects extended ASCII, Unicode, or special symbols in chat
- **Color Code Stripping** – Processes messages after removing `^` color codes
- **Leet Speak Normalization** – Converts common leet characters (e.g., `@`→`a`, `4`→`a`, `$`→`s`, `0`→`o`)
- **Repeated Character Normalization** – Handles spam like `haaaack` or `shiiiiit`
- **Case Insensitive** – All matching is performed in lowercase

### Smart Word Matching Modes
| Mode | Description | Example |
|------|-------------|---------|
| **Auto** (default) | 1-3 chars: exact match; 4+ chars: stem matching | `wh` → exact; `hack` → stem |
| **Exact** | Matches complete words only | `exact:ass` blocks "ass" but not "class" |
| **Stem** | Matches word stems with common suffixes | `hack` blocks "hacker", "hacking", "hacks" |
| **Contains** | Matches any occurrence in message | `contains:fuck` blocks all variations |

### Stem Matching Suffixes
The following suffixes are recognized for stem matching:
`s`, `es`, `ed`, `d`, `er`, `ers`, `ing`, `ings`, `y`, `ies`, `ish`, `ism`, `ist`, `ists`

Example: `hack` blocks → hack, hacks, hacked, hacker, hackers, hacking, hackings

---

## Installation

1. Download or compile the plugin (see below).
2. Place the compiled `chatfilter.so` (Linux) or `chatfilter.dll` (Windows) into the server's `plugins` folder.
3. Copy `blocked_words.txt` to the `plugins` folder.
4. Restart the server or load the plugin manually.

---

## Configuration

### Blocked Words File (`plugins/blocked_words.txt`)

Edit the `blocked_words.txt` file to customize filtered words:

```
# Comments start with #
# Format: [mode:]word

# Auto mode (default) - smart matching based on length
hack
cheat
wh

# Exact match only
exact:ass
exact:dick

# Stem matching (for words > 3 chars)
stem:hacker

# Contains anywhere in message
contains:motherfuck
```

**Default Categories:**
- Cheat accusations (wh, esp, hack, aimbot, wallhack, etc.)
- Severe profanity (fuck, shit, bitch, etc.)
- Strong insults (bastard, cunt, whore, idiot, moron, etc.)
- Short obscene words (ass, dick, cock, tit, tits)
- Family-related terms (mom, dad, mother, father, etc.)
- Sexual/degrading insults (pussy, cocksucker, pervert, etc.)
- Gameplay accusations (spawnkill, walling, tracking, etc.)

---

## How It Works

### Message Processing Pipeline

1. **ASCII Check** – Immediately blocks messages with non-ASCII characters (>127)
2. **Color Code Removal** – Strips `^X` color codes from the message
3. **Lowercase Conversion** – Converts entire message to lowercase
4. **Normalization Checks** – Tests multiple forms:
   - Original cleaned message
   - With repeated chars reduced to 2 (e.g., `haaack` → `haack`)
   - With repeated chars reduced to 1 (e.g., `haaack` → `hack`)
   - Leet speak normalized (e.g., `h4ck` → `hack`)
   - Leet + repeated char combinations

5. **Word Matching** – Checks against blocked words list using appropriate mode

### Notification
When a non-ASCII message is detected, the sender receives:
```
^1Only English characters are allowed.
```

Blocked words are silently hidden (no notification to user).

---

## Validation Logic

| Rule | Valid Example | Invalid Example |
|------|---------------|-----------------|
| ASCII Only | `Good game!`, `Nice shot^1` | `Ñoño`, `Путин`, `日本語` |
| Color Codes | `^1Red^2Team`, `GG^7` | (processed after stripping) |
| Leet Speak | `hello` | `h3ll0`, `4ss`, `$hit` |
| Repeated Chars | `nice` | `niiiiiice`, `shiiiiit` |
| Blocked Words | `especially` | `esp`, `hacker`, `cheater` |

**In short: Messages must contain only ASCII characters. Blocked words are detected even with leet speak substitutions, repeated characters, or common suffixes.**

---

## Building on Linux

The plugin is 32‑bit (COD4X server requirement).

### Requirements
- GCC with 32‑bit support
- 32-bit glibc development headers
- COD4X Plugin SDK (`api/` folder)
- Cloning the repository

### Debian / Ubuntu

```bash
sudo dpkg --add-architecture i386
sudo apt update
sudo apt install gcc-multilib libc6-dev-i386
cd cod4x_chatfilter
make clean && make
```

### Fedora

```bash
sudo dnf install gcc make glibc-devel.i686 libgcc.i686
cd cod4x_chatfilter
make clean && make
```

### Arch

```bash
sudo pacman -S gcc make lib32-glibc lib32-gcc-libs
cd cod4x_chatfilter
make clean && make
```

## Output: build/chatfilter.so

---

## Troubleshooting

**Plugin doesn't load:**
- Ensure file is in `plugins` folder
- Verify server is 32-bit compatible
- Check `blocked_words.txt` exists in `plugins` folder

**Legitimate messages blocked:**
- Review `blocked_words.txt` for overly broad entries
- Use `exact:` prefix for short words to avoid substring matches
- Remove or comment out categories that are too strict for your server

**Players complaining about false positives:**
- Short words (1-3 chars) use exact matching by default
- Consider removing family-related terms if too restrictive
- Adjust `contains:` entries carefully as they match anywhere

---

## Server Logging

Blocked messages are logged to server console:
```
Chat Filter: blocked non-ASCII message from slot <player_slot>
Chat Filter: blocked entry '<word>' using mode <mode_id> from slot <player_slot>
Chat Filter: loaded <count> blocked words
```

Mode IDs: 0=Auto, 1=Exact, 2=Stem, 3=Contains

---

## Default Blocked Words Summary

The included `blocked_words.txt` contains ~150 entries covering:

| Category | Count | Examples |
|----------|-------|----------|
| Cheat Terms | 25+ | wh, esp, hack, aimbot, wallhack |
| Severe Profanity | 15+ | fuck, shit, bitch, asshole |
| Insults | 40+ | bastard, idiot, moron, scumbag |
| Short Words | 5 | ass, dick, cock, tit, tits |
| Family Terms | 8 | mom, dad, mother, father |
| Sexual Insults | 15+ | pussy, cocksucker, pervert |
| Gameplay | 10+ | spawnkill, camping, walling |

Customize this list to match your server's community standards!

---

## Credits

**Developers:** Teo ([@obteo](https://github.com/obteo)) & XV9K ([@sudoxv9k](https://github.com/sudoxv9k))

**Platform:** COD4X (https://cod4x.ovh)

**Disclaimer:** This plugin helps maintain chat quality but may require manual tuning for edge cases. Use alongside other moderation tools. By using this software you agree with the ([LICENSE](https://github.com/sudoxv9k/cod4x_chatfilter/blob/main/LICENSE)).


