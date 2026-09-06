#include "change_place.h"



int main() {
    int count_words = 0;
    char** user_words = readWords(&count_words);

    int user_number = - 1;
    user_words = extractNumberFromFirstWord(user_words, &count_words, &user_number);

    ReverseOneWord(user_words, user_number);
    
    user_words = reverseWords(user_words, count_words);

    printWords(user_words, count_words);
}