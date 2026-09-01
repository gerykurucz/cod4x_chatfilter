/*
 * ============================================================
 *  Chat Filter - COD4X Plugin
 *  Version: 3.2
 *  Developers: Teo(github.com/obteo) & Gery Kurucz (github.com/gerykurucz)
 *  LICENSE: https://github.com/gerykurucz/cod4x_chatfilter/blob/main/LICENSE
 * ============================================================
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "api/pinc.h"

#define BLOCKED_WORDS_FILE "plugins/blocked_words.txt"
#define MAX_BLOCKED_WORDS 512
#define MAX_WORD_LENGTH   64
#define MAX_CHAT_LENGTH   1024
#define Q_COLOR_ESCAPE '^'

/* Change only this macro if pinc.h uses another private-message function. */
#define CHATFILTER_NOTIFY_CLIENT(clientSlot, text) \
    Plugin_ChatPrintf((clientSlot), (text))

typedef enum
{
    MATCH_AUTO = 0,
    MATCH_EXACT,
    MATCH_STEM,
    MATCH_CONTAINS
} matchMode_t;

typedef struct
{
    char text[MAX_WORD_LENGTH];
    matchMode_t mode;
} blockedWord_t;

static blockedWord_t blockedWords[MAX_BLOCKED_WORDS];
static int blockedWordsCount = 0;

static qboolean IsColorCode(const char *text)
{
    if(!text)
        return qfalse;

    return text[0] == Q_COLOR_ESCAPE && text[1] != '\0';
}

static void RemoveColors(char *text)
{
    size_t readPosition = 0;
    size_t writePosition = 0;

    if(!text)
        return;

    while(text[readPosition] != '\0')
    {
        if(IsColorCode(&text[readPosition]))
        {
            readPosition += 2;
            continue;
        }

        text[writePosition++] = text[readPosition++];
    }

    text[writePosition] = '\0';
}

static void TrimString(char *text)
{
    char *start;
    char *end;
    size_t length;

    if(!text)
        return;

    start = text;

    while(*start && isspace((unsigned char)*start))
        start++;

    if(start != text)
        memmove(text, start, strlen(start) + 1);

    length = strlen(text);

    if(length == 0)
        return;

    end = text + length - 1;

    while(end >= text && isspace((unsigned char)*end))
    {
        *end = '\0';
        end--;
    }
}

static void StringToLower(char *text)
{
    size_t i;

    if(!text)
        return;

    for(i = 0; text[i] != '\0'; i++)
        text[i] = (char)tolower((unsigned char)text[i]);
}

static void NormalizeLeetChars(char *text)
{
    size_t i;

    if(!text)
        return;

    for(i = 0; text[i] != '\0'; i++)
    {
        switch(text[i])
        {
            case '@':
            case '4': text[i] = 'a'; break;
            case '$':
            case '5': text[i] = 's'; break;
            case '0': text[i] = 'o'; break;
            case '1':
            case '!':
            case '|': text[i] = 'i'; break;
            case '3': text[i] = 'e'; break;
            case '7': text[i] = 't'; break;
            case '8': text[i] = 'b'; break;
            default: break;
        }
    }
}

static void NormalizeRepeatedChars(
    const char *source,
    char *destination,
    size_t destinationSize,
    int maximumRepeats
)
{
    size_t readPosition = 0;
    size_t writePosition = 0;
    int repeatCount;
    char currentCharacter;

    if(!source || !destination || destinationSize == 0)
        return;

    destination[0] = '\0';

    if(maximumRepeats < 1)
        maximumRepeats = 1;

    while(source[readPosition] != '\0' && writePosition < destinationSize - 1)
    {
        currentCharacter = source[readPosition];
        repeatCount = 0;

        while(source[readPosition] == currentCharacter)
        {
            repeatCount++;
            readPosition++;
        }

        if(repeatCount > maximumRepeats)
            repeatCount = maximumRepeats;

        while(repeatCount > 0 && writePosition < destinationSize - 1)
        {
            destination[writePosition++] = currentCharacter;
            repeatCount--;
        }
    }

    destination[writePosition] = '\0';
}

static qboolean IsWordCharacter(char character)
{
    unsigned char value = (unsigned char)character;
    return isalnum(value) || character == '_';
}

static qboolean ContainsExactWord(const char *message, const char *word)
{
    const char *match;
    size_t wordLength;

    if(!message || !word || word[0] == '\0')
        return qfalse;

    wordLength = strlen(word);
    match = message;

    while((match = strstr(match, word)) != NULL)
    {
        qboolean validBefore;
        qboolean validAfter;

        validBefore = match == message || !IsWordCharacter(match[-1]);
        validAfter = match[wordLength] == '\0' || !IsWordCharacter(match[wordLength]);

        if(validBefore && validAfter)
            return qtrue;

        match++;
    }

    return qfalse;
}

