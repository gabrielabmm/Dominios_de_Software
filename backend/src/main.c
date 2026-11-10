#include "auth.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET Socket;
#define CLOSE_SOCKET closesocket
#define SOCKET_ERROR_CODE SOCKET_ERROR
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int Socket;
#define CLOSE_SOCKET close
#define SOCKET_ERROR_CODE (-1)
#endif

#define MAX_HEADER_SIZE 8192
#define MAX_BODY_SIZE (16 * 1024)
#define MAX_REQUEST_SIZE (MAX_HEADER_SIZE + MAX_BODY_SIZE + 1)
#define MAX_PATH_SIZE 512

/* DTO interno: reúne os campos HTTP já extraídos e validados pelo servidor. */
typedef struct {
  char method[16];
  char path[MAX_PATH_SIZE];
  char content_type[128];
  size_t content_length;
  size_t header_length;
  int has_content_length;
  int has_content_type;
  int invalid_headers;
} HttpRequest;

/* send() pode transmitir menos bytes do que o solicitado; este laço completa
 * a resposta ou indica falha de transporte ao chamador. */
static int send_all(Socket client, const char *data, size_t length) {
  size_t sent = 0;
  while (sent < length) {
    int flags = 0;
#ifdef MSG_NOSIGNAL
    flags = MSG_NOSIGNAL;
#endif
    int result = send(client, data + sent, (int)(length - sent), flags);
    if (result <= 0) {
      return 0;
    }
    sent += (size_t)result;
  }
  return 1;
}

/* Monta uma resposta HTTP/1.1 com tamanho explícito e cabeçalhos comuns.
 * Connection: close simplifica o servidor: uma conexão atende uma requisição. */
static void send_response(Socket client, int status, const char *reason,
                          const char *content_type, const char *body,
                          size_t body_length, const char *additional_headers) {
  char headers[512];
  int header_length = snprintf(
      headers, sizeof(headers),
      "HTTP/1.1 %d %s\r\n"
      "Content-Type: %s\r\n"
      "Content-Length: %lu\r\n"
      "X-Content-Type-Options: nosniff\r\n"
      "%s"
      "Cache-Control: no-store\r\n"
      "Connection: close\r\n\r\n",
      status, reason, content_type, (unsigned long)body_length,
      additional_headers);

  if (header_length > 0 && (size_t)header_length < sizeof(headers)) {
    send_all(client, headers, (size_t)header_length);
    send_all(client, body, body_length);
  }
}

/* Formato comum das respostas JSON de erro, com status e código estáveis. */
static void send_json(Socket client, int status, const char *code,
                      const char *message) {
  char body[512];
  int length = snprintf(body, sizeof(body),
                        "{\"codigo\":\"%s\",\"mensagem\":\"%s\"}",
                        code, message);
  if (length > 0 && (size_t)length < sizeof(body)) {
    send_response(client, status,
                  status == 400 ? "Bad Request"
                  : status == 401 ? "Unauthorized"
                  : status == 404 ? "Not Found"
                  : status == 405 ? "Method Not Allowed"
                  : status == 413 ? "Payload Too Large"
                  : status == 415 ? "Unsupported Media Type"
                                  : "Internal Server Error",
                  "application/json; charset=utf-8", body, (size_t)length,
                  status == 405 ? "Allow: POST\r\n" : "");
  }
}

/* Resposta padrão para método incompatível; Allow informa o método aceito. */
static void send_method_not_allowed(Socket client, const char *allowed_method) {
  char headers[64];
  int length = snprintf(headers, sizeof(headers), "Allow: %s\r\n",
                        allowed_method);
  if (length > 0 && (size_t)length < sizeof(headers)) {
    const char body[] =
        "{\"codigo\":\"METODO_NAO_PERMITIDO\","
        "\"mensagem\":\"Método não permitido.\"}";
    send_response(client, 405, "Method Not Allowed",
                  "application/json; charset=utf-8", body, sizeof(body) - 1,
                  headers);
  }
}

