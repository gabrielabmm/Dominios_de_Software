# 1. Escopo e Requisitos de Autenticação

**Autor:** Emilly Ranny
**Revisor:** Gabriela Brasileiro

## 1. Objetivo

Este documento define o escopo e os requisitos da aplicação web de autenticação de usuários. A aplicação tem responsabilidade estrita de receber credenciais, verificar se o usuário existe no repositório de dados, comparar a credencial fornecida com a registrada e informar o resultado da autenticação.

## 2. Escopo da aplicação

A aplicação deve:

- receber um identificador (nome de usuário ou e-mail) e uma senha;
- consultar o repositório de dados para verificar a existência do usuário associado ao identificador;
- comparar a senha fornecida com a credencial registrada para esse usuário;
- decidir se a autenticação foi bem-sucedida;
- em caso de sucesso, retornar um objeto que identifique o usuário autenticado;
- em caso de falha, retornar um erro de autenticação.

## 3. Limites e itens fora do escopo

A aplicação **não** deve:

- cadastrar, criar ou provisionar usuários;
- recuperar, redefinir ou alterar senhas;
- gerenciar sessões complexas, tokens de sessão, renovação de sessão ou encerramento de sessão;
- administrar perfis, permissões, papéis ou autorização de acesso a recursos;
- oferecer funcionalidades de negócio além da verificação de credenciais;
- gerenciar ou manter o ciclo de vida do repositório de dados de usuários.

O resultado de sucesso limita-se à identificação do usuário autenticado. A aplicação não deve interpretar esse resultado como autorização para acessar recursos nem como requisito para criar ou manter uma sessão.

## 4. Atores e histórias de usuário

1. Como usuário com credenciais registradas, quero informar meu identificador e minha senha para que a aplicação verifique minha identidade.
2. Como usuário com credenciais válidas, quero receber a identificação associada à minha conta para que o resultado da autenticação possa ser reconhecido pelo sistema consumidor.
3. Como usuário com credenciais inválidas ou não registradas, quero receber um erro de autenticação para saber que a verificação não foi bem-sucedida.
4. Como sistema consumidor, quero receber um resultado inequívoco de sucesso ou falha para tratar a autenticação sem depender de funcionalidades fora do escopo.

## 5. Requisitos e regras de autenticação

### RF-01 — Entrada de credenciais

A aplicação deve aceitar uma solicitação de autenticação contendo:

- **identificador:** nome de usuário ou endereço de e-mail;
- **senha:** credencial apresentada pelo usuário.

Esses dados constituem a entrada necessária para iniciar a verificação.

### RF-02 — Verificação da existência do usuário

A aplicação deve consultar o repositório de dados usando o identificador informado e verificar se existe um usuário associado a ele.

- Se não houver usuário associado, a autenticação deve falhar e a aplicação deve retornar um erro de autenticação.
- Se houver usuário associado, a aplicação deve prosseguir para a comparação da credencial.

### RF-03 — Comparação da credencial

Para um usuário encontrado, a aplicação deve comparar a senha fornecida com a credencial registrada para esse usuário.

- Se a credencial fornecida corresponder à registrada, a aplicação deve prosseguir para a decisão de sucesso.
- Se não corresponder, a autenticação deve falhar e a aplicação deve retornar um erro de autenticação.

### RF-04 — Decisão e resultado da autenticação

A aplicação deve produzir um dos seguintes resultados:

- **Sucesso:** retornar um objeto com a identificação do usuário cuja credencial foi verificada.
- **Falha:** retornar um erro de autenticação quando o usuário não existir no repositório ou quando a credencial fornecida não corresponder à registrada.

### 5.1 Fluxo obrigatório

O processamento de uma solicitação deve seguir estas quatro etapas, nesta ordem:

1. **Entrada de credenciais:** receber o identificador (nome de usuário ou e-mail) e a senha.
2. **Validação das credenciais:** verificar no repositório de dados se existe um usuário associado ao identificador.
3. **Comparação da credencial:** comparar a senha fornecida com a credencial registrada para o usuário encontrado.
4. **Decisão de autenticação:** retornar a identificação do usuário em caso de sucesso ou um erro de autenticação em caso de falha.

Se a verificação de existência não encontrar um usuário, o fluxo deve terminar na segunda etapa com falha; não há credencial registrada para comparar.

## 6. Decisões e contrato funcional

- A autenticação depende da existência de um usuário no repositório de dados e da correspondência entre a senha fornecida e a credencial registrada.
- O sucesso deve identificar o usuário autenticado; não deve, por si só, criar uma sessão ou conceder permissões.
- A ausência do usuário e a divergência da senha devem resultar em falha de autenticação.
- Os formatos técnicos do objeto de sucesso, da credencial armazenada e do erro são detalhados no [Entregável 2](./2_Arquitetura_Dados_e_Algoritmos.md), sem ampliar o escopo funcional.
- Falhas de infraestrutura do repositório não devem ser apresentadas como sucesso de autenticação; o tratamento técnico dessas falhas é definido na especificação do protótipo.

## 7. Decisões de teste e critérios de aceitação

Os testes devem verificar o comportamento observável do fluxo de autenticação, sem depender de detalhes internos de implementação. O ponto de teste preferencial é a interface de entrada e saída da autenticação.

O comportamento é aceito quando:

1. Uma solicitação com identificador existente e senha correspondente retorna sucesso e identifica o usuário correto.
2. Uma solicitação com identificador inexistente retorna erro de autenticação.
3. Uma solicitação com identificador existente e senha divergente retorna erro de autenticação.
4. Uma falha ao consultar o repositório não resulta em sucesso de autenticação.
5. O resultado de sucesso não implica cadastro de usuário, recuperação de senha, gerenciamento de sessão ou autorização.

Os cenários e testes automatizados do protótipo estão relacionados no [Entregável 3](./3_Cenarios_Sucesso_e_Falha.md).

## 8. Observações

Este documento delimita os requisitos funcionais da autenticação. Decisões técnicas necessárias à implementação, como formato de entrada e saída, armazenamento e verificação segura de senhas e tratamento de falhas de infraestrutura, devem ser detalhadas sem adicionar funcionalidades fora do escopo estabelecido.
