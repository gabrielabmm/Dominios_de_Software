# 3. Cenários de Sucesso e Falha

**Autor:** Emilly Ranny

**Revisor:** 

## Objetivo

A Tabela 1 especifica os cenários de teste do protótipo de autenticação descrito em [2. Arquitetura, Dados e Algoritmos](./2_Arquitetura_Dados_e_Algoritmos.md). O repositório guarda um hash da senha; no login, o backend calcula o hash da senha digitada e confere se corresponde ao registrado. Os testes exercitam a API HTTP `POST /autenticacao`, a interface estática e o backend C com arquivos JSON temporários. Nenhum cenário cria ou altera usuários.

**Tabela 1 — Cenários funcionais, de validação, HTTP e configuração do protótipo**

| ID | Cenário | Entrada | Pré-condição | Processamento | Resultado esperado |
|---|---|---|---|---|---|
| SU-01 | Credenciais válidas | `POST /autenticacao` com identificador existente e senha correta | O repositório JSON contém o usuário e o hash da senha | Validar a requisição, localizar o usuário e comparar o hash calculado com o registrado | `200 OK`; resposta contém somente `usuarioId`; cabeçalho `Cache-Control: no-store`; não retorna senha nem hash |
| FA-01 | Identificador inexistente | Credenciais estruturalmente válidas com identificador não registrado | Repositório disponível, sem usuário com esse identificador | Consultar o repositório, executar derivação fictícia de custo equivalente e não comparar uma credencial inexistente | `401 Unauthorized`; código `AUTENTICACAO_INVALIDA` e mensagem genérica |
| FA-02 | Senha incorreta | Identificador registrado e senha digitada que não corresponde à registrada | Usuário e hash válidos no repositório | Calcular o hash da senha digitada e comparar com o hash registrado | Mesmo status, código e mensagem de FA-01: `401 Unauthorized`, sem revelar a existência do usuário |
| FA-03 | Espaços fazem parte da senha | Identificador registrado e senha fornecida com espaços adicionais em relação à credencial original | Usuário e hash válidos no repositório | Calcular e comparar o hash sem aparar ou normalizar a senha | `401 Unauthorized`; a senha não é transformada antes da verificação |
| EN-01 | Identificador ausente | Corpo JSON contém `senha`, mas não contém `identificador` | Servidor disponível | Validar os campos antes de chamar o módulo de autenticação | `400 Bad Request`; código `REQUISICAO_INVALIDA`; autenticação não é chamada |
| EN-02 | Senha ausente | Corpo JSON contém `identificador`, mas não contém `senha` | Servidor disponível | Validar os campos antes de chamar o módulo de autenticação | `400 Bad Request`; código `REQUISICAO_INVALIDA`; autenticação não é chamada |
| EN-03 | Identificador vazio ou somente espaços | Corpo JSON contém identificador sem conteúdo útil e senha não vazia | Servidor disponível | Validar o identificador sem alterar seu valor para a busca | `400 Bad Request`; código `REQUISICAO_INVALIDA`; autenticação não é chamada |
| EN-04 | Senha vazia ou somente espaços | Corpo JSON contém senha sem conteúdo útil | Servidor disponível | Validar a senha sem aparar ou normalizar a credencial enviada | `400 Bad Request`; código `REQUISICAO_INVALIDA`; autenticação não é chamada |
| EN-05 | Tipos de campo inválidos | `identificador` ou `senha` não é texto | Servidor disponível | Validar o formato do DTO de entrada | `400 Bad Request`; código `REQUISICAO_INVALIDA`; autenticação não é chamada |
| EN-06 | JSON malformado | Corpo não pode ser interpretado como JSON | Servidor disponível e `Content-Type: application/json` | Tentar interpretar o corpo e rejeitar a entrada inválida | `400 Bad Request`; código `REQUISICAO_INVALIDA` |
| EN-07 | Corpo vazio | Requisição sem conteúdo | Servidor disponível e `Content-Type: application/json` | Tentar interpretar o corpo e rejeitar a entrada inválida | `400 Bad Request`; código `REQUISICAO_INVALIDA` |
| EN-08 | Corpo acima do limite | Corpo maior que 16 KiB | Servidor disponível e `Content-Type: application/json` | Interromper a leitura lógica do conteúdo e rejeitar o tamanho excedido | `413 Payload Too Large`; código `CORPO_MUITO_GRANDE`; autenticação não é chamada |
| EN-09 | Campo não previsto | DTO contém campos além de `identificador` e `senha` | Servidor disponível | Rejeitar campos fora do contrato de entrada | `400 Bad Request`; código `REQUISICAO_INVALIDA` |
| HT-01 | Tipo de conteúdo incompatível | Requisição para autenticação sem `Content-Type: application/json` | Servidor disponível | Verificar o tipo de conteúdo antes de ler e validar credenciais | `415 Unsupported Media Type`; código `TIPO_DE_CONTEUDO_INVALIDO` |
| HT-02 | Método HTTP incompatível | Método diferente de `POST` em `/autenticacao` | Servidor disponível | Resolver a rota e rejeitar o método não permitido | `405 Method Not Allowed`; cabeçalho `Allow: POST` |
| HT-03 | Rota inexistente | Requisição a caminho diferente de `/autenticacao` | Servidor disponível | Resolver a rota antes de executar autenticação | `404 Not Found`; autenticação não é chamada |
| ER-01 | Hash armazenado inválido | Usuário existente com hash incompatível ou malformado | Repositório carregado; usuário encontrado | Tentar calcular e comparar o hash; tratar formato inválido como falha interna | `500 Internal Server Error`; erro genérico na resposta; log sem senha, hash ou detalhes internos |
| CFG-01 | Arquivo ou JSON do repositório inválido | Arquivo ausente, JSON malformado ou JSON de usuários inválido | Processo iniciado com `AUTH_USERS_FILE` ou `AUTH_USERS_JSON` apontando para dados inválidos | Carregar e validar o repositório antes de abrir o servidor | Servidor não inicia; processo termina com evento de erro sem expor conteúdo dos dados |
| CFG-02 | Registros ambíguos ou inseguros | Repositório contém identificadores/IDs duplicados, estrutura inválida ou senha em texto puro | Processo tenta carregar o repositório JSON | Validar os registros antes de disponibilizá-los | Servidor não inicia; nenhuma operação de autenticação prossegue |
| UI-01 | Interface navegável | Requisições `GET /`, `GET /app.js` e `GET /style.css` | Servidor C ativo com os arquivos estáticos disponíveis | Servir a página HTML e seus recursos estáticos | `200 OK`; página em português; JavaScript e folha CSS são retornados com seus tipos de conteúdo corretos |

**Legenda:** `SU` = sucesso; `FA` = falha de autenticação; `EN` = entrada inválida; `HT` = método, rota ou transporte HTTP; `ER` = falha interna; `CFG` = configuração do repositório; `UI` = interface navegável.

## Execução automatizada

Os códigos HTTP e os códigos de erro esperados correspondem ao contrato do protótipo.

Na pasta `backend`, execute `python -m unittest discover -s tests -v`. A suíte compila o backend C e testa a API HTTP e a interface estática. Ela requer GCC, Python 3 e, em Linux, as bibliotecas de desenvolvimento do OpenSSL; não requer banco SQL ou serviços externos. Arquivos e processos de teste são isolados e removidos ao fim de cada teste.
