#include "exchanger.h"

#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <assert.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

//================================================================//
//                    BASIC STRUCTURE OF DATA:                    //
//     [time][recipient_name][sender_name][sender_id]<message>    //
//================================================================//

const int file_access_mode = 0644; // права доступа для функции open

int main(int argument_count, char** argument_vector) {
  
  if (argument_count != 3) {
    printf("argc: Incorrect number of arguments\n");
    return 0;
  }
  
  int operation_error_code = 0;
 
  if (operation_error_code != 0) {
    printf("GetUserData: Error!\n");
    return 0;
  }

  pid_t child_process_id = fork();
  
  if (child_process_id == -1) {
    printf("fork(): Error!\n");
  } else if (child_process_id == 0) {
    user_data_record_t user_data_record = FetchingUserData(argument_vector, &operation_error_code);
    operation_error_code = ReadingUserData(&user_data_record);
  } else {
    user_data_record_t user_data_record = FetchingUserData(argument_vector, &operation_error_code);
    operation_error_code = SavingUserData(&user_data_record);
  }
    
  return 0;
}

user_data_record_t FetchingUserData(char** argument_vector, int* operation_error_code) {
  assert(argument_vector);
  assert(operation_error_code);

  user_data_record_t user_data_record = {0};

  user_data_record.sender_name = argument_vector[1];
  user_data_record.sender_process_id = getpid();
  user_data_record.database_file_descriptor = OpeningDatabaseFile(argument_vector[2], operation_error_code);

  return user_data_record;
}

int OpeningDatabaseFile(const char* database_file_path, int* operation_error_code) {
  assert(database_file_path);
  assert(operation_error_code);
  
  int database_file_descriptor = open(database_file_path, O_RDWR | O_CREAT | O_APPEND, file_access_mode);

  if (database_file_descriptor == -1) {
    printf("open(): Error!\n");
    *operation_error_code = -1;
  }

  return database_file_descriptor;
}

int ReadingUserData(user_data_record_t* user_data_record) {
  assert(user_data_record);
    
  int sleep_duration_seconds = 3;
  time_t previous_read_timestamp = 0;

  while (1) {
    size_t total_file_size = 0;
    
    char* file_buffer = LoadingFileIntoBuffer(user_data_record->database_file_descriptor, &total_file_size);
    
    if (!file_buffer) {
      sleep(sleep_duration_seconds);
      continue; 
    }

    // поиск последнего сообщения
    time_t latest_message_timestamp = ProcessingNewMessages(file_buffer, previous_read_timestamp, user_data_record->sender_name);

    // очистка старых сообщений
    CleaningExpiredMessages(user_data_record->database_file_descriptor, file_buffer, previous_read_timestamp);

    // обновление времени последнего прочитанного сообщения
    previous_read_timestamp = latest_message_timestamp;

    free(file_buffer);
    sleep(sleep_duration_seconds);
  }

  return 0;
}

char* LoadingFileIntoBuffer(int database_file_descriptor, size_t* output_file_size) {
  assert(output_file_size);

  lseek(database_file_descriptor, 0, SEEK_SET);
  size_t total_file_size = GettingDatabaseFileSize(database_file_descriptor);
  *output_file_size = total_file_size;

  if (total_file_size == 0) {
    return NULL;
  }

  char* file_buffer = (char*)calloc(total_file_size + 1, sizeof(char));
  if (!file_buffer) {
    return NULL;
  }

  ssize_t bytes_read = read(database_file_descriptor, file_buffer, total_file_size);
  if (bytes_read <= 0) {
    free(file_buffer);
    return NULL;
  }

  file_buffer[bytes_read] = '\0';
  return file_buffer;
}

