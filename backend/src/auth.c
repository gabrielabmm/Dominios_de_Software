#include "auth.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <openssl/evp.h>
#endif

#define HASH_ITERATIONS 600000
#define HASH_SALT_SIZE 16
#define HASH_TAG_SIZE 32

/* Estado do parser JSON: texto não precisa ser terminado por '\0'; length
 * limita a leitura, e position avança conforme os tokens são consumidos. */
typedef struct {
  const char *text;
  size_t length;
  size_t position;
} JsonParser;

/* Avança espaços permitidos pelo JSON sem ultrapassar o fim do buffer. */
static void skip_whitespace(JsonParser *parser) {
  while (parser->position < parser->length) {
    char value = parser->text[parser->position];
    if (value != ' ' && value != '\t' && value != '\r' && value != '\n') {
      break;
    }
    parser->position++;
  }
}

/* Consome um caractere estrutural esperado, ignorando espaços antes dele.
 * Retorna 0 sem avançar se o próximo token não corresponder. */
static int consume(JsonParser *parser, char expected) {
  skip_whitespace(parser);
  if (parser->position >= parser->length ||
      parser->text[parser->position] != expected) {
    return 0;
  }
  parser->position++;
  return 1;
}

/* Codifica um ponto Unicode como UTF-8 e verifica espaço para o terminador
 * nulo. Os valores inválidos (surrogates isolados, NUL e > U+10FFFF) falham. */
static int append_utf8(char *output, size_t capacity, size_t *written,
                       unsigned int codepoint) {
  unsigned char bytes[4];
  size_t count;

  if (codepoint == 0 || codepoint > 0x10ffff ||
      (codepoint >= 0xd800 && codepoint <= 0xdfff)) {
    return 0;
  }

  if (codepoint <= 0x7f) {
    bytes[0] = (unsigned char)codepoint;
    count = 1;
  } else if (codepoint <= 0x7ff) {
    bytes[0] = (unsigned char)(0xc0 | (codepoint >> 6));
    bytes[1] = (unsigned char)(0x80 | (codepoint & 0x3f));
    count = 2;
  } else if (codepoint <= 0xffff) {
    bytes[0] = (unsigned char)(0xe0 | (codepoint >> 12));
    bytes[1] = (unsigned char)(0x80 | ((codepoint >> 6) & 0x3f));
    bytes[2] = (unsigned char)(0x80 | (codepoint & 0x3f));
    count = 3;
  } else {
    bytes[0] = (unsigned char)(0xf0 | (codepoint >> 18));
    bytes[1] = (unsigned char)(0x80 | ((codepoint >> 12) & 0x3f));
    bytes[2] = (unsigned char)(0x80 | ((codepoint >> 6) & 0x3f));
    bytes[3] = (unsigned char)(0x80 | (codepoint & 0x3f));
    count = 4;
  }

  if (*written + count >= capacity) {
    return 0;
  }
  memcpy(output + *written, bytes, count);
  *written += count;
  return 1;
}

/* Converte um dígito hexadecimal em seu valor numérico; -1 indica erro. */
static int hex_value(char value) {
  if (value >= '0' && value <= '9') {
    return value - '0';
  }
  if (value >= 'a' && value <= 'f') {
    return value - 'a' + 10;
  }
  if (value >= 'A' && value <= 'F') {
    return value - 'A' + 10;
  }
  return -1;
}

/* Lê exatamente quatro dígitos hexadecimais do escape JSON \\uXXXX. */
static int parse_hex_quad(JsonParser *parser, unsigned int *value) {
  unsigned int result = 0;
  int index;

  if (parser->length - parser->position < 4) {
    return 0;
  }
  for (index = 0; index < 4; index++) {
    int digit = hex_value(parser->text[parser->position++]);
    if (digit < 0) {
      return 0;
    }
    result = (result << 4) | (unsigned int)digit;
  }
  *value = result;
  return 1;
}

/* Lê uma string JSON, tratando escapes padrão e pares surrogate e convertendo
 * escapes Unicode em UTF-8. Rejeita controles, escapes desconhecidos e strings
 * que não cabem no buffer de destino. */