static const char *allowedStemSuffixes[] =
{
    "", "s", "es", "ed", "d", "er", "ers", "ing", "ings",
    "y", "ies", "ish", "ism", "ist", "ists", NULL
};

static qboolean IsAllowedStemSuffix(const char *suffix)
{
    int i;

    if(!suffix)
        return qfalse;

    for(i = 0; allowedStemSuffixes[i] != NULL; i++)
    {
        if(strcmp(suffix, allowedStemSuffixes[i]) == 0)
            return qtrue;
    }

    return qfalse;
}

static qboolean ContainsStemWord(const char *message, const char *stem)
{
    const char *position;
    size_t stemLength;

    if(!message || !stem || stem[0] == '\0')
        return qfalse;

    stemLength = strlen(stem);
    position = message;

    while(*position)
    {
        const char *tokenStart;
        const char *tokenEnd;
        size_t tokenLength;
        size_t suffixLength;
        char suffix[MAX_WORD_LENGTH];

        while(*position && !IsWordCharacter(*position))
            position++;

        if(!*position)
            break;

        tokenStart = position;

        while(*position && IsWordCharacter(*position))
            position++;

        tokenEnd = position;
        tokenLength = (size_t)(tokenEnd - tokenStart);

        if(tokenLength < stemLength)
            continue;

        if(strncmp(tokenStart, stem, stemLength) != 0)
            continue;

        suffixLength = tokenLength - stemLength;

        if(suffixLength >= sizeof(suffix))
            continue;

        memcpy(suffix, tokenStart + stemLength, suffixLength);
        suffix[suffixLength] = '\0';

        if(IsAllowedStemSuffix(suffix))
            return qtrue;
    }

    return qfalse;
}

static matchMode_t ResolveMatchMode(const blockedWord_t *blockedWord)
{
    if(!blockedWord)
        return MATCH_EXACT;

    if(blockedWord->mode != MATCH_AUTO)
        return blockedWord->mode;

    if(strlen(blockedWord->text) <= 3)
        return MATCH_EXACT;

    return MATCH_STEM;
}

static qboolean DoesBlockedWordMatch(const char *message, const blockedWord_t *blockedWord)
{
    matchMode_t mode;

    if(!message || !blockedWord)
        return qfalse;

    mode = ResolveMatchMode(blockedWord);

    switch(mode)
    {
        case MATCH_EXACT:
            return ContainsExactWord(message, blockedWord->text);

        case MATCH_STEM:
            return ContainsStemWord(message, blockedWord->text);

        case MATCH_CONTAINS:
            return strstr(message, blockedWord->text) != NULL;

        case MATCH_AUTO:
        default:
            return qfalse;
    }
}

static qboolean IsDuplicateBlockedWord(const char *text, matchMode_t mode)
{
    int i;

    if(!text)
        return qfalse;

    for(i = 0; i < blockedWordsCount; i++)
    {
        if(blockedWords[i].mode == mode && strcmp(blockedWords[i].text, text) == 0)
            return qtrue;
    }

    return qfalse;
}

static void LoadBlockedWords(void)
{
    FILE *file;
    char line[256];

    blockedWordsCount = 0;
    memset(blockedWords, 0, sizeof(blockedWords));

    file = fopen(BLOCKED_WORDS_FILE, "r");

    if(!file)
    {
        Plugin_Printf("Chat Filter: %s not found\n", BLOCKED_WORDS_FILE);
        return;
    }

    while(fgets(line, sizeof(line), file))
    {
        char *wordText;
        matchMode_t matchMode;
        size_t wordLength;

        TrimString(line);

        if(line[0] == '\0' || line[0] == '#')
            continue;

        matchMode = MATCH_AUTO;
        wordText = line;

        if(strncmp(line, "exact:", 6) == 0)
        {
            matchMode = MATCH_EXACT;
            wordText = line + 6;
        }
        else if(strncmp(line, "stem:", 5) == 0)
        {
            matchMode = MATCH_STEM;
            wordText = line + 5;
        }
        else if(strncmp(line, "contains:", 9) == 0)
        {
            matchMode = MATCH_CONTAINS;
            wordText = line + 9;
        }

        TrimString(wordText);
        StringToLower(wordText);

        if(wordText[0] == '\0')
            continue;

        wordLength = strlen(wordText);

        if(wordLength >= MAX_WORD_LENGTH)
        {
            Plugin_Printf(
                "Chat Filter: ignored entry longer than %i characters: %s\n",
                MAX_WORD_LENGTH - 1,
                wordText
            );
            continue;
        }

        if(IsDuplicateBlockedWord(wordText, matchMode))
            continue;

        if(blockedWordsCount >= MAX_BLOCKED_WORDS)
        {
            Plugin_Printf("Chat Filter: maximum of %i blocked words reached\n", MAX_BLOCKED_WORDS);
            break;
        }

        strncpy(blockedWords[blockedWordsCount].text, wordText, MAX_WORD_LENGTH - 1);
        blockedWords[blockedWordsCount].text[MAX_WORD_LENGTH - 1] = '\0';
        blockedWords[blockedWordsCount].mode = matchMode;
        blockedWordsCount++;
    }

    fclose(file);

    Plugin_Printf("Chat Filter: loaded %i blocked words\n", blockedWordsCount);
}