/* Nomes de cabeçalhos HTTP não diferenciam maiúsculas de minúsculas. */
static int equals_ignore_case(const char *left, const char *right) {
  while (*left != '\0' && *right != '\0') {
    if (tolower((unsigned char)*left) != tolower((unsigned char)*right)) {
      return 0;
    }
    left++;
    right++;
  }
  return *left == '\0' && *right == '\0';
}

/* Interpreta a linha inicial e os cabeçalhos que afetam o corpo. Rejeita
 * Content-Length repetido/malformado e Transfer-Encoding, evitando aceitar
 * enquadramentos de corpo ambíguos neste servidor HTTP mínimo. */
static int parse_request_headers(char *buffer, size_t header_length,
                                 HttpRequest *request) {
  char *line_end;
  char *line;
  char *first_space;
  char *second_space;

  memset(request, 0, sizeof(*request));
  request->header_length = header_length;
  line_end = strstr(buffer, "\r\n");
  if (line_end == NULL || (size_t)(line_end - buffer) >= 1024) {
    return 0;
  }
  *line_end = '\0';
  first_space = strchr(buffer, ' ');
  second_space = first_space == NULL ? NULL : strchr(first_space + 1, ' ');
  if (first_space == NULL || second_space == NULL ||
      first_space == buffer || second_space == first_space + 1) {
    return 0;
  }
  *first_space = '\0';
  *second_space = '\0';
  if (strlen(buffer) >= sizeof(request->method) ||
      strlen(first_space + 1) >= sizeof(request->path)) {
    return 0;
  }
  strcpy(request->method, buffer);
  strcpy(request->path, first_space + 1);
  line = line_end + 2;

  while (line < buffer + header_length - 2) {
    char *colon;
    char *value;
    size_t name_length;
    line_end = strstr(line, "\r\n");
    if (line_end == NULL || line_end == line) {
      break;
    }
    *line_end = '\0';
    colon = strchr(line, ':');
    if (colon == NULL) {
      request->invalid_headers = 1;
      return 1;
    }
    name_length = (size_t)(colon - line);
    value = colon + 1;
    *colon = '\0';
    while (*value == ' ' || *value == '\t') {
      value++;
    }
    {
      char *value_end = value + strlen(value);
      while (value_end > value &&
             (value_end[-1] == ' ' || value_end[-1] == '\t')) {
        *--value_end = '\0';
      }
    }

    if (name_length == 14 &&
        equals_ignore_case(line, "Content-Length")) {
      char *end;
      unsigned long parsed;
      const char *digit;
      if (request->has_content_length || *value == '\0' || *value == '-') {
        request->invalid_headers = 1;
        return 1;
      }
      for (digit = value; *digit != '\0'; digit++) {
        if (*digit < '0' || *digit > '9') {
          request->invalid_headers = 1;
          return 1;
        }
      }
      errno = 0;
      parsed = strtoul(value, &end, 10);
      if (errno != 0 || *end != '\0') {
        request->invalid_headers = 1;
        return 1;
      }
      request->content_length = (size_t)parsed;
      request->has_content_length = 1;
    } else if (name_length == 12 &&
               equals_ignore_case(line, "Content-Type")) {
      if (request->has_content_type ||
          strlen(value) >= sizeof(request->content_type)) {
        request->invalid_headers = 1;
        return 1;
      }
      strcpy(request->content_type, value);
      request->has_content_type = 1;
    } else if (name_length == 17 &&
               equals_ignore_case(line, "Transfer-Encoding")) {
      request->invalid_headers = 1;
      return 1;
    }
    line = line_end + 2;
  }
  return 1;
}

/* Descarta, em blocos pequenos, o restante do corpo que excedeu o limite,
 * permitindo que a resposta 413 seja enviada antes de fechar a conexão. */
