# 5. Interfaces, Dependências e Decisões Arquiteturais

## 5.1. Interfaces de comunicação

As interfaces definem como os diferentes componentes da aplicação trocam informações e solicitam serviços uns aos outros.

Na aplicação Web de autenticação, a comunicação acontece principalmente por meio de requisições HTTP entre o navegador e o servidor, além das chamadas internas realizadas pelo próprio back-end.

### 5.1.1. Interface entre Front-end e Back-end

A comunicação entre o navegador e o servidor ocorre por meio do protocolo HTTP.

Quando o usuário informa suas credenciais, o JavaScript prepara os dados no formato JSON e envia uma requisição POST para o endpoint `/autenticacao`.

A requisição utiliza o cabeçalho `Content-Type: application/json`, indicando o formato dos dados enviados.

**Características da interface:**

- Protocolo: HTTP.
- Método: POST.
- Endpoint: `/autenticacao`.
- Formato dos dados: JSON.
- Origem: interface Web.
- Destino: servidor HTTP implementado em C.

O servidor recebe a requisição e encaminha as informações para o processamento da autenticação.

### 5.1.2. Interface entre Servidor HTTP e Módulo de Autenticação

Após receber a requisição, o servidor HTTP encaminha as credenciais para o módulo responsável pela autenticação.

Essa comunicação ocorre internamente no back-end, sem a necessidade de uma nova requisição HTTP externa.

O módulo recebe os dados necessários, consulta a base de usuários e executa a verificação das credenciais.

Ao finalizar o processamento, retorna o resultado ao servidor HTTP, que fica responsável por construir e enviar a resposta ao navegador.

### 5.1.3. Interface entre Módulo de Autenticação e UserDatabase

O módulo de autenticação utiliza a estrutura UserDatabase para consultar os usuários cadastrados.

Essa estrutura é carregada a partir de um arquivo JSON durante a inicialização do servidor e permanece disponível em memória para as consultas realizadas durante a execução da aplicação.

A consulta permite localizar o usuário correspondente às credenciais recebidas e obter os dados necessários para a verificação da senha.

### 5.1.4. Interface entre Arquivo JSON e UserDatabase

O arquivo JSON funciona como fonte inicial dos dados dos usuários.

Durante a inicialização do servidor, as informações são lidas e carregadas para a estrutura UserDatabase, que passa a ser utilizada pelo módulo de autenticação.

Dessa forma, o arquivo fornece os dados iniciais, enquanto a estrutura em memória permite que as consultas sejam realizadas durante o funcionamento da aplicação.

## 5.2. Dependências entre componentes

Os componentes da aplicação possuem dependências necessárias para que o processo de autenticação funcione corretamente.

| Componente | Dependência | Finalidade |
|---|---|---|
| Interface Web | Servidor HTTP | Enviar credenciais e receber respostas. |
| Servidor HTTP | Módulo de autenticação | Encaminhar os dados para verificação. |
| Módulo de autenticação | UserDatabase | Consultar os usuários. |
| Módulo de autenticação | Implementação de PBKDF2-HMAC-SHA-256 | Verificar as senhas. |
| UserDatabase | Arquivo JSON de usuários | Obter os dados durante a inicialização. |

Essas dependências demonstram que o processo de autenticação depende da integração entre a interface, o servidor, a lógica de verificação e a base de usuários.

## 5.3. Decisões arquiteturais e justificativas

As decisões arquiteturais identificadas na aplicação estão relacionadas à organização dos componentes, à comunicação e ao armazenamento dos dados.

**Arquitetura cliente-servidor**

A aplicação utiliza uma separação entre a interface apresentada ao usuário e o processamento realizado no servidor. Essa organização permite distinguir as responsabilidades do front-end e do back-end.

**Utilização de C no servidor**

O back-end foi implementado em linguagem C, concentrando o atendimento às requisições HTTP e o processamento da autenticação em um mesmo processo.

**Comunicação HTTP com dados em JSON**

O uso de HTTP permite a comunicação entre o navegador e o servidor, enquanto o JSON organiza os dados enviados durante a autenticação em um formato estruturado.

**Separação lógica de responsabilidades**

Mesmo com as funcionalidades executadas no mesmo processo, o sistema diferencia as responsabilidades de comunicação HTTP e processamento da autenticação. Isso facilita a compreensão da arquitetura.

**Armazenamento inicial em arquivo JSON e consultas em memória**

Os usuários são carregados de um arquivo JSON durante a inicialização do servidor e disponibilizados em uma estrutura em memória. Essa abordagem permite consultar os dados durante as tentativas de autenticação sem precisar carregar novamente o arquivo a cada solicitação.

**Verificação de senhas com PBKDF2-HMAC-SHA-256**

A utilização desse mecanismo permite realizar a verificação das credenciais com uma função específica para derivação de chaves a partir de senhas, em vez de realizar uma comparação direta de senhas em texto puro.

## 5.4. Considerações finais

A documentação das interfaces e dependências permite compreender como os componentes trabalham em conjunto para realizar a autenticação.

A identificação das decisões arquiteturais também registra as escolhas realizadas na implementação e suas justificativas, contribuindo para a manutenção, compreensão e evolução do sistema.