static const blockedWord_t *FindBlockedWord(const char *message)
{
    int i;

    if(!message)
        return NULL;

    for(i = 0; i < blockedWordsCount; i++)
    {
        if(DoesBlockedWordMatch(message, &blockedWords[i]))
            return &blockedWords[i];
    }

    return NULL;
}

static const blockedWord_t *FindBlockedWordInAllForms(const char *cleanMessage)
{
    char repeatedDouble[MAX_CHAT_LENGTH];
    char repeatedSingle[MAX_CHAT_LENGTH];
    char leetMessage[MAX_CHAT_LENGTH];
    char leetRepeatedDouble[MAX_CHAT_LENGTH];
    char leetRepeatedSingle[MAX_CHAT_LENGTH];
    const blockedWord_t *result;

    if(!cleanMessage)
        return NULL;

    result = FindBlockedWord(cleanMessage);
    if(result)
        return result;

    NormalizeRepeatedChars(cleanMessage, repeatedDouble, sizeof(repeatedDouble), 2);
    result = FindBlockedWord(repeatedDouble);
    if(result)
        return result;

    NormalizeRepeatedChars(cleanMessage, repeatedSingle, sizeof(repeatedSingle), 1);
    result = FindBlockedWord(repeatedSingle);
    if(result)
        return result;

    strncpy(leetMessage, cleanMessage, sizeof(leetMessage) - 1);
    leetMessage[sizeof(leetMessage) - 1] = '\0';
    NormalizeLeetChars(leetMessage);

    result = FindBlockedWord(leetMessage);
    if(result)
        return result;

    NormalizeRepeatedChars(leetMessage, leetRepeatedDouble, sizeof(leetRepeatedDouble), 2);
    result = FindBlockedWord(leetRepeatedDouble);
    if(result)
        return result;

    NormalizeRepeatedChars(leetMessage, leetRepeatedSingle, sizeof(leetRepeatedSingle), 1);
    return FindBlockedWord(leetRepeatedSingle);
}

PCL void OnMessageSent(char *message, int slot, qboolean *show, int mode)
{
    size_t i = 0;
    size_t messageStart = 0;
    char checkMessage[MAX_CHAT_LENGTH];
    const blockedWord_t *blockedWord;

    (void)mode;

    if(!message || !show || message[0] == '\0')
        return;

    /* Preserve original behavior: B3 commands are not excluded. */
    if((unsigned char)message[0] == 0x15)
    {
        messageStart = 1;
        i = 1;
    }

    while(message[i] != '\0')
    {
        if(IsColorCode(&message[i]))
        {
            i += 2;
            continue;
        }

        if((unsigned char)message[i] > 127)
        {
            Plugin_Printf("Chat Filter: blocked non-ASCII message from slot %i\n", slot);
            *show = qfalse;

            CHATFILTER_NOTIFY_CLIENT(
                slot,
                "^1Only English characters are allowed."
            );

            return;
        }

        i++;
    }

    strncpy(checkMessage, message + messageStart, sizeof(checkMessage) - 1);
    checkMessage[sizeof(checkMessage) - 1] = '\0';

    RemoveColors(checkMessage);
    StringToLower(checkMessage);

    blockedWord = FindBlockedWordInAllForms(checkMessage);

    if(blockedWord)
    {
        Plugin_Printf(
            "Chat Filter: blocked entry '%s' using mode %i from slot %i\n",
            blockedWord->text,
            (int)ResolveMatchMode(blockedWord),
            slot
        );

        /* Forbidden words are silently hidden. */
        *show = qfalse;
        return;
    }
}

PCL int OnInit(void)
{
    LoadBlockedWords();

    Plugin_Printf(
        "Chat Filter has been started.\n"
    );

    return 0;
}

PCL void OnInfoRequest(pluginInfo_t *info)
{
    if(!info)
        return;

    info->handlerVersion.major = PLUGIN_HANDLER_VERSION_MAJOR;
    info->handlerVersion.minor = PLUGIN_HANDLER_VERSION_MINOR;
    info->pluginVersion.major = 3;
    info->pluginVersion.minor = 2;

    strncpy(info->fullName, "Chat Filter", sizeof(info->fullName) - 1);
    info->fullName[sizeof(info->fullName) - 1] = '\0';

    strncpy(info->shortDescription, "Blocks non-ASCII characters and forbidden words from chat", sizeof(info->shortDescription) - 1);
    info->shortDescription[sizeof(info->shortDescription) - 1] = '\0';

    strncpy(info->longDescription, "Hides chat messages containing non-ASCII characters or configured blocked words", sizeof(info->longDescription) - 1);
    info->longDescription[sizeof(info->longDescription) - 1] = '\0';
}