static void drain_request_body(Socket client, size_t already_read,
                               size_t expected_length) {
  char discard[4096];
  size_t remaining =
      expected_length > already_read ? expected_length - already_read : 0;

  /* Drenar o corpo excedente permite devolver 413 antes de fechar a conexão. */
  while (remaining > 0) {
    int received = recv(
        client, discard,
        (int)(remaining < sizeof(discard) ? remaining : sizeof(discard)), 0);
    if (received <= 0) {
      return;
    }
    remaining -= (size_t)received;
  }
}

/* Limita a espera por bytes do cliente para que uma conexão lenta não prenda
 * indefinidamente o único processo que atende requisições. */
static void set_receive_timeout(Socket client) {
#ifdef _WIN32
  DWORD timeout = 3000;
  setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout,
             (int)sizeof(timeout));
#else
  struct timeval timeout;
  timeout.tv_sec = 3;
  timeout.tv_usec = 0;
  setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
#endif
}

/* Lê primeiro os cabeçalhos e depois exatamente o Content-Length informado.
 * Retorna 200 para requisição recebida, 400 para entrada incompleta/malformada
 * ou 413 quando os limites de cabeçalho/corpo são excedidos. */
static int receive_request(Socket client, char *buffer, size_t capacity,
                           size_t *request_length, HttpRequest *request) {
  size_t used = 0;
  char *header_end;
  size_t header_length;
  size_t expected_length;

  for (;;) {
    int received;
    if (used >= capacity - 1) {
      return 413;
    }
    received = recv(client, buffer + used, (int)(capacity - used - 1), 0);
    if (received <= 0) {
      return 400;
    }
    used += (size_t)received;
    buffer[used] = '\0';
    header_end = strstr(buffer, "\r\n\r\n");
    if (header_end != NULL) {
      break;
    }
    if (used > MAX_HEADER_SIZE) {
      return 413;
    }
  }

  header_length = (size_t)(header_end - buffer) + 4;
  if (header_length > MAX_HEADER_SIZE) {
    return 413;
  }
  if (!parse_request_headers(buffer, header_length, request) ||
      request->invalid_headers) {
    return 400;
  }
  if (request->content_length > MAX_BODY_SIZE) {
    drain_request_body(client, used - header_length,
                       request->content_length);
    return 413;
  }
  expected_length = header_length + request->content_length;
  while (used < expected_length) {
    int received;
    if (used >= capacity - 1) {
      return 413;
    }
    received = recv(client, buffer + used, (int)(capacity - used - 1), 0);
    if (received <= 0) {
      return 400;
    }
    used += (size_t)received;
    buffer[used] = '\0';
  }
  *request_length = used;
  return 200;
}

/* Escapa aspas, barras invertidas e controles antes de inserir texto do
 * identificador do usuário numa resposta JSON criada por snprintf. */
static void escape_json_string(const char *input, char *output,
                               size_t output_size) {
  size_t written = 0;
  const unsigned char *cursor = (const unsigned char *)input;

  while (*cursor != '\0' && written + 7 < output_size) {
    if (*cursor == '"' || *cursor == '\\') {
      output[written++] = '\\';
      output[written++] = (char)*cursor++;
    } else if (*cursor < 0x20) {
      int count = snprintf(output + written, output_size - written,
                           "\\u%04x", (unsigned int)*cursor++);
      if (count < 0 || (size_t)count >= output_size - written) {
        break;
      }
      written += (size_t)count;
    } else {
      output[written++] = (char)*cursor++;
    }
  }
  output[written] = '\0';
}

/* Aceita application/json, com parâmetros opcionais como charset=utf-8,
 * mas rejeita tipos incompatíveis. */
static int content_type_is_json(const char *content_type) {
  static const char expected[] = "application/json";
  size_t index;
  for (index = 0; index < sizeof(expected) - 1; index++) {
    if (tolower((unsigned char)content_type[index]) != expected[index]) {
      return 0;
    }
  }
  return content_type[sizeof(expected) - 1] == '\0' ||
         content_type[sizeof(expected) - 1] == ';';
}

