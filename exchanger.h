#ifndef EXTRA_UTILS_H_
#define EXTRA_UTILS_H_

#include <sys/types.h>
#include <unistd.h>

typedef struct user_data_record_s {
  const char* sender_name;                  // имя отправителя
  pid_t sender_process_id;                  // идентификатор процесса отправителя (pid)
  int database_file_descriptor;             // файловый дескриптор (база данных)
} user_data_record_t;

typedef struct message_record_s {
  char* target_recipient_name;              // имя получателя
  char* message_content;                    // текст сообщения от отправителя
  time_t message_timestamp;                 // время отправки сообщения
} message_record_t;

user_data_record_t FetchingUserData(char** command_arguments, int* operation_error_code);

int OpeningDatabaseFile(const char* database_file_path, int* operation_error_code);

int ReadingUserData(user_data_record_t* user_record);

int SavingUserData(user_data_record_t* user_record);

message_record_t ParsingMessageLine(char* text_line, ssize_t line_length);

char* CreatingMessageChunk(user_data_record_t* user_record, message_record_t* parsed_tokens, int* chunk_size_bytes);

void FreeingParsedTokens(message_record_t* parsed_tokens);

message_record_t ScanningMessageMetadata(char* text_line);

size_t GettingDatabaseFileSize(int database_file_descriptor);

char* LoadingFileIntoBuffer(int database_file_descriptor, size_t* output_file_size);

time_t ProcessingNewMessages(const char* file_buffer, time_t previous_read_timestamp, const char* active_user_name);

void CleaningExpiredMessages(int database_file_descriptor, const char* file_buffer, time_t maximum_retention_timestamp);

void ScanningMessageHistory(size_t total_file_size, char* file_buffer, user_data_record_t* user_record);

#endif