# 4. Descrição detalhada do ambiente operacional
Autor: Gabriela Brasileiro
Revisor:

## 4.1. Visão geral do ambiente

O ambiente operacional da aplicação de autenticação é composto por um navegador Web, responsável pela interação com o usuário, e por um processo servidor desenvolvido em linguagem C, responsável por disponibilizar a interface Web, receber as requisições HTTP e executar a lógica de autenticação.

A interface da aplicação é desenvolvida utilizando HTML, CSS e JavaScript, enquanto o processamento das requisições e a verificação das credenciais são realizados por uma API implementada em C. Os dados dos usuários são mantidos em um arquivo JSON, utilizado exclusivamente para leitura durante a execução do protótipo.

Uma característica importante da implementação é que o servidor Web e o servidor de aplicação não são processos separados. O mesmo processo desenvolvido em C disponibiliza os arquivos estáticos da interface e executa o processamento das requisições de autenticação. Dessa forma, a comunicação entre os componentes ocorre por meio de requisições HTTP, sem a necessidade de um servidor Web intermediário independente.

## 4.2. Browser (Navegador Web)

O navegador Web constitui o ambiente de interação entre o usuário e a aplicação. É responsável por apresentar a interface gráfica, permitir o preenchimento das credenciais e encaminhar as informações fornecidas ao servidor.

A interface é composta por arquivos HTML, CSS e JavaScript, disponibilizados pelo próprio processo servidor em C. O HTML define a estrutura da página, o CSS estabelece sua apresentação visual e o JavaScript controla os eventos de interação e a comunicação com a API.

Quando o usuário informa seu identificador e sua senha e solicita a autenticação, o JavaScript organiza os dados em um objeto JSON e envia uma requisição HTTP utilizando o método POST para o endpoint `/autenticacao`.

O navegador também é responsável por receber a resposta HTTP do servidor e apresentar o resultado correspondente na interface. Entretanto, a decisão sobre a validade das credenciais não é realizada no navegador, mas exclusivamente pelo backend desenvolvido em C.

## 4.3. Servidor Web

O servidor Web é responsável por receber as requisições HTTP provenientes do navegador e disponibilizar os recursos necessários à aplicação.

No protótipo desenvolvido, essa responsabilidade é implementada pelo próprio servidor em C, que também atua como servidor de aplicação. Portanto, não existe uma camada independente baseada em ferramentas como Apache ou Nginx na arquitetura descrita.

Entre suas responsabilidades estão:

- Aceitar conexões provenientes dos clientes;
- Receber e interpretar as requisições HTTP;
- Disponibilizar os arquivos estáticos da interface, como HTML, CSS e JavaScript;
- Identificar a rota solicitada e verificar o método HTTP utilizado;
- Encaminhar as solicitações de autenticação para o processamento correspondente;
- Construir e devolver as respostas HTTP ao navegador.

A aplicação utiliza o endpoint `POST /autenticacao` para receber as solicitações de autenticação. Essa rota exige que o conteúdo da requisição esteja no formato JSON, identificado pelo cabeçalho `Content-Type: application/json`.

No ambiente de desenvolvimento local, o servidor utiliza, por padrão, o endereço `127.0.0.1` e a porta `3000`, podendo ter sua configuração ajustada conforme o ambiente de execução.

## 4.4. Servidor de Aplicação

O servidor de aplicação corresponde à parte responsável pela execução das regras e operações da aplicação. Neste projeto, essa função também é desempenhada pelo processo servidor implementado em C.

Após receber uma requisição de autenticação válida em sua estrutura, o controlador HTTP interpreta os dados enviados pelo navegador e os encaminha ao módulo responsável pela autenticação.

O processamento envolve as seguintes responsabilidades:

1. Interpretar e validar os campos de entrada, identificador e senha;
2. Localizar o usuário correspondente no conjunto de dados carregado em memória;
3. Verificar a senha fornecida utilizando o mecanismo PBKDF2-HMAC-SHA-256;
4. Determinar se as credenciais são válidas;
5. Produzir o resultado da autenticação e encaminhá-lo ao controlador HTTP para construção da resposta.

O repositório de usuários é representado por um arquivo JSON, carregado durante a inicialização do servidor para uma estrutura de dados em memória denominada `UserDatabase`. Dessa maneira, a busca pelos usuários ocorre diretamente nos registros carregados, sem a necessidade de consultar o arquivo novamente a cada requisição.

O protótipo utiliza um limite de 256 registros de usuários e mantém o repositório somente para leitura, não oferecendo operações de cadastro ou alteração de credenciais.

Após o processamento, o servidor retorna uma resposta HTTP ao navegador. Em caso de sucesso, utiliza o status `200 OK` e retorna a identificação do usuário autenticado. Para credenciais inválidas, utiliza `401 Unauthorized`. Já as falhas internas de processamento são representadas pelo status `500 Internal Server Error`.

## 4.5. Considerações sobre a integração dos componentes

A integração dos elementos ocorre por meio da comunicação HTTP entre o navegador e o processo servidor em C. A interface Web e a API compartilham o mesmo processo de execução, enquanto o módulo de autenticação e o repositório de usuários são utilizados internamente pelo backend.

Essa organização simplifica a infraestrutura do protótipo, pois dispensa a configuração de servidores independentes para a interface e para a API. Além disso, mantém a responsabilidade pela validação das credenciais concentrada no backend, evitando que a interface Web determine diretamente o resultado da autenticação.

Portanto, o ambiente operacional pode ser caracterizado como uma aplicação Web cliente-servidor, com interface executada no navegador, processamento centralizado em um servidor desenvolvido em C e armazenamento local de dados em formato JSON, carregado em memória durante a inicialização.