static int parse_json_string(JsonParser *parser, char *output,
                             size_t capacity) {
  size_t written = 0;

  if (capacity == 0 || !consume(parser, '"')) {
    return 0;
  }
  output[0] = '\0';

  while (parser->position < parser->length) {
    unsigned char value = (unsigned char)parser->text[parser->position++];
    if (value == '"') {
      output[written] = '\0';
      return 1;
    }
    if (value < 0x20) {
      return 0;
    }
    if (value == '\\') {
      unsigned int codepoint;
      if (parser->position >= parser->length) {
        return 0;
      }
      value = (unsigned char)parser->text[parser->position++];
      switch (value) {
      case '"':
      case '\\':
      case '/':
        codepoint = value;
        break;
      case 'b':
        codepoint = '\b';
        break;
      case 'f':
        codepoint = '\f';
        break;
      case 'n':
        codepoint = '\n';
        break;
      case 'r':
        codepoint = '\r';
        break;
      case 't':
        codepoint = '\t';
        break;
      case 'u':
        if (!parse_hex_quad(parser, &codepoint)) {
          return 0;
        }
        if (codepoint >= 0xd800 && codepoint <= 0xdbff) {
          unsigned int low_surrogate;
          if (parser->length - parser->position < 6 ||
              parser->text[parser->position] != '\\' ||
              parser->text[parser->position + 1] != 'u') {
            return 0;
          }
          parser->position += 2;
          if (!parse_hex_quad(parser, &low_surrogate) ||
              low_surrogate < 0xdc00 || low_surrogate > 0xdfff) {
            return 0;
          }
          codepoint = 0x10000 + ((codepoint - 0xd800) << 10) +
                      (low_surrogate - 0xdc00);
        }
        break;
      default:
        return 0;
      }
      if (!append_utf8(output, capacity, &written, codepoint)) {
        return 0;
      }
      continue;
    }

    if (written + 1 >= capacity) {
      return 0;
    }
    output[written++] = (char)value;
  }

  return 0;
}

/* Usado para validar campos obrigatórios: espaços, tabs e quebras de linha
 * não contam como um identificador/hash preenchido. Não altera o conteúdo. */
static int is_blank(const char *value) {
  const unsigned char *cursor = (const unsigned char *)value;
  while (*cursor != '\0') {
    if (*cursor != ' ' && *cursor != '\t' && *cursor != '\r' &&
        *cursor != '\n') {
      return 0;
    }
    cursor++;
  }
  return 1;
}

/* Constrói um AuthUser a partir de um objeto JSON. Exige exatamente os três
 * campos previstos, sem repetições, e não aceita dados extras nem valores
 * vazios. A senha armazenada deve ser o hash, nunca a senha em texto puro. */
static int parse_user(JsonParser *parser, AuthUser *user) {
  int has_id = 0;
  int has_identifier = 0;
  int has_hash = 0;

  if (!consume(parser, '{')) {
    return 0;
  }
  skip_whitespace(parser);
  if (parser->position < parser->length &&
      parser->text[parser->position] == '}') {
    return 0;
  }

  for (;;) {
    char key[32];
    if (!parse_json_string(parser, key, sizeof(key)) ||
        !consume(parser, ':')) {
      return 0;
    }
    if (strcmp(key, "usuarioId") == 0 && !has_id) {
      has_id = parse_json_string(parser, user->usuario_id,
                                 sizeof(user->usuario_id));
      if (!has_id) {
        return 0;
      }
    } else if (strcmp(key, "identificador") == 0 && !has_identifier) {
      has_identifier = parse_json_string(
          parser, user->identificador, sizeof(user->identificador));
      if (!has_identifier) {
        return 0;
      }
    } else if (strcmp(key, "senhaHash") == 0 && !has_hash) {
      has_hash =
          parse_json_string(parser, user->senha_hash, sizeof(user->senha_hash));
      if (!has_hash) {
        return 0;
      }
    } else {
      return 0;
    }

    skip_whitespace(parser);
    if (consume(parser, '}')) {
      break;
    }
    if (!consume(parser, ',')) {
      return 0;
    }
  }

  return has_id && has_identifier && has_hash &&
         !is_blank(user->usuario_id) && !is_blank(user->identificador) &&
         !is_blank(user->senha_hash);
}

/* Valida o documento completo {"usuarios":[...]}, até 256 registros, sem
 * dados sobrando no final, IDs repetidos ou identificadores repetidos.
 * Rejeitar duplicidades torna a busca por identificador inequívoca. */
