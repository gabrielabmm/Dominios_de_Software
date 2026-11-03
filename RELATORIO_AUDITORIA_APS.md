# Relatório de Auditoria da APS — Autenticação

**Autor:** Emilly Ranny

**Revisor:** Emilly Ranny

## 1. Escopo e critérios avaliados

Esta auditoria examinou os três documentos da APS, o código C, a interface web, os testes de integração e as instruções do protótipo. Os critérios abaixo foram extraídos do pedido de auditoria. Os textos integrais dos Apêndices A e B e da rubrica oficial da APS não estão presentes no repositório; portanto, não é possível atestar requisitos que dependam de conteúdo não fornecido.

## 2. Resultado item a item

### 2.1 Escopo e essencialidade — Apêndices A e B

A Tabela 1 registra a avaliação do escopo e a limitação causada pela ausência dos apêndices.

**Tabela 1 — Escopo e essencialidade**

| Item | Resultado | Evidência e observação |
|---|---|---|
| Código limitado à verificação de credenciais | [OK] | A única operação de negócio é `POST /autenticacao`; não há cadastro, recuperação/alteração de senha, sessão, autorização ou gestão de usuários. O serviço de arquivos estáticos atende à navegação da interface. |
| Limites documentados no Entregável 1 | [OK] | A Seção 3 de [1_Escopo_e_Requisitos_Autenticacao.md](./1_Escopo_e_Requisitos_Autenticacao.md) enumera funcionalidades excluídas e esclarece que autenticação não cria sessão nem concede autorização. |
| Conformidade integral com os Apêndices A e B | [PENDENTE/CORRIGIR] | Os apêndices não existem neste repositório e seu conteúdo não foi fornecido. A conformidade foi avaliada somente contra os critérios reproduzidos no pedido. |

**Legenda:** `[OK]` indica critério atendido; `[PENDENTE/CORRIGIR]` indica item que depende de material não disponível.

### 2.2 Quatro etapas obrigatórias — Entregáveis 2 e 3

A Tabela 2 verifica individualmente as quatro etapas obrigatórias.

**Tabela 2 — Etapas do fluxo de autenticação**

| Etapa | Resultado | Evidência |
|---|---|---|
| Entrada: identificador/e-mail e senha | [OK] | O DTO JSON exige `identificador` e `senha`; a interface solicita ambos. Ver `parse_credentials` e a Seção 4.2 de [2_Arquitetura_Dados_e_Algoritmos.md](./2_Arquitetura_Dados_e_Algoritmos.md). |
| Validação: existência no repositório | [OK] | `authenticate` percorre `UserDatabase.users` e compara o identificador com cada registro. O arquivo JSON é carregado e validado na inicialização. |
| Comparação segura da credencial | [OK] | `verify_password` deriva PBKDF2-HMAC-SHA-256 (600.000 iterações) e compara a chave em tempo constante. |
| Decisão de autenticação | [OK] | Sucesso retorna `200` com `{"usuarioId":"..."}`; identificador inexistente ou senha incorreta retorna `401` com erro genérico. |

**Legenda:** `[OK]` indica etapa implementada e documentada de acordo com o critério.

### 2.3 Consistência entre código e documentação — Entregável 4, Critério 6

A Tabela 3 consolida a comparação entre os modelos, algoritmos e respostas HTTP documentados e implementados.

**Tabela 3 — Consistência da documentação com o código**

| Item | Resultado | Evidência e correção |
|---|---|---|
| Entidade `Usuário` | [OK] | A Tabela 1 do Entregável 2 agora distingue as chaves JSON (`usuarioId`, `identificador`, `senhaHash`) dos campos reais C (`usuario_id`, `identificador`, `senha_hash`), seus tipos e capacidades. |
| DTO de entrada | [OK] | A Tabela 2 corresponde às strings JSON e aos buffers C de 256 e 512 bytes, incluindo o terminador nulo. Não há uma estrutura `Credenciais` em C; a documentação agora identifica corretamente o DTO como contrato HTTP. |
| DTO de sucesso | [OK] | A Tabela 3 registra o campo JSON `usuarioId` e o buffer C `user_id` de 128 bytes, incluindo o terminador. |
| DTO de erro | [OK] | A Tabela 4 corresponde aos campos JSON string `codigo` e `mensagem`, emitidos por `send_json` ou pela resposta de método não permitido. |
| Algoritmo de autenticação | [OK] | O pseudocódigo agora reflete a busca no vetor carregado, os resultados `AUTH_SUCCESS`, `AUTH_INVALID_CREDENTIALS` e `AUTH_INTERNAL_ERROR`, a derivação fictícia e as falhas internas. |
| Status, códigos e mensagens HTTP | [OK] | A Tabela 5 foi alinhada ao controlador, incluindo `400`, `401`, `404`, `405`, `413`, `415`, `500`, os valores exatos de códigos e mensagens e as diferenças entre falhas HTTP e de inicialização. |
| Trechos desatualizados ou arquitetura não implementada | [OK] | Foram removidas afirmações de que ainda não havia implementação/testes e referências a interfaces/adaptadores de repositório e DTOs C inexistentes. |

**Legenda:** `[OK]` indica correspondência verificada após as correções desta auditoria.

### 2.4 Cenários de sucesso e falha — Entregável 5

A Tabela 4 resume a cobertura dos grupos de cenários e a execução automatizada.

**Tabela 4 — Cobertura e validação dos cenários**

