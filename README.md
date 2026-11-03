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
| [1. Escopo e Requisitos](./1_Escopo_e_Requisitos_Autenticacao.md) | Apresenta o objetivo, o escopo e os requisitos funcionais e não funcionais do sistema. |
| [2. Arquitetura, Dados e Algoritmos](./2_Arquitetura_Dados_e_Algoritmos.md) | Descreve a arquitetura, os módulos, os modelos de dados e os algoritmos utilizados. |
| [3. Cenários de Sucesso e Falha](./3_Cenarios_Sucesso_e_Falha.md) | Documenta os cenários de teste e os comportamentos esperados do sistema. |
| [4. Descrição Detalhada do Ambiente Operacional](./4_Descrição_Detalhada_Ambiente_Operacional.md) | Apresenta o ambiente operacional, as tecnologias e os requisitos para execução da aplicação. |
| [5. Interfaces, Dependências e Relações](./5_Interfaces_Dependencias_e_Relacoes.md) | Descreve as interfaces entre os componentes, suas dependências e as decisões de integração. |
| [Glossário](./GLOSSARY.md) | Reúne os principais termos técnicos utilizados no projeto. |
| [Relatório de Auditoria](./RELATORIO_AUDITORIA_APS.md) | Apresenta os registros e resultados da auditoria do projeto. |

## Diagramas

Os diagramas representam diferentes aspectos da estrutura e do funcionamento do sistema.

## Estrutura do repositório

```text
Dominios_de_Software/
├── backend/
│   └── README.md
├── Diagramas/
│   ├── Diagrama_Componentes/
│   │   ├── Diagrama_Comp.puml
│   │   └── Diagrama_Componentes.png
│   ├── Diagrama_Implantacao/
│   │   ├── Diagrama_Imp.puml
│   │   └── Diagrama_Implantacao.png
│   └── Diagrama_Interacao/
│       ├── Diagrama_Int.puml
│       └── DIagrama_Interacao.png
│   ├── Diagrama_Sequencia/
│   │   └── Diagrama_Sequencia.png
├── 1_Escopo_e_Requisitos_Autenticacao.md
├── 2_Arquitetura_Dados_e_Algoritmos.md
├── 3_Cenarios_Sucesso_e_Falha.md
├── 4_Descrição_Detalhada_Ambiente_Operacional.md
├── 5_Interfaces_Dependencias_e_Relacoes.md
├── GLOSSARY.md
├── RELATORIO_AUDITORIA_APS.md
├── render.yaml
└── README.md
```

O protótipo e seus passos de execução estão descritos em [backend/README.md](./backend/README.md). Os testes automatizados podem ser executados na pasta `backend` com `python -m unittest discover -s tests -v`.

Os apêndices e as figuras previstos em materiais externos da APS não estão incluídos neste repositório.