static int parse_user_database(const char *json, size_t length,
                               UserDatabase *database) {
  JsonParser parser = {json, length, 0};
  char key[32];
  size_t index;

  database->count = 0;
  if (!consume(&parser, '{') ||
      !parse_json_string(&parser, key, sizeof(key)) ||
      strcmp(key, "usuarios") != 0 || !consume(&parser, ':') ||
      !consume(&parser, '[')) {
    return 0;
  }

  skip_whitespace(&parser);
  if (parser.position < parser.length &&
      parser.text[parser.position] != ']') {
    for (;;) {
      if (database->count == AUTH_MAX_USERS ||
          !parse_user(&parser, &database->users[database->count])) {
        return 0;
      }
      database->count++;
      skip_whitespace(&parser);
      if (consume(&parser, ']')) {
        break;
      }
      if (!consume(&parser, ',')) {
        return 0;
      }
    }
  } else if (!consume(&parser, ']')) {
    return 0;
  }

  if (!consume(&parser, '}')) {
    return 0;
  }
  skip_whitespace(&parser);
  if (parser.position != parser.length) {
    return 0;
  }

  for (index = 0; index < database->count; index++) {
    size_t other;
    for (other = index + 1; other < database->count; other++) {
      if (strcmp(database->users[index].usuario_id,
                 database->users[other].usuario_id) == 0 ||
          strcmp(database->users[index].identificador,
                 database->users[other].identificador) == 0) {
        return 0;
      }
    }
  }
  return 1;
}

/* Carrega o arquivo em memória, com limite de 1 MiB, e valida seu JSON antes
 * de disponibilizar a base. Retorna 1 somente se leitura e parsing concluírem. */
int load_user_database(const char *file_path, UserDatabase *database) {
  FILE *file;
  long file_size;
  char *contents;
  size_t bytes_read;
  int valid;

  file = fopen(file_path, "rb");
  if (file == NULL) {
    return 0;
  }
  if (fseek(file, 0, SEEK_END) != 0 || (file_size = ftell(file)) < 0 ||
      file_size > 1024 * 1024 || fseek(file, 0, SEEK_SET) != 0) {
    fclose(file);
    return 0;
  }
  contents = (char *)malloc((size_t)file_size + 1);
  if (contents == NULL) {
    fclose(file);
    return 0;
  }
  bytes_read = fread(contents, 1, (size_t)file_size, file);
  fclose(file);
  contents[bytes_read] = '\0';
  valid = bytes_read == (size_t)file_size &&
          parse_user_database(contents, bytes_read, database);
  free(contents);
  return valid;
}

/* Variante usada quando os dados vêm diretamente da variável de ambiente
 * AUTH_USERS_JSON; preserva o mesmo limite e as mesmas regras do arquivo. */
int load_user_database_json(const char *json, size_t length,
                            UserDatabase *database) {
  if (length > 1024 * 1024) {
    return 0;
  }
  return parse_user_database(json, length, database);
}

/* Analisa o DTO recebido em POST /autenticacao. Exige somente duas strings
 * não vazias (identificador e senha), sem campos duplicados ou extras; não
 * normaliza a senha, pois espaços podem fazer parte da credencial. */
int parse_credentials(const char *json, size_t length, char *identifier,
                      size_t identifier_size, char *password,
                      size_t password_size) {
  JsonParser parser = {json, length, 0};
  char key[32];
  int has_identifier = 0;
  int has_password = 0;

  if (!consume(&parser, '{')) {
    return 0;
  }
  skip_whitespace(&parser);
  if (parser.position < parser.length && parser.text[parser.position] == '}') {
    return 0;
  }

  for (;;) {
    if (!parse_json_string(&parser, key, sizeof(key)) ||
        !consume(&parser, ':')) {
      return 0;
    }
    if (strcmp(key, "identificador") == 0 && !has_identifier) {
      has_identifier =
          parse_json_string(&parser, identifier, identifier_size);
      if (!has_identifier) {
        return 0;
      }
    } else if (strcmp(key, "senha") == 0 && !has_password) {
      has_password = parse_json_string(&parser, password, password_size);
      if (!has_password) {
        return 0;
      }
    } else {
      return 0;
    }

    skip_whitespace(&parser);
    if (consume(&parser, '}')) {
      break;
    }
    if (!consume(&parser, ',')) {
      return 0;
    }
  }

  skip_whitespace(&parser);
  return parser.position == parser.length && has_identifier &&
         has_password && !is_blank(identifier) && !is_blank(password);
}