| Grupo de cenários | Resultado | Cobertura automatizada |
|---|---|---|
| Credenciais válidas, caracteres Unicode, identificador inexistente e senha incorreta | [OK] | Testes de sucesso e comparação segura; inexistente e senha incorreta verificam o mesmo `401`. |
| Campos ausentes, vazios, tipo inválido, campo extra, JSON malformado/vazio e corpo excessivo | [OK] | Cobertos por `test_entrada_invalida_retorna_400`, `test_json_malformado_ou_vazio_retorna_400` e `test_corpo_acima_de_16_kib_retorna_413`. |
| `Content-Type` incompatível ou ausente | [OK] | `test_tipo_de_conteudo_invalido_retorna_415` agora verifica ambos os casos. |
| Método, rota e interface estática | [OK] | Testes verificam `405`/`Allow`, `404` e os recursos HTML, JavaScript e CSS. |
| Hash inválido, arquivo ausente e JSON/registros inválidos | [OK] | Testes verificam `500` para hash inválido e impedimento de inicialização para repositório inválido ou ausente. |
| Execução da suíte | [OK] | `python -m unittest discover -s backend\tests -v`: **16 testes passaram**; compilação C usa `-Wall -Wextra -Werror`. |

**Legenda:** `[OK]` indica que o grupo tem cobertura automatizada e que a suíte foi executada sem falhas.

A Tabela 5 mapeia cada cenário da Tabela 1 do Entregável 3 ao método automatizado correspondente.

**Tabela 5 — Correspondência dos cenários com os testes**

| Cenário(s) | Teste automatizado |
|---|---|
| SU-01 | `test_credenciais_validas_retorna_somente_a_identidade` |
| FA-01 e FA-02 | `test_usuario_inexistente_e_senha_incorreta_retorna_o_mesmo_401` |
| FA-03 | `test_senha_com_espacos_significativos_nao_e_normalizada` |
| EN-01 a EN-05 e EN-09 | `test_entrada_invalida_retorna_400` |
| EN-06 e EN-07 | `test_json_malformado_ou_vazio_retorna_400` |
| EN-08 | `test_corpo_acima_de_16_kib_retorna_413` |
| HT-01 | `test_tipo_de_conteudo_invalido_retorna_415` (tipo incompatível e cabeçalho ausente) |
| HT-02 | `test_metodo_invalido_retorna_405_e_allow` |
| HT-03 | `test_rota_inexistente_retorna_404` |
| ER-01 | `test_hash_invalido_retorna_500_e_nao_expoe_detalhes_no_log` |
| CFG-01 | `test_repositorio_invalido_impede_inicializacao`, `test_json_de_usuarios_invalido_no_ambiente_impede_inicializacao` e `test_arquivo_de_usuarios_ausente_impede_inicializacao` |
| CFG-02 | `test_repositorio_invalido_impede_inicializacao` |
| UI-01 | `test_interface_estatica_e_servida_com_a_api` |

**Legenda:** os IDs identificam linhas da tabela de cenários do Entregável 3; nomes em fonte monoespaçada são métodos de `backend/tests/test_api.py`.

### 2.5 Formatação, autoria e linguagem — Regras 5 e 7, Critério 9

A Tabela 6 resume a verificação de autoria, apresentação e linguagem dos documentos.

**Tabela 6 — Formatação e autoria**

| Item | Resultado | Evidência e observação |
|---|---|---|
| Identificação de Autor e Revisor | [OK] | Os campos estão no topo dos três entregáveis, do README, do glossário, do README do backend e deste relatório. Permanecem em branco para preenchimento pela equipe. |
| Títulos, legendas e referências de tabelas/figuras | [OK] | As Tabelas 1 a 5 do Entregável 2 e a Tabela 1 do Entregável 3 têm título, legenda e referência no texto. Não há figuras nos documentos auditados. |
| Português do Brasil formal e objetivo | [OK] | Foram corrigidos trechos desatualizados e descrições imprecisas de implementação. A revisão linguística foi feita sobre os documentos presentes. |
| Atendimento literal às Regras 5 e 7 e ao Critério 9 | [PENDENTE/CORRIGIR] | O texto integral da rubrica oficial não está no repositório nem foi anexado. A avaliação só pode afirmar atendimento aos requisitos explicitados no pedido. |

**Legenda:** `[OK]` indica conformidade com as instruções fornecidas; `[PENDENTE/CORRIGIR]` identifica validação impossível sem a rubrica oficial.

## 3. Correções realizadas nesta auditoria

1. Atualização do Entregável 1 para remover a afirmação obsoleta de inexistência de código/testes e apontar a especificação técnica.
2. Alinhamento do Entregável 2 ao modelo C real: campos e capacidades, arquitetura, fluxo, algoritmo, códigos e mensagens HTTP.
3. Inclusão de títulos, legendas e referências explícitas para as tabelas dos Entregáveis 2 e 3.
4. Inclusão de um teste para `Content-Type` ausente, além do tipo incompatível.
5. Inclusão dos campos Autor/Revisor nos documentos do projeto e correção da árvore do README, que citava diretórios de imagens e apêndices inexistentes.

## 4. Pendências externas

- Fornecer os Apêndices A e B para uma comparação integral com esses critérios.
- Fornecer a rubrica oficial da APS para confirmar literalmente as Regras 5 e 7 e o Critério 9.
- Preencher os campos Autor e Revisor com os responsáveis definidos pela equipe.
- A compilação local e a suíte foram verificadas no Windows. A compilação Linux/OpenSSL e o deploy no Render não foram executados nesta auditoria.
