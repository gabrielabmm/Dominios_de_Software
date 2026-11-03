# 2. Arquitetura, Dados e Algoritmos

**Autor:** Emilly Ranny

**Revisor:** Gabriela Brasileiro

## 1. Objetivo e premissas

Este documento define o fluxo HTTP, os modelos de dados e os algoritmos do protótipo de autenticação, em conformidade com [1. Escopo e Requisitos de Autenticação](./1_Escopo_e_Requisitos_Autenticacao.md). A interface usa HTML, CSS mínimo e JavaScript; a API e a autenticação são implementadas em C. O repositório de usuários é JSON e somente para leitura. Essas escolhas atendem ao protótipo e não estabelecem a tecnologia da solução final.

A única operação de negócio descrita é verificar credenciais e retornar a identidade do usuário autenticado. Não há cadastro, recuperação de senha, autorização ou criação e gerenciamento de sessão.

## 2. Módulos e responsabilidades

- **Controlador HTTP em C:** recebe a requisição, valida sua estrutura, transforma a entrada HTTP em DTO de entrada, chama o módulo de autenticação e converte o resultado em resposta HTTP. Não consulta diretamente o repositório nem verifica senhas.
- **Módulo de autenticação:** a função C `authenticate` recebe `const UserDatabase *database`, o identificador, a senha e um buffer de saída para o identificador do usuário. Ela percorre os registros carregados, verifica a senha e retorna um valor `AuthResult`.
- **Repositório de usuários:** o protótipo carrega o JSON na inicialização para `UserDatabase`, uma estrutura em memória com limite de 256 registros. `authenticate` procura o identificador por comparação exata; não há uma interface de repositório nem adaptadores intercambiáveis.
- **Verificador de senha:** a função interna `verify_password` usa PBKDF2-HMAC-SHA-256 para verificar a senha, usando a API criptográfica nativa do Windows no desenvolvimento local e OpenSSL no Linux/Render. A senha digitada não é persistida nem incluída na resposta.
- **Interface web:** HTML, CSS mínimo e JavaScript enviam as credenciais ao backend e apresentam o resultado. O JavaScript não decide se a autenticação é válida; essa decisão é exclusivamente do backend C.

O controlador é o adaptador de transporte HTTP. A função `authenticate` recebe explicitamente a base carregada pelo processo, mas consulta diretamente o vetor em memória. Os testes automatizados são de integração HTTP: compilam o servidor, inicializam-no com arquivos JSON temporários e verificam as respostas e os recursos estáticos.

## 3. Fluxo detalhado da autenticação

O protótipo recebe a requisição em `POST /autenticacao`.

1. **Recepção HTTP:** o servidor aceita a conexão, lê a linha de requisição e os cabeçalhos, valida `Content-Length` e recebe o corpo dentro dos limites definidos.
2. **Rota e método:** para `POST /autenticacao`, o controlador confirma o método e exige `Content-Type: application/json`. Método incorreto retorna `405` com `Allow: POST`; rota desconhecida retorna `404`; conteúdo ausente ou incompatível retorna `415`.
3. **Entrada:** `parse_credentials` interpreta o objeto JSON e exige os campos `identificador` e `senha` como textos não vazios. Campo ausente, vazio, tipo incorreto, campo extra ou JSON malformado retorna `400`; corpo acima de 16 KiB retorna `413`. A autenticação não é chamada nesses casos e a senha não é aparada nem normalizada.
4. **Busca:** `authenticate` percorre `database->users` e compara `identificador` exatamente, com distinção entre maiúsculas e minúsculas. A base já foi carregada na inicialização; não há consulta externa por requisição.
5. **Usuário inexistente:** se nenhum registro corresponder, o módulo executa uma derivação PBKDF2 fictícia. Se a derivação funcionar, retorna `AUTH_INVALID_CREDENTIALS`; se falhar, retorna `AUTH_INTERNAL_ERROR`.
6. **Comparação:** para o registro encontrado, `verify_password` decodifica o salt e o hash, deriva a chave PBKDF2 e compara os 32 bytes em tempo constante. Hash malformado ou falha de derivação retorna `AUTH_INTERNAL_ERROR`; senha divergente retorna `AUTH_INVALID_CREDENTIALS`.
7. **Decisão de sucesso:** se a comparação corresponder, `authenticate` copia `usuario_id` para o buffer de saída e retorna `AUTH_SUCCESS`.
8. **Resposta HTTP:** `AUTH_SUCCESS` produz `200 OK` com `{"usuarioId":"..."}`; credenciais inválidas produzem `401 Unauthorized`; erro interno produz `500 Internal Server Error`. As respostas JSON usam os campos `codigo` e `mensagem`; todas as respostas HTTP incluem `Cache-Control: no-store`.