/* Em log interno registra apenas um evento genérico: não inclui credenciais,
 * hashes, nem detalhes de implementação que poderiam revelar dados sensíveis. */
static void log_internal_error(void) {
  fprintf(stderr,
          "{\"evento\":\"falha_interna_de_autenticacao\","
          "\"tipoErro\":\"ErroInterno\"}\n");
  fflush(stderr);
}

/* Serve somente nomes de arquivo escolhidos pelo roteador (não o caminho
 * fornecido pelo cliente), limitando o recurso estático a 1 MiB. */
static void serve_static_file(Socket client, const char *web_root,
                              const char *file_name,
                              const char *content_type) {
  char file_path[1024];
#ifdef _WIN32
  const char path_separator = '\\';
#else
  const char path_separator = '/';
#endif
  FILE *file;
  long file_size;
  char *contents;
  size_t bytes_read;
  int path_length = snprintf(file_path, sizeof(file_path), "%s%c%s", web_root,
                             path_separator, file_name);

  if (path_length < 0 || (size_t)path_length >= sizeof(file_path)) {
    log_internal_error();
    send_json(client, 500, "ERRO_INTERNO",
              "Não foi possível concluir a requisição.");
    return;
  }
  file = fopen(file_path, "rb");
  if (file == NULL) {
    send_json(client, 404, "RECURSO_NAO_ENCONTRADO", "Recurso não encontrado.");
    return;
  }
  if (fseek(file, 0, SEEK_END) != 0 || (file_size = ftell(file)) < 0 ||
      file_size > 1024 * 1024 || fseek(file, 0, SEEK_SET) != 0) {
    fclose(file);
    log_internal_error();
    send_json(client, 500, "ERRO_INTERNO",
              "Não foi possível concluir a requisição.");
    return;
  }
  contents = (char *)malloc((size_t)file_size + 1);
  if (contents == NULL) {
    fclose(file);
    log_internal_error();
    send_json(client, 500, "ERRO_INTERNO",
              "Não foi possível concluir a requisição.");
    return;
  }
  bytes_read = fread(contents, 1, (size_t)file_size, file);
  fclose(file);
  if (bytes_read != (size_t)file_size) {
    free(contents);
    log_internal_error();
    send_json(client, 500, "ERRO_INTERNO",
              "Não foi possível concluir a requisição.");
    return;
  }
  send_response(client, 200, "OK", content_type, contents, bytes_read, "");
  free(contents);
}

/* Roteador/controller HTTP do protótipo:
 * - GET /, /index.html, /app.js e /style.css servem a interface;
 * - POST /autenticacao valida o contrato e delega a regra a authenticate();
 * - demais métodos/caminhos retornam 405/404 sem chamar a autenticação. */
