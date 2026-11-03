# Autenticação de Usuários

**Autor:** Emilly Ranny

**Revisor:** Emilly Ranny

Vocabulário do domínio de autenticação de usuários e da verificação de credenciais.

## Linguagem

**Autenticação**:
Verificação de que as credenciais fornecidas correspondem às credenciais registradas para um usuário.
_Avoid_: Autorização, criação de sessão

**Credenciais**:
Identificador e senha apresentados para verificar a identidade de um usuário.
_Avoid_: Dados de cadastro

**Identificador**:
Valor que localiza um usuário para autenticação; pode ser um nome de usuário ou um e-mail, de acordo com a política definida pelo sistema.
_Avoid_: Identidade do Usuário

**Identidade do Usuário**:
Identificador estável do usuário retornado quando a verificação das credenciais tem sucesso.
_Avoid_: Credenciais, perfil do usuário

**Usuário**:
Pessoa representada por um registro existente no repositório e associada a um identificador e a uma credencial de autenticação.
_Avoid_: Sessão, conta autenticada