O repositório não é consultado em tempo de requisição: arquivo ou JSON inválido impede a inicialização. Durante a autenticação, hash inválido e falha criptográfica são erros internos (`500`), e não devem ser convertidos em `401` ou sucesso.

O fluxo de autenticação da aplicação está representado visualmente no [Diagrama de Sequência](./Diagramas/Diagrama_Sequencia/Diagrama_Sequencia.png), que ilustra a interação entre os componentes envolvidos durante o processamento da requisição.

## 4. Modelo de dados e DTOs

As tabelas a seguir registram os nomes JSON e C efetivamente usados no protótipo. Os DTOs são objetos JSON no limite HTTP; não há `struct` C própria para DTO de entrada, saída ou erro. Os limites de buffers C incluem o byte terminador nulo e são limites em bytes, não em caracteres Unicode.

### 4.1 Entidade `Usuário` no repositório

A Tabela 1 descreve o objeto de cada elemento de `usuarios` no JSON e o seu armazenamento em `AuthUser`.

**Tabela 1 — Campos da entidade `Usuário`**

| Campo JSON | Campo C | Tipo e limite no C | Obrigatório |
|---|---|---:|---|
| `usuarioId` | `usuario_id` | `char[128]` (até 127 bytes úteis) | Sim |
| `identificador` | `identificador` | `char[256]` (até 255 bytes úteis) | Sim |
| `senhaHash` | `senha_hash` | `char[256]` (até 255 bytes úteis) | Sim |

**Legenda:** os nomes à esquerda são as chaves do registro JSON; os nomes à direita e os tipos são os campos reais da estrutura C `AuthUser`. O vetor de usuários tem limite de 256 registros (`AUTH_MAX_USERS`); `UserDatabase.count` é do tipo `size_t`.

O parser rejeita registros com campos ausentes, campos não previstos, identificadores ou IDs duplicados, e senha em texto puro. `senhaHash` guarda o resultado PBKDF2 da senha, não a senha original.

### 4.2 DTO de entrada — `Credenciais`

A Tabela 2 relaciona o JSON recebido com os buffers preenchidos por `parse_credentials`.

**Tabela 2 — Campos do DTO de entrada `Credenciais`**

| Campo JSON | Tipo JSON | Buffer C | Capacidade |
|---|---|---|---:|
| `identificador` | String | `identifier` (`char`) | 256 bytes, incluindo `\0` |
| `senha` | String | `password` (`char`) | 512 bytes, incluindo `\0` |

**Legenda:** `parse_credentials` rejeita strings que não caibam nesses buffers; o terminador nulo ocupa um byte da capacidade indicada.

Exemplo de corpo JSON:

```json
{
  "identificador": "usuario@exemplo.com",
  "senha": "senha-fornecida"
}
```

O exemplo é ilustrativo; a senha nunca deve ser incluída em logs, mensagens de erro ou respostas.

### 4.3 DTO de saída de sucesso — `Identidade do Usuário`

A Tabela 3 identifica o campo serializado após uma autenticação bem-sucedida.

**Tabela 3 — Campos do DTO de sucesso `Identidade do Usuário`**

| Campo JSON | Tipo JSON | Buffer C | Capacidade |
|---|---|---:|---|
| `usuarioId` | String | `user_id` (`char`) | 128 bytes, incluindo `\0` |

**Legenda:** o identificador é texto JSON e é copiado para um buffer C de capacidade fixa antes da serialização.

