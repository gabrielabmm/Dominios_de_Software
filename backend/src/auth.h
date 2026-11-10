#ifndef AUTH_H
#define AUTH_H

#include <stddef.h>

#define AUTH_MAX_USERS 256
#define AUTH_USER_ID_SIZE 128
#define AUTH_IDENTIFIER_SIZE 256
#define AUTH_HASH_SIZE 256
#define AUTH_PASSWORD_SIZE 512

/* Registro carregado do JSON. senha_hash contém o hash PBKDF2 codificado,
 * não a senha original. Identificador é comparado exatamente. */
typedef struct {
  char usuario_id[AUTH_USER_ID_SIZE];
  char identificador[AUTH_IDENTIFIER_SIZE];
  char senha_hash[AUTH_HASH_SIZE];
} AuthUser;

/* Repositório somente de leitura residente em memória durante a vida do
 * servidor; count informa quantas posições de users estão preenchidas. */
typedef struct {
  AuthUser users[AUTH_MAX_USERS];
  size_t count;
} UserDatabase;

/* Resultado da regra de autenticação, mapeado pelo HTTP para 200, 401 ou 500. */
typedef enum {
  AUTH_SUCCESS,
  AUTH_INVALID_CREDENTIALS,
  AUTH_INTERNAL_ERROR
} AuthResult;

/* Carrega e valida o arquivo JSON de usuários (limite de 1 MiB). */
int load_user_database(const char *file_path, UserDatabase *database);

/* Alternativa ao arquivo: carrega e valida JSON já presente em memória. */
int load_user_database_json(const char *json, size_t length,
                            UserDatabase *database);

/* Valida o corpo JSON de entrada e copia identificador/senha para os buffers
 * fornecidos. Campos extras, duplicados, ausentes ou vazios são rejeitados. */
int parse_credentials(const char *json, size_t length,
                      char *identifier, size_t identifier_size,
                      char *password, size_t password_size);

/* Aplica a regra de autenticação. Em sucesso grava usuarioId no buffer de
 * saída; os argumentos e o banco são emprestados, não modificados. */
AuthResult authenticate(const UserDatabase *database, const char *identifier,
                        const char *password, char *user_id,
                        size_t user_id_size);

#endif
