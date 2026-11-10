
# APS - Domínios de Software (2026/2)

**Autor:** Emilly Ranny  
**Revisor:** Gabriela Brasileiro

Repositório da Atividade Prática Supervisionada (APS) da disciplina **Domínios de Software**, do curso de Bacharelado em Engenharia de Software da Universidade Federal de Goiás (UFG).

## Apresentação

O projeto reúne a especificação e um protótipo de aplicação web restrita ao domínio de **autenticação de usuários**: recepção de credenciais, verificação do usuário, comparação da senha e decisão de autenticação. O protótipo utiliza HTML, CSS e JavaScript na interface, com API em C e repositório JSON somente para leitura.

## Equipe e autoria

Conforme as diretrizes da disciplina, os artefatos são produzidos e revisados pelos membros da equipe:

- **Membro 1:** Gabriela Brasileiro
- **Membro 2:** Emilly Ranny
- **Membro 3:** Thyago Siriano

- **Docente responsável:** Profa. Dra. Adriana Silveira Souza (`adriana.silveira.souza@gmail.com`)

## Documentação

Os documentos do projeto estão organizados de acordo com seus respectivos conteúdos:

| Documento | Descrição |
|---|---|
| [1. Escopo e Requisitos de Autenticação](./1_Escopo_e_Requisitos_Autenticacao.md) | Apresenta o objetivo, os limites do sistema e os requisitos funcionais e não funcionais da autenticação. |
| [2. Arquitetura, Dados e Algoritmos](./2_Arquitetura_Dados_e_Algoritmos.md) | Descreve a arquitetura, os módulos, os modelos de dados, os algoritmos e o fluxo de autenticação. |
| [3. Cenários de Sucesso e Falha](./3_Cenarios_Sucesso_e_Falha.md) | Documenta os cenários de teste e os comportamentos esperados do sistema. |
| [4. Descrição Detalhada do Ambiente Operacional](./4_Descrição_Detalhada_Ambiente_Operacional.md) | Apresenta o ambiente operacional, as tecnologias e os requisitos para execução da aplicação. |
| [5. Interfaces, Dependências e Relações](./5_Interfaces_Dependencias_e_Relacoes.md) | Descreve as interfaces entre os componentes, suas dependências e as decisões de integração. |
| [6. Registros de Comunicação da Equipe](./6_Registros_da_Equipe.md) | Reúne os registros das reuniões da equipe e suas respectivas evidências. |
| [Vocabulário do Domínio (DD)](./DD.md) | Define os principais termos e conceitos do domínio de autenticação de usuários. |
| [Relatório de Auditoria](./RELATORIO_AUDITORIA_APS.md) | Apresenta a análise de conformidade e os resultados da auditoria dos artefatos do projeto. |

## Diagramas

Os diagramas representam diferentes aspectos da estrutura, implantação e funcionamento do sistema. Cada pasta contém, quando aplicável, o arquivo-fonte PlantUML (`.puml`) e sua versão renderizada (`.png`).

- **[Diagrama de Componentes](./Diagramas/Diagrama_Componentes/Diagrama_Componentes.png)** — apresenta a organização dos componentes da aplicação e suas relações.
- **[Diagrama de Implantação](./Diagramas/Diagrama_Implantacao/Diagrama_Implantacao.png)** — representa a distribuição dos elementos da aplicação no ambiente de execução.
- **[Diagrama de Interação](./Diagramas/Diagrama_Interacao/DIagrama_Interacao.png)** — ilustra a interação entre os elementos envolvidos no processo.
- **[Diagrama de Sequência](./Diagramas/Diagrama_Sequencia/Diagrama_Sequencia.png)** — detalha a sequência de comunicação durante o fluxo de autenticação.

## Planilha de autoria e revisão

**Observação:** a equipe também utiliza uma planilha no Google Sheets para organizar e acompanhar a autoria e a revisão dos artefatos.

[Acesse a planilha da equipe](https://docs.google.com/spreadsheets/d/1vnOIEZ_0hOgg_lEcIqwNEjSXt3X3a8gfPKegd1t6TTo/edit?gid=0#gid=0).

## Acesso ao protótipo

O protótipo e seus passos de execução estão descritos em [backend/README.md](./backend/README.md).

Para testar o login, utilize as credenciais abaixo:

- **E-mail:** `aluno@ufg.br`
- **Senha:** `123456`

Essas credenciais foram testadas e estão funcionando no ambiente de demonstração. **Utilize-as somente para testes; não são credenciais para uso em produção.**

Os testes automatizados podem ser executados na pasta `backend` com:

```bash
python -m unittest discover -s tests -v
```

## Estrutura do repositório

```text
Dominios_de_Software/
├── backend/
│   ├── data/
│   │   └── users.example.json
│   ├── public/
│   ├── src/
│   ├── tests/
│   └── README.md
├── Diagramas/
│   ├── Diagrama_Componentes/
│   │   ├── Diagrama_Comp.puml
│   │   └── Diagrama_Componentes.png
│   ├── Diagrama_Implantacao/
│   │   ├── Diagrama_Imp.puml
│   │   └── Diagrama_Implantacao.png
│   ├── Diagrama_Interacao/
│   │   ├── Diagrama_Int.puml
│   │   └── DIagrama_Interacao.png
│   └── Diagrama_Sequencia/
│       └── Diagrama_Sequencia.png
├── 1_Escopo_e_Requisitos_Autenticacao.md
├── 2_Arquitetura_Dados_e_Algoritmos.md
├── 3_Cenarios_Sucesso_e_Falha.md
├── 4_Descrição_Detalhada_Ambiente_Operacional.md
├── 5_Interfaces_Dependencias_e_Relacoes.md
├── 6_Registros_da_Equipe.md
├── DD.md
├── RELATORIO_AUDITORIA_APS.md
└── README.md
```

Os arquivos de dados locais com usuários de teste não são versionados. Consulte o README do backend para configurar o repositório de usuários e executar a aplicação.
