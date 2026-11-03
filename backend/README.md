# Protótipo da API de Autenticação

**Autor:** Emilly Ranny

**Revisor:** 

Aplicação de demonstração com interface estática em HTML/CSS/JavaScript e API HTTP escrita em C. A verificação definitiva das credenciais acontece exclusivamente no backend. O protótipo não usa SQL e não cadastra usuários, recupera senhas, cria sessões ou autoriza acesso a outros recursos.

## Compilar e executar localmente

Requisitos: GCC, Python 3 para os testes e OpenSSL no ambiente Linux. No Windows, o backend usa a API criptográfica do sistema e Winsock.

No PowerShell, a partir desta pasta:

```powershell
gcc -std=c11 -Wall -Wextra -Werror src/main.c src/auth.c -lws2_32 -o auth-server.exe
$env:AUTH_USERS_FILE = "data/users.example.json"
$env:WEB_ROOT = "public"
$env:HOST = "127.0.0.1"
$env:PORT = "3000"
.\auth-server.exe
```

O navegador pode acessar `http://127.0.0.1:3000`. A interface inclui um formulário simples e sem estilos; `POST /autenticacao` recebe JSON com `identificador` e `senha`. Em Linux, compile com `gcc -std=c11 -Wall -Wextra -Werror src/main.c src/auth.c -lcrypto -o auth-server`.

O backend carrega o repositório na inicialização, por `AUTH_USERS_JSON` ou pelo arquivo indicado em `AUTH_USERS_FILE`. Se nenhuma variável estiver definida, usa `data/users.example.json`. O formato é:

```json
{
  "usuarios": [
    {
      "usuarioId": "id-estavel",
      "identificador": "pessoa@example.com",
      "senhaHash": "$pbkdf2-sha256$600000$saltBase64SemPadding$hashBase64SemPadding"
    }
  ]
}
```

O hash é PBKDF2-HMAC-SHA-256, com 600.000 iterações, salt de 16 bytes e chave derivada de 32 bytes. O JSON deve ser provisionado por um processo externo confiável; a API é somente para leitura e não modifica usuários. Identificadores são comparados exatamente. Mudanças no arquivo exigem reiniciar o processo.

O arquivo de exemplo está vazio; sem provisionar ao menos um usuário, nenhuma autenticação poderá ter sucesso.

Os erros internos são registrados em JSON no `stderr`, sem mensagens, credenciais ou hashes. Não use usuários ou hashes de teste em produção.

## Hospedagem no Render

O repositório inclui um `render.yaml` na raiz. O serviço compila o backend em C, serve a interface e a API pela mesma origem e usa `PORT` fornecida pelo Render. Configure `AUTH_USERS_JSON` no ambiente do serviço com os registros provisionados; trate o conteúdo como dado sensível e não o inclua no repositório. O ambiente de build instala os cabeçalhos do OpenSSL necessários.

## Testes

Execute `python -m unittest discover -s tests -v`. A suíte compila o servidor e testa a API HTTP e os arquivos estáticos em portas e diretórios temporários. Não requer SQL ou serviços externos.
