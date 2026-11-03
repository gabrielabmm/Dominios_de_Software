#ifndef AUTH_H
#define AUTH_H

#include <stddef.h>

#define AUTH_MAX_USERS 256
#define AUTH_USER_ID_SIZE 128
#define AUTH_IDENTIFIER_SIZE 256
#define AUTH_HASH_SIZE 256
#define AUTH_PASSWORD_SIZE 512

typedef struct {
  char usuario_id[AUTH_USER_ID_SIZE];
  char identificador[AUTH_IDENTIFIER_SIZE];
  char senha_hash[AUTH_HASH_SIZE];
} AuthUser;

typedef struct {
  AuthUser users[AUTH_MAX_USERS];
  size_t count;
} UserDatabase;

typedef enum {
  AUTH_SUCCESS,
  AUTH_INVALID_CREDENTIALS,
  AUTH_INTERNAL_ERROR
} AuthResult;

int load_user_database(const char *file_path, UserDatabase *database);
int load_user_database_json(const char *json, size_t length,
                            UserDatabase *database);
int parse_credentials(const char *json, size_t length,
                      char *identifier, size_t identifier_size,
                      char *password, size_t password_size);
AuthResult authenticate(const UserDatabase *database, const char *identifier,
                        const char *password, char *user_id,
                        size_t user_id_size);

#endif