/* Decodifica um caractere do alfabeto Base64 sem padding. */
static int base64_value(char value) {
  if (value >= 'A' && value <= 'Z') {
    return value - 'A';
  }
  if (value >= 'a' && value <= 'z') {
    return value - 'a' + 26;
  }
  if (value >= '0' && value <= '9') {
    return value - '0' + 52;
  }
  if (value == '+') {
    return 62;
  }
  if (value == '/') {
    return 63;
  }
  return -1;
}

/* Decodifica o Base64 sem padding usado no formato de hash e exige o tamanho
 * binário exato esperado para salt ou chave derivada. */
static int decode_base64(const char *input, size_t input_length,
                         unsigned char *output, size_t output_length) {
  size_t input_index;
  size_t output_index = 0;
  unsigned int accumulator = 0;
  unsigned int bits = 0;

  if (input_length != (output_length * 8 + 5) / 6) {
    return 0;
  }
  for (input_index = 0; input_index < input_length; input_index++) {
    int value = base64_value(input[input_index]);
    if (value < 0) {
      return 0;
    }
    accumulator = (accumulator << 6) | (unsigned int)value;
    bits += 6;
    if (bits >= 8) {
      bits -= 8;
      if (output_index >= output_length) {
        return 0;
      }
      output[output_index++] =
          (unsigned char)((accumulator >> bits) & 0xff);
    }
  }
  return output_index == output_length &&
         (bits == 0 || (accumulator & ((1u << bits) - 1u)) == 0);
}

/* Deriva uma chave PBKDF2-HMAC-SHA-256. No Windows usa CNG (bcrypt.dll);
 * nos demais sistemas usa OpenSSL. A implementação não armazena a senha. */
static int derive_key(const char *password, const unsigned char *salt,
                      unsigned char *output) {
#ifdef _WIN32
  typedef LONG NTSTATUS;
  typedef NTSTATUS(WINAPI *OpenAlgorithmProvider)(void **, const wchar_t *,
                                                  const wchar_t *, ULONG);
  typedef NTSTATUS(WINAPI *DeriveKeyPbkdf2)(void *, unsigned char *, ULONG,
                                            unsigned char *, ULONG, ULONGLONG,
                                            unsigned char *, ULONG, ULONG);
  typedef NTSTATUS(WINAPI *CloseAlgorithmProvider)(void *, ULONG);
  HMODULE library = LoadLibraryA("bcrypt.dll");
  OpenAlgorithmProvider open_algorithm;
  DeriveKeyPbkdf2 derive_key_pbkdf2;
  CloseAlgorithmProvider close_algorithm;
  void *algorithm = NULL;
  NTSTATUS status;
  size_t password_length = strlen(password);

  if (library == NULL) {
    return 0;
  }
  open_algorithm = (OpenAlgorithmProvider)GetProcAddress(
      library, "BCryptOpenAlgorithmProvider");
  derive_key_pbkdf2 = (DeriveKeyPbkdf2)GetProcAddress(
      library, "BCryptDeriveKeyPBKDF2");
  close_algorithm = (CloseAlgorithmProvider)GetProcAddress(
      library, "BCryptCloseAlgorithmProvider");
  if (open_algorithm == NULL || derive_key_pbkdf2 == NULL ||
      close_algorithm == NULL ||
      open_algorithm(&algorithm, L"SHA256", NULL, 0x00000008) < 0) {
    FreeLibrary(library);
    return 0;
  }

  status = derive_key_pbkdf2(
      algorithm, (unsigned char *)password, (ULONG)password_length,
      (unsigned char *)salt, HASH_SALT_SIZE, HASH_ITERATIONS, output,
      HASH_TAG_SIZE, 0);
  close_algorithm(algorithm, 0);
  FreeLibrary(library);
  return status >= 0;
#else
  return PKCS5_PBKDF2_HMAC(password, (int)strlen(password), salt,
                           HASH_SALT_SIZE, HASH_ITERATIONS, EVP_sha256(),
                           HASH_TAG_SIZE, output) == 1;
#endif
}

