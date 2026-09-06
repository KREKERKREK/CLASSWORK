#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>



char** readWords(int* wordCount);
void printWords(char** words, int wordCount);
char** reverseWords(char** words, int wordCount);
char** extractNumberFromFirstWord(char** words, int* ptr_word_count, int* number);
void reverseWord(char* word);
void ReverseOneWord(char** user_words, int user_number);



char** readWords(int* wordCount) {
    int capacity = 10;  // начальная вместимость массива слов
    char** words = (char**)malloc(sizeof(char*) * capacity);
    if (!words) {
        *wordCount = 0;
        return nullptr;
    }
    
    *wordCount = 0;
    char ch;
    char* currentWord = nullptr;
    int wordLen = 0;
    int wordCap = 10;
    bool prevWasExclamation = false;
    bool endOfText = false;
    
    currentWord = (char*)malloc(sizeof(char) * wordCap);
    if (!currentWord) {
        free(words);
        *wordCount = 0;
        return nullptr;
    }
    
    while (!endOfText) {
        ch = getchar();
        
        // Проверяем конец текста (!!)
        if (ch == '!') {
            if (prevWasExclamation) {
                // Второй '!' - конец ввода
                endOfText = true;
                if (wordLen > 0) {
                    currentWord[wordLen] = '\0';
                    words[*wordCount] = (char*)malloc(sizeof(char) * (wordLen + 1));
                    if (words[*wordCount]) {
                        strcpy(words[*wordCount], currentWord);
                    }
                    (*wordCount)++;
                }
                continue;
            }
            prevWasExclamation = true;
            
            if (wordLen > 0) {
                currentWord[wordLen] = '\0';
                if (*wordCount >= capacity) {
                    capacity *= 2;
                    words = (char**)realloc(words, sizeof(char*) * capacity);
                    if (!words) {
                        free(currentWord);
                        *wordCount = 0;
                        return nullptr;
                    }
                }
                words[*wordCount] = (char*)malloc(sizeof(char) * (wordLen + 1));
                if (words[*wordCount]) {
                    strcpy(words[*wordCount], currentWord);
                }
                (*wordCount)++;
                wordLen = 0;
            }
            continue;
        }
        
        prevWasExclamation = false;
        
        // Разделители
        if (ch == ' ' || ch == '\t' || ch == '\n') {
            if (wordLen > 0) {
                currentWord[wordLen] = '\0';
                if (*wordCount >= capacity) {
                    capacity *= 2;
                    words = (char**)realloc(words, sizeof(char*) * capacity);
                    if (!words) {
                        free(currentWord);
                        *wordCount = 0;
                        return nullptr;
                    }
                }
                words[*wordCount] = (char*)malloc(sizeof(char) * (wordLen + 1));
                if (words[*wordCount]) {
                    strcpy(words[*wordCount], currentWord);
                }
                (*wordCount)++;
                wordLen = 0;
            }
            continue;
        }
        
        // Обычный символ
        if (wordLen >= wordCap - 1) {
            wordCap *= 2;
            currentWord = (char*)realloc(currentWord, sizeof(char) * wordCap);
            if (!currentWord) {
                free(words);
                *wordCount = 0;
                return nullptr;
            }
        }
        currentWord[wordLen] = ch;
        wordLen++;
    }
    
    free(currentWord);
    return words;
}



void printWords(char** words, int wordCount) {
    for (int i = 0; i < wordCount; i++) {
        printf("%s", words[i]);
        if (i < wordCount - 1) {
            printf(" ");
        }
    }
    printf("\n");
}



char** reverseWords(char** words, int wordCount) {
    if (wordCount == 0) {
        return nullptr;
    }
    
    char** reversed = (char**)malloc(sizeof(char*) * wordCount);
    if (!reversed) {
        return nullptr;
    }
    
    for (int i = 0; i < wordCount; i++) {
        int len = strlen(words[i]);
        reversed[i] = (char*)malloc(sizeof(char) * (len + 1));
        if (!reversed[i]) {
            // Освобождаем уже выделенную память при ошибке
            for (int j = 0; j < i; j++) {
                free(reversed[j]);
            }
            free(reversed);
            return nullptr;
        }
        strcpy(reversed[i], words[wordCount - 1 - i]);
    }
    
    return reversed;
}



char** extractNumberFromFirstWord(char** words, int* ptr_word_count, int* number) {
    // Проверка на NULL и пустой массив
    if (!words || *ptr_word_count == 0 || !number) {
        if (number) *number = 0;
        return words;
    }
    
    // Проверка, что первый указатель не NULL
    if (!words[0]) {
        *number = 0;
        return words;
    }
    
    char* firstWord = words[0];
    bool isNumber = true;
    int i = 0;
    
    // Проверяем каждый символ
    while (firstWord[i] != '\0') {
        if (firstWord[i] < '0' || firstWord[i] > '9') {
            isNumber = false;
            break;
        }
        i++;
    }
    
    if (isNumber && i > 0) {
        // Преобразуем строку в число
        *number = atoi(firstWord);
        
        // Освобождаем память первого слова
        free(words[0]);
        words[0] = NULL;  // Обнуляем для безопасности
        
        if (*ptr_word_count == 1) {
            // Если было только одно слово, возвращаем NULL
            free(words);
            return nullptr;
        }
        
        // Создаем новый массив на одно слово меньше
        char** newWords = (char**)malloc(sizeof(char*) * (*ptr_word_count - 1));
        if (!newWords) {
            // Если не удалось выделить память, возвращаем старый массив
            // Но первое слово уже удалено, поэтому лучше вернуть NULL
            free(words);
            return nullptr;
        }
        
        // Копируем указатели на оставшиеся слова
        for (int j = 1; j < *ptr_word_count; j++) {
            newWords[j - 1] = words[j];
        }
        
        // Освобождаем старый массив
        free(words);

        *ptr_word_count -= 1;
        return newWords;
    } else {
        // Первое слово не число
        *number = -1;
        return words;
    }
}



void reverseWord(char* word) {
    if (!word) return;
    
    int len = strlen(word);
    if (len <= 1) return;
    
    char* start = word;
    char* end = word + len - 1;
    
    while (start < end) {
        // Меняем местами символы
        char temp = *start;
        *start = *end;
        *end = temp;
        
        start++;
        end--;
    }
}



void ReverseOneWord(char** user_words, int user_number) {
    if (user_number <= 0) {
        return;
    } else {
        reverseWord(*(user_words + user_number - 1));
    }
}