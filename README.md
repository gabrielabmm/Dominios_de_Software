# APS - Domínios de Software (2026/2)

**Autor:** Emilly Ranny

**Revisor:** 

Repositório da Atividade Prática Supervisionada (APS) da disciplina **Domínios de Software**, do curso de Bacharelado em Engenharia de Software da Universidade Federal de Goiás (UFG).

## Apresentação

O projeto reúne a especificação e um protótipo de aplicação web restrita ao domínio de **autenticação de usuários**: recepção de credenciais, verificação do usuário, comparação da senha e decisão de autenticação. O protótipo utiliza HTML, CSS e JavaScript na interface, com API em C e repositório JSON somente para leitura.

## Equipe e autoria

Conforme as diretrizes da disciplina, os artefatos são produzidos e revisados pelos membros da equipe:

- **Membro 1:** Gabriela Brasileiro
- **Membro 2:** Emilly Ranny
- **Membro 3:** Thyago Siriano

- **Docente responsável:** Profa. Dra. Adriana Silveira Souza (`adriana.silveira.souza@gmail.com`)

## Estrutura do repositório

```text
├── backend/                # Protótipo da API e testes automatizados
├── 1_Escopo_e_Requisitos_Autenticacao.md
├── 2_Arquitetura_Dados_e_Algoritmos.md
├── 3_Cenarios_Sucesso_e_Falha.md
├── GLOSSARY.md             # Vocabulário do domínio de autenticação
├── RELATORIO_AUDITORIA_APS.md
└── README.md               # Apresentação do repositório
```

O protótipo e seus passos de execução estão descritos em [backend/README.md](./backend/README.md). Os testes automatizados podem ser executados na pasta `backend` com `python -m unittest discover -s tests -v`.

Os apêndices e as figuras previstos em materiais externos da APS não estão incluídos neste repositório.