static void handle_request(Socket client, const char *request_buffer,
                           size_t request_length,
                           const HttpRequest *request,
                           const UserDatabase *database,
                           const char *web_root) {
  const char *body = request_buffer + request->header_length;

  if (strcmp(request->path, "/") == 0 ||
      strcmp(request->path, "/index.html") == 0) {
    if (strcmp(request->method, "GET") != 0) {
      send_method_not_allowed(client, "GET");
      return;
    }
    serve_static_file(client, web_root, "index.html",
                      "text/html; charset=utf-8");
    return;
  }
  if (strcmp(request->path, "/app.js") == 0 ||
      strcmp(request->path, "/style.css") == 0) {
    if (strcmp(request->method, "GET") != 0) {
      send_method_not_allowed(client, "GET");
      return;
    }
    if (strcmp(request->path, "/app.js") == 0) {
      serve_static_file(client, web_root, "app.js",
                        "text/javascript; charset=utf-8");
    } else {
      serve_static_file(client, web_root, "style.css", "text/css; charset=utf-8");
    }
    return;
  }
  if (strcmp(request->path, "/autenticacao") != 0) {
    send_json(client, 404, "RECURSO_NAO_ENCONTRADO",
              "Recurso não encontrado.");
    return;
  }
  if (strcmp(request->method, "POST") != 0) {
    send_method_not_allowed(client, "POST");
    return;
  }
  if (!content_type_is_json(request->content_type)) {
    send_json(client, 415, "TIPO_DE_CONTEUDO_INVALIDO",
              "O corpo deve usar application/json.");
    return;
  }
  if (request->content_length == 0 ||
      request_length < request->header_length + request->content_length) {
    send_json(client, 400, "REQUISICAO_INVALIDA",
              "A requisição contém dados ausentes ou inválidos.");
    return;
  }

  {
    /* DTO de entrada, buffer de saída e buffers de serialização têm tamanhos
     * fixos definidos no contrato de auth.h; não guardam estado entre chamadas. */
    char identifier[AUTH_IDENTIFIER_SIZE];
    char password[AUTH_PASSWORD_SIZE];
    char user_id[AUTH_USER_ID_SIZE];
    char escaped_user_id[AUTH_USER_ID_SIZE * 6];
    char response_body[AUTH_USER_ID_SIZE * 6 + 32];
    int response_length;
    AuthResult result;

    if (!parse_credentials(body, request->content_length, identifier,
                           sizeof(identifier), password, sizeof(password))) {
      send_json(client, 400, "REQUISICAO_INVALIDA",
                "A requisição contém dados ausentes ou inválidos.");
      return;
    }
    /* A busca e a decisão de autenticação vivem no módulo auth, não no HTTP. */
    result = authenticate(database, identifier, password, user_id,
                          sizeof(user_id));
    if (result == AUTH_INVALID_CREDENTIALS) {
      send_json(client, 401, "AUTENTICACAO_INVALIDA",
                "Identificador ou senha inválidos.");
      return;
    }
    if (result == AUTH_INTERNAL_ERROR) {
      log_internal_error();
      send_json(client, 500, "ERRO_INTERNO",
                "Não foi possível concluir a autenticação.");
      return;
    }

    /* O ID pode conter caracteres especiais; escapamos antes da serialização.
     * A resposta de sucesso expõe somente a identidade, nunca senha ou hash. */
    escape_json_string(user_id, escaped_user_id, sizeof(escaped_user_id));
    response_length =
        snprintf(response_body, sizeof(response_body),
                 "{\"usuarioId\":\"%s\"}", escaped_user_id);
    if (response_length < 0 ||
        (size_t)response_length >= sizeof(response_body)) {
      log_internal_error();
      send_json(client, 500, "ERRO_INTERNO",
                "Não foi possível concluir a autenticação.");
      return;
    }
    send_response(client, 200, "OK", "application/json; charset=utf-8",
                  response_body, (size_t)response_length, "");
  }
}

/* Converte PORT em inteiro válido para TCP (1..65535); evita truncamentos e
 * valores que não poderiam ser usados como porta. */
static int parse_port(const char *value, unsigned short *port) {
  char *end;
  unsigned long parsed;
  if (value == NULL || *value == '\0' || *value == '-') {
    return 0;
  }
  errno = 0;
  parsed = strtoul(value, &end, 10);
  if (errno != 0 || *end != '\0' || parsed == 0 || parsed > 65535) {
    return 0;
  }
  *port = (unsigned short)parsed;
  return 1;
}

/* Ponto de entrada do servidor:
 * 1. lê configuração do ambiente e carrega/valida usuários uma única vez;
 * 2. inicializa Winsock quando aplicável e abre o socket TCP;
 * 3. aceita conexões sequencialmente, processa uma requisição e fecha o socket.
 * AUTH_USERS_JSON tem precedência sobre AUTH_USERS_FILE; o arquivo padrão é
 * apenas o exemplo vazio. Falha de configuração impede o servidor de iniciar. */
