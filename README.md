# X360Browser

**Navegador experimental que roda 100% diretamente no Xbox 360 RGH/JTAG.** É um aplicativo nativo `default.xex`, feito para o processador PowerPC do console. **Não precisa de PC, streaming, navegador remoto, proxy ou servidor intermediário** para funcionar.

A conexão é direta do Xbox 360 com os sites da internet. **Os servidores dos próprios sites continuam necessários**, assim como a conexão com a internet. Recursos como o catálogo X360HUB consultam dados públicos do próprio site.

![X360Browser](docs/green-home.png)

## Baixar e instalar

**Versão atual: 0.10.0 (experimental)**

[Baixar default.xex](https://github.com/heltindev/X360Browser/releases/download/v0.10.0-dom-foundation-preview/default.xex) · [Ver a versão atual](https://github.com/heltindev/X360Browser/releases/tag/v0.10.0-dom-foundation-preview)

1. Baixe o arquivo `default.xex` na página de Releases.
2. Copie o arquivo para o HD ou USB do Xbox 360 RGH/JTAG.
3. Execute o XEX pelo Aurora.

Não é necessário iniciar nada em um computador ou hospedar um servidor próprio. Para páginas HTTPS, mantenha a data/hora do console configuradas corretamente.

## O que já existe

- Interface escura e verde, estilo Smart TV, com barra que se recolhe.
- Mouse virtual pelo controle, até quatro abas, histórico e barra de endereço.
- Download direto de páginas HTML, CSS e imagens por HTTP/HTTPS.
- Suporte parcial a JavaScript e layout responsivo; imagens JPEG, PNG, BMP e primeiro quadro de GIF.
- Página inicial do Google com busca; resultados do Google ainda não funcionam corretamente.
- Alternativa opcional de resultados do Bing, identificada como Bing.
- Visualização do catálogo X360HUB com dados públicos recebidos diretamente pelo console.

**Atenção:** é uma versão em desenvolvimento. Nem todos os sites abrem corretamente. Os testes em PC não substituem um teste no Xbox 360 físico.

## Controles

| Botão | Função |
|---|---|
| Analógico esquerdo / direcional | Mover cursor |
| A | Clicar |
| LT / RT ou analógico direito | Rolar a página |
| B / LB | Voltar |
| RB | Avançar |
| Y / X | Endereço ou pesquisa |
| START / R3 | Recarregar |
| BACK | Página inicial |
| L3 | Menu |

## O que falta atualizar

- **Motor JavaScript moderno:** portar um motor compatível com o Xbox 360 PowerPC.
- **DOM completo:** eventos, formulários, atualização dinâmica das páginas e navegação.
- **Web APIs:** implementar fetch, XHR, cookies, armazenamento, tarefas assíncronas e compatibilidade HTTPS.
- **HTML/CSS:** melhorar Flexbox, Grid, fontes, posicionamento e páginas responsivas.
- **Google:** carregar e navegar pelos resultados reais, sem tela branca ou outro buscador no lugar.
- **Vídeos:** adicionar suporte real a MP4/H.264, reprodução HTML5 e, futuramente, sites como YouTube.
- **Estabilidade:** corrigir bugs, uso de memória, velocidade e testar cada recurso em um Xbox físico.

Tudo isso deve funcionar **localmente no próprio Xbox 360**, sem depender de PC ou servidor intermediário.

## Desenvolvimento

Código C/C++ para **Visual Studio 2010 + Xbox 360 SDK**. Para compilar, execute `build-release.cmd`; o resultado é `Release/default.xex`.

A versão atual usa litehtml e Duktape, com limitações conhecidas. O projeto é independente, não afiliado à Microsoft, LG ou Google. Licença: GPL-3.0. Bibliotecas e licenças em [THIRD_PARTY.md](THIRD_PARTY.md).

**Desenvolvido por [@heltonleiras](https://github.com/heltindev).**