time_t ProcessingNewMessages(const char* file_buffer, time_t previous_read_timestamp, const char* active_user_name) {
  assert(file_buffer);
  assert(active_user_name);

  const char* current_chunk_pointer = file_buffer;
  time_t latest_message_timestamp = previous_read_timestamp;

  while (current_chunk_pointer && *current_chunk_pointer != '\0') {
    if (strchr(current_chunk_pointer, '[') && strchr(current_chunk_pointer, '<')) {

      message_record_t parsed_tokens = ScanningMessageMetadata((char*)current_chunk_pointer);

      if (parsed_tokens.message_timestamp > previous_read_timestamp) {
        if (parsed_tokens.target_recipient_name && strcmp(active_user_name, parsed_tokens.target_recipient_name) == 0) {
          printf("%s\n", parsed_tokens.message_content);
          fflush(stdout);
        }
        
        if (parsed_tokens.message_timestamp > latest_message_timestamp) {
          latest_message_timestamp = parsed_tokens.message_timestamp;
        }
      }
      FreeingParsedTokens(&parsed_tokens);
    }
    current_chunk_pointer = strchr(current_chunk_pointer, '\n');
    if (current_chunk_pointer) ++current_chunk_pointer;
  }

  return latest_message_timestamp;
}

void CleaningExpiredMessages(int database_file_descriptor, const char* file_buffer, time_t previous_read_timestamp) {
  assert(file_buffer);

  lseek(database_file_descriptor, 0, SEEK_SET);
  
  const char* current_chunk_pointer = file_buffer;
  off_t updated_file_size = 0;

  while (current_chunk_pointer && *current_chunk_pointer != '\0') {
    const char* next_chunk_pointer = strchr(current_chunk_pointer, '\n');
    size_t current_line_length = next_chunk_pointer ? (size_t)(next_chunk_pointer - current_chunk_pointer + 1) : strlen(current_chunk_pointer);

    if (strchr(current_chunk_pointer, '[') && strchr(current_chunk_pointer, '<')) {
      message_record_t parsed_tokens = ScanningMessageMetadata((char*)current_chunk_pointer);

      if (parsed_tokens.message_timestamp > previous_read_timestamp) {
        write(database_file_descriptor, current_chunk_pointer, current_line_length);
        updated_file_size += current_line_length;
      }
      FreeingParsedTokens(&parsed_tokens);
    }
    
    current_chunk_pointer = next_chunk_pointer ? next_chunk_pointer + 1 : NULL;
  }

  // Обрезаем файл под новый размер оставшихся сообщений
  ftruncate(database_file_descriptor, updated_file_size);
}

size_t GettingDatabaseFileSize(int database_file_descriptor) {
  struct stat file_status_info = {0};

  if (fstat(database_file_descriptor, &file_status_info) == -1) {
    printf("fstat(): error!\n");
    return 0;
  }

  return file_status_info.st_size;
}

void ScanningMessageHistory(size_t total_file_size, char* file_buffer, user_data_record_t* user_data_record) {
  assert(file_buffer);
  assert(user_data_record);
  
  const char* active_user_name = user_data_record->sender_name;

  char* current_chunk_pointer = file_buffer;
   
  while (current_chunk_pointer && strchr(current_chunk_pointer, '[')) {
    message_record_t parsed_tokens = ScanningMessageMetadata(current_chunk_pointer);
    
    if (strlen(active_user_name) == strlen(parsed_tokens.target_recipient_name) && 
        strcmp(active_user_name, parsed_tokens.target_recipient_name) == 0) {
      printf("%s\n", parsed_tokens.message_content);
      fflush(stdout);
    }

    current_chunk_pointer = strchr(current_chunk_pointer, '\n');
    if (current_chunk_pointer) ++current_chunk_pointer;

    FreeingParsedTokens(&parsed_tokens);
  }
}