int main(void) {
  const char *data_path = getenv("AUTH_USERS_FILE");
  const char *data_json = getenv("AUTH_USERS_JSON");
  const char *web_root = getenv("WEB_ROOT");
  const char *host = getenv("HOST");
  const char *port_text = getenv("PORT");
  unsigned short port;
  UserDatabase *database;
  Socket server_socket;
  struct sockaddr_in address;

  if (data_path == NULL || *data_path == '\0') {
#ifdef _WIN32
    data_path = "data\\users.example.json";
#else
    data_path = "data/users.example.json";
#endif
  }
  if (web_root == NULL || *web_root == '\0') {
    web_root = "public";
  }
  if (host == NULL || *host == '\0') {
    host = "127.0.0.1";
  }
  if (port_text == NULL || *port_text == '\0') {
    port_text = "3000";
  }
  if (!parse_port(port_text, &port)) {
    fprintf(stderr,
            "{\"evento\":\"falha_de_inicializacao\","
            "\"tipoErro\":\"PortaInvalida\"}\n");
    return 1;
  }

  database = (UserDatabase *)malloc(sizeof(*database));
  if (database == NULL ||
      !(data_json != NULL
            ? load_user_database_json(data_json, strlen(data_json), database)
            : load_user_database(data_path, database))) {
    free(database);
    fprintf(stderr,
            "{\"evento\":\"falha_de_inicializacao\","
            "\"tipoErro\":\"RepositorioDeUsuariosInvalido\"}\n");
    return 1;
  }

#ifdef _WIN32
  {
    WSADATA data;
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
      free(database);
      fprintf(stderr,
              "{\"evento\":\"falha_de_inicializacao\","
              "\"tipoErro\":\"FalhaDeRede\"}\n");
      return 1;
    }
  }
#endif

  server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket == (Socket)SOCKET_ERROR_CODE) {
    free(database);
    fprintf(stderr,
            "{\"evento\":\"falha_de_inicializacao\","
            "\"tipoErro\":\"FalhaDeRede\"}\n");
    return 1;
  }

  {
    int reuse_address = 1;
#ifdef _WIN32
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR,
               (const char *)&reuse_address, (int)sizeof(reuse_address));
#else
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &reuse_address,
               sizeof(reuse_address));
#endif
  }

  memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_port = htons(port);
  address.sin_addr.s_addr = inet_addr(host);
  if (address.sin_addr.s_addr == INADDR_NONE ||
      bind(server_socket, (struct sockaddr *)&address,
#ifdef _WIN32
           (int)sizeof(address)
#else
           (socklen_t)sizeof(address)
#endif
           ) == SOCKET_ERROR_CODE ||
      listen(server_socket, 16) == SOCKET_ERROR_CODE) {
    CLOSE_SOCKET(server_socket);
    free(database);
#ifdef _WIN32
    WSACleanup();
#endif
    fprintf(stderr,
            "{\"evento\":\"falha_de_inicializacao\","
            "\"tipoErro\":\"FalhaDeRede\"}\n");
    return 1;
  }

  for (;;) {
    /* O servidor é síncrono e atende uma conexão por vez. O timeout de leitura
     * e os limites acima protegem o ciclo, mas não o tornam concorrente. */
    Socket client = accept(server_socket, NULL, NULL);
    char request_buffer[MAX_REQUEST_SIZE];
    size_t request_length = 0;
    HttpRequest request;
    int status;
    if (client == (Socket)SOCKET_ERROR_CODE) {
      continue;
    }
    set_receive_timeout(client);
    status = receive_request(client, request_buffer, sizeof(request_buffer),
                             &request_length, &request);
    if (status == 413) {
      send_json(client, 413, "CORPO_MUITO_GRANDE",
                "O corpo da requisição excede o limite permitido.");
    } else if (status != 200) {
      send_json(client, 400, "REQUISICAO_INVALIDA",
                "A requisição contém dados ausentes ou inválidos.");
    } else {
      handle_request(client, request_buffer, request_length, &request,
                     database, web_root);
    }
    CLOSE_SOCKET(client);
  }
}