/* Compara todos os bytes sem sair no primeiro byte divergente, reduzindo
 * vazamento de informação pelo tempo de comparação. */
static int constant_time_equal(const unsigned char *left,
                               const unsigned char *right, size_t length) {
  /* Acumular todas as diferenças evita encerrar a comparação no primeiro byte. */
  volatile unsigned char difference = 0;
  size_t index;
  for (index = 0; index < length; index++) {
    difference |= (unsigned char)(left[index] ^ right[index]);
  }
  return difference == 0;
}

/* Valida o formato versionado do hash armazenado, decodifica salt e hash
 * esperado, deriva a chave para a senha digitada e compara os 32 bytes.
 * Retorna 1 para senha correta, 0 para divergência e -1 para erro interno ou
 * hash malformado; o controlador transforma o último caso em HTTP 500. */
static int verify_password(const char *password, const char *encoded_hash) {
  /* Custos variáveis vindos do JSON permitiriam exigir trabalho ilimitado. */
  static const char prefix[] = "$pbkdf2-sha256$600000$";
  const char *salt_text;
  const char *tag_text;
  const char *separator;
  unsigned char salt[HASH_SALT_SIZE];
  unsigned char expected_tag[HASH_TAG_SIZE];
  unsigned char actual_tag[HASH_TAG_SIZE];

  if (strncmp(encoded_hash, prefix, sizeof(prefix) - 1) != 0) {
    return -1;
  }
  salt_text = encoded_hash + sizeof(prefix) - 1;
  separator = strchr(salt_text, '$');
  if (separator == NULL || strchr(separator + 1, '$') != NULL) {
    return -1;
  }
  tag_text = separator + 1;
  if (!decode_base64(salt_text, (size_t)(separator - salt_text), salt,
                     sizeof(salt)) ||
      !decode_base64(tag_text, strlen(tag_text), expected_tag,
                     sizeof(expected_tag))) {
    return -1;
  }
  if (!derive_key(password, salt, actual_tag)) {
    return -1;
  }
  return constant_time_equal(actual_tag, expected_tag, sizeof(actual_tag));
}

/* Executa uma derivação com salt fixo quando o identificador não existe.
 * Isso aproxima o custo computacional dos dois caminhos e reduz diferenças
 * de tempo que poderiam facilitar a descoberta de contas cadastradas. */
static int perform_dummy_derivation(const char *password) {
  static const unsigned char dummy_salt[HASH_SALT_SIZE] = {
      0x61, 0x75, 0x74, 0x68, 0x2d, 0x64, 0x75, 0x6d,
      0x6d, 0x79, 0x2d, 0x73, 0x61, 0x6c, 0x74, 0x21};
  unsigned char ignored_tag[HASH_TAG_SIZE];
  /* Mantém trabalho criptográfico semelhante sem consultar uma conta fictícia. */
  return derive_key(password, dummy_salt, ignored_tag);
}

/* Regra central da aplicação: procura o identificador por igualdade exata,
 * verifica o hash apenas para o usuário encontrado e copia somente usuarioId
 * no sucesso. Conta inexistente e senha incorreta resultam no mesmo enum;
 * falhas de hash/crypto ou buffer insuficiente são erros internos. */
AuthResult authenticate(const UserDatabase *database, const char *identifier,
                        const char *password, char *user_id,
                        size_t user_id_size) {
  size_t index;
  for (index = 0; index < database->count; index++) {
    const AuthUser *user = &database->users[index];
    int password_matches;
    if (strcmp(user->identificador, identifier) != 0) {
      continue;
    }
    password_matches = verify_password(password, user->senha_hash);
    if (password_matches < 0) {
      return AUTH_INTERNAL_ERROR;
    }
    if (!password_matches) {
      return AUTH_INVALID_CREDENTIALS;
    }
    if (strlen(user->usuario_id) + 1 > user_id_size) {
      return AUTH_INTERNAL_ERROR;
    }
    strcpy(user_id, user->usuario_id);
    return AUTH_SUCCESS;
  }
  return perform_dummy_derivation(password) ? AUTH_INVALID_CREDENTIALS
                                            : AUTH_INTERNAL_ERROR;
}