O buffer é preenchido por `authenticate`; o controlador serializa seu conteúdo como `usuarioId`.

Exemplo de resposta `200 OK`:

```json
{
  "usuarioId": "id-estavel-do-usuario"
}
```

O DTO não inclui senha, hash, segredo, permissões nem dados desnecessários à identificação. Retorná-lo não cria sessão nem concede autorização.

### 4.4 DTO de erro

A Tabela 4 relaciona os campos que aparecem nas respostas JSON de erro.

**Tabela 4 — Campos do DTO de erro**

| Campo JSON | Tipo JSON | Origem no C |
|---|---|---|
| `codigo` | String | Argumento `code` de `send_json` ou valor constante de `send_method_not_allowed` |
| `mensagem` | String | Argumento `message` de `send_json` ou valor constante de `send_method_not_allowed` |

**Legenda:** ambos os campos são strings JSON; os valores variam conforme o erro HTTP e são definidos no controlador.

Exemplo para falha de autenticação (`401 Unauthorized`):

```json
{
  "codigo": "AUTENTICACAO_INVALIDA",
  "mensagem": "Identificador ou senha inválidos."
}
```

O mesmo código e a mesma mensagem devem ser usados para usuário inexistente e senha incorreta, evitando revelar a existência de contas.

**Legenda das Tabelas 2 a 4:** os tipos JSON e os nomes serializados correspondem ao contrato HTTP. Os nomes de buffers identificam a representação C usada pelo controlador; não existem estruturas DTO C separadas.

## 5. Algoritmos e tratamento de erros

### 5.1 Validação da existência e decisão de autenticação

O Algoritmo 1 representa o fluxo implementado por `authenticate`; a base é um vetor já carregado, não um repositório consultado externamente.

**Algoritmo 1 — Localização do usuário, verificação de senha e decisão**

```text
função authenticate(database, identifier, password, user_id):
    para cada user em database.users:
        se user.identificador == identifier:
            resultado = verify_password(password, user.senha_hash)
            se resultado indica erro:
                retornar AUTH_INTERNAL_ERROR
            se resultado indica divergência:
                retornar AUTH_INVALID_CREDENTIALS
            copiar user.usuario_id para user_id
            retornar AUTH_SUCCESS

    se perform_dummy_derivation(password) falhar:
        retornar AUTH_INTERNAL_ERROR
    retornar AUTH_INVALID_CREDENTIALS
```

O enum C `AuthResult` possui exatamente os resultados `AUTH_SUCCESS`, `AUTH_INVALID_CREDENTIALS` e `AUTH_INTERNAL_ERROR`. O arquivo JSON é validado no início do processo; falha de leitura ou parsing impede a inicialização, em vez de ocorrer como erro de uma consulta por requisição.

### 5.2 Comparação segura da senha

O arquivo guarda um hash da senha, não a senha original. Para verificar o login, o backend aplica PBKDF2-HMAC-SHA-256 à senha digitada, usando o salt guardado no registro, e compara o resultado com o hash registrado. Cada usuário tem seu próprio salt; o protótipo usa 600.000 iterações e compara os bytes em tempo constante.

Assim, o servidor confirma se a senha digitada corresponde à cadastrada sem salvar nem devolver a senha em texto puro. Se a senha não corresponder, o resultado é `AUTH_INVALID_CREDENTIALS`; se o hash estiver malformado ou a operação criptográfica falhar, o resultado é `AUTH_INTERNAL_ERROR`. O arquivo de usuários é preparado fora da API, que não cadastra usuários nem gera hashes.

### 5.3 Mapeamento de erros HTTP

O controlador usa os códigos e as mensagens da Tabela 5; os valores são fixos no código, não sugestões.

**Tabela 5 — Mapeamento HTTP de erros implementado**