message_record_t ScanningMessageMetadata(char* text_line) {
  assert(text_line);

  char* opening_bracket_pointer = strchr(text_line, '[');
  char* closing_bracket_pointer = strchr(opening_bracket_pointer, ']');
  time_t parsed_timestamp = 0;

  ++opening_bracket_pointer;
  while (opening_bracket_pointer < closing_bracket_pointer) {
    parsed_timestamp = parsed_timestamp * 10 + (*opening_bracket_pointer - '0');
    ++opening_bracket_pointer;
  }
  
  opening_bracket_pointer = strchr(closing_bracket_pointer, '[');
  ++opening_bracket_pointer;
  closing_bracket_pointer = strchr(opening_bracket_pointer, ']');
  char* extracted_recipient_name = strndup(opening_bracket_pointer, closing_bracket_pointer - opening_bracket_pointer);

  opening_bracket_pointer = strchr(text_line, '<');
  ++opening_bracket_pointer;
  closing_bracket_pointer = strchr(opening_bracket_pointer, '>');
  char* extracted_message_content = strndup(opening_bracket_pointer, closing_bracket_pointer - opening_bracket_pointer);
  
  message_record_t parsed_record = {extracted_recipient_name, extracted_message_content, parsed_timestamp};
  return parsed_record;
}

int SavingUserData(user_data_record_t* user_data_record) {
  assert(user_data_record);
  
  char* input_line_buffer = NULL;
  size_t buffer_capacity = 0;
  ssize_t bytes_read_from_stdin = 0;
  int chunk_size_bytes = 0;

  while ((bytes_read_from_stdin = getline(&input_line_buffer, &buffer_capacity, stdin)) > 0) {
    if (input_line_buffer[bytes_read_from_stdin - 1] == '\n') {
      input_line_buffer[bytes_read_from_stdin - 1] = '\0';
    }

    message_record_t parsed_tokens = ParsingMessageLine(input_line_buffer, bytes_read_from_stdin);
  
    char* formatted_message_chunk = CreatingMessageChunk(user_data_record, &parsed_tokens, &chunk_size_bytes);

    write(user_data_record->database_file_descriptor, formatted_message_chunk, chunk_size_bytes);
    //printf("success write chunk: %s\n", formatted_message_chunk);
    
    if (formatted_message_chunk) free(formatted_message_chunk);
    FreeingParsedTokens(&parsed_tokens);
  }

  free(input_line_buffer);
  return 0;
}

message_record_t ParsingMessageLine(char* input_text_line, ssize_t line_length) {
  assert(input_text_line);

  message_record_t parsed_record = {0};
  
  char* colon_pointer = strchr(input_text_line, ':');
  char* whitespace_pointer = strchr(input_text_line, ' '); 

  parsed_record.message_timestamp = time(NULL);
  parsed_record.target_recipient_name = strndup(input_text_line, colon_pointer - input_text_line);
  parsed_record.message_content = strdup(whitespace_pointer + 1);
  
  return parsed_record;
}

char* CreatingMessageChunk(user_data_record_t* user_data_record, message_record_t* parsed_tokens, int* chunk_size_bytes) {
  assert(user_data_record);
  assert(parsed_tokens);
  assert(chunk_size_bytes);
  
  char* formatted_message_chunk = NULL;
  *chunk_size_bytes = 0;
  
  size_t probe_length = 0;
  int required_buffer_length = snprintf(formatted_message_chunk, probe_length, "[%ld][%s][%s][%d]<%s>\n",
                         parsed_tokens->message_timestamp, parsed_tokens->target_recipient_name,
                         user_data_record->sender_name, user_data_record->sender_process_id, parsed_tokens->message_content);

  formatted_message_chunk = (char*)calloc(required_buffer_length + 1, sizeof(char));
  if (!formatted_message_chunk) {
    return NULL;
  }
  
  // snprintf записывает (capacity - 1) символов + '\0'
  *chunk_size_bytes = snprintf(formatted_message_chunk, required_buffer_length + 1, "[%ld][%s][%s][%d]<%s>\n",
                    parsed_tokens->message_timestamp, parsed_tokens->target_recipient_name,
                    user_data_record->sender_name, user_data_record->sender_process_id, parsed_tokens->message_content);

  if (*chunk_size_bytes != required_buffer_length) {
    free(formatted_message_chunk);
    return NULL;
  }

  return formatted_message_chunk;
}

void FreeingParsedTokens(message_record_t* parsed_tokens) {
  assert(parsed_tokens);

  free(parsed_tokens->target_recipient_name);
  free(parsed_tokens->message_content);
}