| Situação | Status HTTP | Código | Mensagem externa |
|---|---:|---|---|
| Cabeçalhos/requisição inválidos, corpo malformado/vazio, credenciais ausentes, vazias ou inválidas | `400 Bad Request` | `REQUISICAO_INVALIDA` | `A requisição contém dados ausentes ou inválidos.` |
| Corpo da requisição acima de 16 KiB | `413 Payload Too Large` | `CORPO_MUITO_GRANDE` | `O corpo da requisição excede o limite permitido.` |
| `Content-Type` ausente ou diferente de JSON na autenticação | `415 Unsupported Media Type` | `TIPO_DE_CONTEUDO_INVALIDO` | `O corpo deve usar application/json.` |
| Rota desconhecida ou arquivo estático inexistente | `404 Not Found` | `RECURSO_NAO_ENCONTRADO` | `Recurso não encontrado.` |
| Método incompatível com a rota | `405 Method Not Allowed` | `METODO_NAO_PERMITIDO` | `Método não permitido.` |
| Usuário não encontrado ou senha incorreta | `401 Unauthorized` | `AUTENTICACAO_INVALIDA` | `Identificador ou senha inválidos.` |
| Hash inválido, falha criptográfica ou falha ao servir arquivo | `500 Internal Server Error` | `ERRO_INTERNO` | `Não foi possível concluir a autenticação.` na autenticação; `Não foi possível concluir a requisição.` ao servir arquivo. |

**Legenda:** os códigos e as mensagens são os valores enviados pelo controlador; status HTTP aplica-se a requisições atendidas pelo servidor.

Falhas de inicialização não geram resposta HTTP: o processo termina com evento `falha_de_inicializacao` e tipo `PortaInvalida`, `RepositorioDeUsuariosInvalido` ou `FalhaDeRede`, conforme a causa. A Tabela 5 não inclui esses eventos de processo.

O corpo de resposta segue o DTO de erro definido acima. A mensagem externa não deve conter stack trace, consulta, hash, senha, existência do usuário ou detalhe da infraestrutura. O erro interno deve ser registrado para diagnóstico sem registrar credenciais ou segredos. A aplicação não deve capturar indiscriminadamente qualquer erro e convertê-lo em `401` ou em resposta de sucesso.

## 6. Verificação por testes

Os testes de integração exercitam o processo HTTP compilado e os cenários da [Tabela 1 do Entregável 3](./3_Cenarios_Sucesso_e_Falha.md). Devem cobrir:

- credenciais válidas: sucesso contendo o `usuarioId` correto;
- usuário inexistente e senha incorreta: mesma categoria, código e mensagem externos (`401`);
- campos ausentes, vazios ou corpo malformado: `400`, sem chamada ao fluxo de autenticação;
- hash inválido ou falha do verificador: `500`, sem revelar detalhes internos; falha de carga do repositório: processo não inicia;
- resposta de sucesso sem senha ou hash;
- verificação de uma senha derivada por PBKDF2; comparação em tempo constante e nenhuma persistência de senha em texto puro. O tempo constante é garantido pelo algoritmo implementado, não medido pelo teste de integração.

Os cenários automatizados correspondentes estão descritos em [3. Cenários de Sucesso e Falha](./3_Cenarios_Sucesso_e_Falha.md) e são executados com `python -m unittest discover -s tests -v` na pasta `backend`.

## 7. Decisões do protótipo e pontos para a solução final

- O endpoint do protótipo é `POST /autenticacao`; o servidor escuta em `127.0.0.1:3000` por padrão.
- O repositório do protótipo é um arquivo JSON local configurável por `AUTH_USERS_FILE`, lido na inicialização. A API não cadastra nem altera usuários; identificadores são comparados exatamente.
- O protótipo usa PBKDF2-HMAC-SHA-256 com 600.000 iterações; a interface e a API são servidas pelo mesmo processo C.
- Quando o identificador não existe, o protótipo realiza uma derivação fictícia de custo equivalente antes de retornar `401`, reduzindo vazamento de existência por diferenças de tempo.
- Em Render, o repositório pode ser fornecido pela variável de ambiente `AUTH_USERS_JSON`; localmente, pode ser carregado do arquivo indicado por `AUTH_USERS_FILE`.
- Para uma solução final, definir o repositório apropriado, a política de identificadores, os parâmetros criptográficos e o mecanismo confiável de provisionamento dos registros, sem ampliar a API de autenticação para incluir cadastro.
